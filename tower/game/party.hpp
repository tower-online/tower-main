#pragma once

#include <tower/network/client.hpp>
#include <tower/system/container/concurrent_map.hpp>

namespace tower::game {
using namespace std::chrono;
using namespace tower::network;

class PartyManager {
    class Party;

public:
    struct PendingRequest {
        explicit PendingRequest(const uint32_t requestee_id)
            : requestee_id {requestee_id}, _requested_time {steady_clock::now()} {}

        const uint32_t requestee_id;

        bool is_valid() const { return steady_clock::now() < _requested_time + MAX_WAIT_TIME; }

    private:
        static constexpr auto MAX_WAIT_TIME {10s};
        const steady_clock::time_point _requested_time;
    };

    uint32_t add_party();
    std::vector<uint32_t> remove_party(uint32_t party_id);
    std::optional<uint32_t> get_current_party_id(uint32_t client_id) const;

    bool is_same_party(uint32_t client_id1, uint32_t client_id2) const;

    bool add_member(uint32_t party_id, const std::shared_ptr<Client>& client);
    bool remove_member(uint32_t party_id, uint32_t client_id);
    void remove_client(uint32_t client_id);
    void broadcast(uint32_t party_id, const std::shared_ptr<flatbuffers::DetachedBuffer>& buffer);

    // <requester_id, request>
    ConcurrentMap<uint32_t, std::shared_ptr<PendingRequest>> pending_requests {};
private:
    // <client_id, Party>
    ConcurrentMap<uint32_t, std::shared_ptr<Party>> _parties {};
    // <client_id, party_id>
    ConcurrentMap<uint32_t, uint32_t> _client_to_party {};

    std::atomic<uint32_t> _party_id_generator {0};
};


class PartyManager::Party {
public:
    explicit Party(const uint32_t party_id, const size_t max_members = 4)
        : party_id {party_id}, _max_members {max_members} {}

    bool add_member(const std::shared_ptr<Client>& client);
    bool remove_member(uint32_t client_id);

    bool is_member(const uint32_t client_id) const { return _members.contains(client_id); }
    size_t max_members() const { return _max_members; }
    size_t size() const { return _members.size(); }
    bool empty() const { return _members.empty(); }

    auto get_members_guard();

    const uint32_t party_id;

private:
    ConcurrentMap<uint32_t, std::shared_ptr<Client>> _members {};
    size_t _max_members;
};


inline bool PartyManager::Party::add_member(const std::shared_ptr<Client>& client) {
    if (_members.size() >= _max_members) return false;
    _members.insert_or_assign(client->client_id, client);
    return true;
}

inline bool PartyManager::Party::remove_member(const uint32_t client_id) {
    return _members.erase(client_id) != 0;
}

inline auto PartyManager::Party::get_members_guard() {
    return _members.get_shared_guard();
}

inline uint32_t PartyManager::add_party() {
    auto party {std::make_shared<Party>(++_party_id_generator)};
    const auto party_id {party->party_id};

    _parties.insert_or_assign(party_id, std::move(party));

    return party_id;
}

/**
 *
 * @return The list of member clients' IDs.
 */
inline std::vector<uint32_t> PartyManager::remove_party(const uint32_t party_id) {
    std::shared_ptr<Party> party;
    if (!_parties.try_at(party_id, party)) return {};

    std::vector<uint32_t> members_id {};
    for (const auto members_guard {party->get_members_guard()}; const auto& [id, _] : members_guard.map) {
        members_id.emplace_back(id);
    }
    _parties.erase(party_id);

    return members_id;
}

inline std::optional<uint32_t> PartyManager::get_current_party_id(const uint32_t client_id) const {
    uint32_t party_id;
    if (!_client_to_party.try_at(client_id, party_id)) return {};
    return party_id;
}

inline bool PartyManager::is_same_party(const uint32_t client_id1, const uint32_t client_id2) const {
    uint32_t party_id1, party_id2;

    if (!_client_to_party.try_at(client_id1, party_id1) || !_client_to_party.try_at(client_id2, party_id2)) {
        return false;
    }
    return party_id1 == party_id2;
}

inline bool PartyManager::add_member(const uint32_t party_id, const std::shared_ptr<Client>& client) {
    // If already in the party, nope
    if (_client_to_party.contains(client->client_id)) return false;

    std::shared_ptr<Party> party;
    if (!_parties.try_at(party_id, party)) return false;
    _client_to_party.insert_or_assign(client->client_id, party->party_id);
    return party->add_member(client);
}

inline bool PartyManager::remove_member(const uint32_t party_id, const uint32_t client_id) {
    if (!_parties.contains(party_id)) return false;

    std::shared_ptr<Party> party;

    if (!_parties.try_at(party_id, party)) return false;
    const auto result {party->remove_member(client_id)};

    if (party->empty()) {
        remove_party(party_id);
    }

    return result;
}

inline void PartyManager::remove_client(const uint32_t client_id) {
    uint32_t party_id;
    if (!_client_to_party.try_at(client_id, party_id)) return;
    _client_to_party.erase(client_id);

    std::shared_ptr<Party> party;
    if (!_parties.try_at(party_id, party)) return;
    party->remove_member(client_id);
}

inline void PartyManager::broadcast(const uint32_t party_id, const std::shared_ptr<flatbuffers::DetachedBuffer>& buffer) {
    std::shared_ptr<Party> party;
    if (!_parties.try_at(party_id, party)) return;

    const auto members_guard {party->get_members_guard()};
    for (const auto& [_, client] : members_guard.map) {
        client->send_packet(buffer);
    }
}
}
