#pragma once

#include <tower/item/equipment/weapon/melee_attackable.hpp>
#include <tower/network/zone.hpp>
#include <tower/network/packet/entity_types.hpp>

namespace tower::skill {
class MeleeAttack {
public:
    static void use(network::Zone* zone, std::shared_ptr<entity::Entity>& user, const item::equipment::MeleeAttackable* weapon) {
        using namespace tower::network::packet;

        if (!user->state_machine.try_transition("Attacking")) return;

        const auto colliders =  zone->subworld()->get_collisions(
            weapon->melee_attack_shape(), std::to_underlying(physics::ColliderLayer::ENTITIES));

        std::vector<EntityResourceChange> damages {};
        for (const auto& collider : colliders) {
            entity::Entity* entity;
            if (entity = dynamic_cast<entity::Entity*>(collider->get_root().get()); !entity) continue;
            if (user->entity_id == entity->entity_id) continue; // Don't attack myself

            //TODO: Caculate damage
            const int damage {weapon->melee_attack_damage()};
            entity->get_damage(user, EntityResourceType::HEALTH, damage);
            damages.emplace_back(EntityResourceChangeMode::ADD, EntityResourceType::HEALTH, entity->entity_id, -damage);
        }

        // Broadcast that entity is damaged
        if (!damages.empty()) {
            flatbuffers::FlatBufferBuilder builder {512};
            const auto changes = CreateEntityResourceChangesDirect(builder, &damages);
            builder.FinishSizePrefixed(CreatePacketBase(builder, PacketType::EntityResourceChanges, changes.Union()));
            zone->broadcast(std::make_shared<flatbuffers::DetachedBuffer>(builder.Release()));
        }
        // Brodacast melee attack
        {
            flatbuffers::FlatBufferBuilder builder {128};
            const auto attack_replication = CreateSkillMeleeAttack(builder, user->entity_id);
            builder.FinishSizePrefixed(CreatePacketBase(builder, PacketType::SkillMeleeAttack, attack_replication.Union()));
            zone->broadcast(std::make_shared<flatbuffers::DetachedBuffer>(builder.Release()));
        }

        // Transition back to IDLE after attack interval
        user->state_machine.transition_delayed("Idle", zone->strand(), 1000ms);
    }
};
}