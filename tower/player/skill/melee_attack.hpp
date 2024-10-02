#pragma once

#include <tower/item/equipment/weapon/melee_attackable.hpp>
#include <tower/item/equipment/weapon/weapon.hpp>
#include <tower/player/player.hpp>
#include <tower/player/skill/skill.hpp>
#include <tower/world/subworld.hpp>

namespace tower::player::skill {
class MeleeAttack final : public Skill {
public:
    struct Result {
        uint32_t target_entity_id;
    };

    static std::vector<Result> use(
        boost::asio::strand<boost::asio::any_io_executor>& strand, const std::shared_ptr<Player>& user,
        const std::shared_ptr<Weapon>& weapon, const Subworld& subworld);
};

inline std::vector<MeleeAttack::Result> MeleeAttack::use(boost::asio::strand<boost::asio::any_io_executor>& strand,
    const std::shared_ptr<Player>& user, const std::shared_ptr<Weapon>& weapon, const Subworld& subworld) {
    std::vector<Result> results {};
    const auto melee_attackable {dynamic_cast<const MeleeAttackable*>(weapon.get())};
    if (!melee_attackable) return {};
    if (!user->state_machine.try_transition("Attacking")) return {};

    const auto colliders =  subworld.get_collisions(
        melee_attackable->melee_attack_shape(), std::to_underlying(ColliderLayer::ENTITIES));

    for (const auto& collider : colliders) {
        if (user->node_id == collider->node_id) continue; // Don't attack myself
        if (const auto entity {dynamic_cast<const Entity*>(collider->get_root().get())}) {
            results.emplace_back(entity->entity_id);
        }
    }

    co_spawn(strand, [&strand, user]->boost::asio::awaitable<void> {
        //TODO: Get attack period from weapon
        boost::asio::steady_timer timer {strand, 1000ms};
        auto [_] = co_await timer.async_wait(as_tuple(boost::asio::use_awaitable));
        user->state_machine.try_transition("Idle");
    }, boost::asio::detached);

    return results;
}
}
