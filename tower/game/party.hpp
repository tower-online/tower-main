#pragma once

#include <tower/network/client.hpp>

namespace tower::game {
using namespace tower::network;

class Party {
public:
    explicit Party(const uint32_t party_id)
        : party_id {party_id} {}

    void add_member(const std::shared_ptr<Client>& client);
    void remove_member(uint32_t client_id);

    bool is_member(const uint32_t client_id) const { return _members.contains(client_id); }

    auto begin() const { return _members.begin(); }
    auto end() const { return _members.end(); }

    const uint32_t party_id;

private:
    std::unordered_map<uint32_t, std::shared_ptr<Client>> _members {};
};


class PartyManager {
public:
    uint32_t add_party();
    std::vector<uint32_t> remove_party(uint32_t party_id);
    Party* get_party(uint32_t party_id) const;
    Party* get_current_party(uint32_t client_id) const;

    bool is_same_party(uint32_t client_id1, uint32_t client_id2) const;

    void broadcast(uint32_t party_id, const std::shared_ptr<flatbuffers::DetachedBuffer>& buffer);

private:
    std::unordered_map<uint32_t, std::unique_ptr<Party>> _parties {};
    std::unordered_map<uint32_t, uint32_t> _client_to_party {};

    std::atomic<uint32_t> _party_id_generator {0};
};


inline void Party::add_member(const std::shared_ptr<Client>& client) {
    _members.insert_or_assign(client->client_id, client);
}

inline void Party::remove_member(uint32_t client_id) {
    _members.erase(client_id);
}

inline uint32_t PartyManager::add_party() {
    auto party {std::make_unique<Party>(++_party_id_generator)};
    const auto party_id {party->party_id};

    _parties.insert_or_assign(party_id, std::move(party));

    return party_id;
}

/**
 *
 * @return The list of member clients' IDs.
 */
inline std::vector<uint32_t> PartyManager::remove_party(const uint32_t party_id) {
    if (!_parties.contains(party_id)) return {};

    std::vector<uint32_t> members_id {};
    for (const auto& [id, _] : *_parties.at(party_id)) {
        members_id.emplace_back(id);
    }
    _parties.erase(party_id);

    return members_id;
}

inline Party* PartyManager::get_party(const uint32_t party_id) const {
    if (!_parties.contains(party_id)) return nullptr;
    return _parties.at(party_id).get();
}

inline Party* PartyManager::get_current_party(const uint32_t client_id) const {
    if (!_client_to_party.contains(client_id)) return nullptr;
    return _parties.at(_client_to_party.at(client_id)).get();
}

inline bool PartyManager::is_same_party(const uint32_t client_id1, const uint32_t client_id2) const {
    if (!_client_to_party.contains(client_id1) || !_client_to_party.contains(client_id2)) return false;
    return _client_to_party.at(client_id1) == _client_to_party.at(client_id2);
}

inline void PartyManager::broadcast(const uint32_t party_id, const std::shared_ptr<flatbuffers::DetachedBuffer>& buffer) {
    if (!_parties.contains(party_id)) return;

    for (const auto& [_, client] : *_parties.at(party_id)) {
        client->send_packet(buffer);
    }
}
}
