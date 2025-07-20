#pragma once

#include <minecs/internals/archetype.hpp>
#include <minecs/internals/bitset_tree.hpp>
#include <minecs/internals/ecs_descriptor.hpp>
#include <minecs/internals/entity.hpp>
#include <minecs/internals/entity_view.hpp>
#include <minecs/internals/sparse_set.hpp>

#include <bitset>
#include <tuple>
#include <vector>

namespace minecs
{
    template <typename T>
    requires is_ecs_descriptor_v<T>
    class ecs;

    template <typename T, typename... Args>
    requires is_ecs_descriptor_v<ecs_descriptor<T, Args...>>
    class ecs<ecs_descriptor<T, Args...>>
    {
    public:
        using bitset = std::bitset<sizeof...(Args)>;
        using size_type = T;
        using descriptor = ecs_descriptor<T, Args...>;

        ecs() = default;
        ~ecs() = default;

        ecs(const ecs&) = delete;
        ecs(ecs&&) noexcept = delete;

        ecs& operator=(const ecs&) = delete;
        ecs& operator=(ecs&&) noexcept = delete;

        [[nodiscard]] inline entity create_blank_entity()
        {
            if (m_free_list.empty())
            {
                m_entities.push_back({static_cast<std::uint32_t>(m_entities.size()), 0});
                m_entity_masks.push_back(bitset{});

                return m_entities.back();
            }
            else
            {
                std::uint32_t index = m_free_list.back();

                m_free_list.pop_back();
                m_entities[index].generation++;

                return m_entities[index];
            }
        }

        [[nodiscard]] inline std::vector<entity> create_blank_entities(std::size_t count)
        {
            std::vector<entity> entities;
            entities.reserve(count);

            for (std::size_t i = 0; i < count; i++)
            {
                entities.push_back(create_blank_entity());
            }

            return entities;
        }

        template <typename... Args2>
        requires(descriptor::template contains<Args2> && ...)
        inline entity create_entity(Args2&&... components)
        {
            entity entity = create_blank_entity();

            add_components<Args2...>(entity, std::forward<Args2>(components)...);

            return entity;
        }

        template <typename... Args2>
        requires(descriptor::template contains<Args2> && ...)
        inline std::vector<entity> create_entities(std::size_t count, Args2&&... components)
        {
            std::vector<entity> entities;
            entities.reserve(count);

            for (std::size_t i = 0; i < count; i++)
            {
                entities.push_back(create_entity(std::forward<Args2>(components)...));
            }

            return entities;
        }

        inline bool destroy_entity(entity entity)
        {
            if (has_entity(entity))
            {
                const bitset& bitset = m_entity_masks[entity.id];

                RemoveEntityFrom_sparse_sets(entity, bitset);

                if (archetype<size_type>* archetype = m_archetypes.get(bitset))
                {
                    if (!archetype->remove(entity.id))
                    {
                        return true;
                    }

                    if (archetype->entities().dense().size() == 0)
                    {
                        m_archetypes.remove(bitset);
                    }
                }

                m_free_list.push_back(entity.id);
                m_entities[entity.id] = {std::numeric_limits<std::uint32_t>::max(), entity.generation};
                m_entity_masks[entity.id].reset();
            }
            else
            {
                return true;
            }

            return false;
        }

        inline bool destroy_entities(const std::vector<entity>& entities)
        {
            for (const auto& entity : entities)
            {
                if (destroy_entity(entity))
                {
                    return true;
                }
            }

            return false;
        }

        [[nodiscard]] inline bool has_entity(entity entity) const
        {
            return entity.id < m_entities.size() && m_entities[entity.id].generation == entity.generation && m_entities[entity.id].id != std::numeric_limits<std::uint32_t>::max();
        }

        template <typename U>
        requires(descriptor::template contains<U>)
        [[nodiscard]] inline bool has_component(entity entity) const
        {
            if (has_entity(entity))
            {
                constexpr std::size_t index = descriptor::template index<U>();

                return m_entity_masks[entity.id].test(index);
            }

            return false;
        }

        template <typename U>
        requires(descriptor::template contains<U>)
        inline bool add_component(entity entity, const U& component = U())
        {
            if (has_entity(entity))
            {
                auto& set = std::get<sparse_set<U, T>>(m_sparse_sets);

                if (!set.insert(entity.id, component))
                {
                    return true;
                }

                constexpr std::size_t index = descriptor::template index<U>();

                bitset& bitset = m_entity_masks[entity.id];
                std::bitset<sizeof...(Args)> oldbitset = bitset;

                bitset.set(index);

                update_archetype(entity, oldbitset, bitset);

                return false;
            }
            else
            {
                return true;
            }
        }

        template <typename... Args2>
        requires(descriptor::template contains<Args2> && ...)
        inline void add_components(entity entity, const Args2&... components)
        {
            return (add_component<Args2>(entity, components) || ...);
        }

        template <typename U>
        requires(descriptor::template contains<U>)
        inline bool remove_component(entity entity)
        {
            if (has_entity(entity))
            {
                auto& set = std::get<sparse_set<U, T>>(m_sparse_sets);

                if (!set.remove(entity.id))
                {
                    return true;
                }

                constexpr std::size_t index = descriptor::template index<U>();

                bitset& bitset = m_entity_masks[entity.id];
                std::bitset<sizeof...(Args)> oldbitset = bitset;

                bitset.reset(index);

                update_archetype(entity, oldbitset, bitset);

                return false;
            }
            else
            {
                return true;
            }
        }

        template <typename... Args2>
        requires(descriptor::template contains<Args2> && ...)
        inline bool remove_components(entity entity)
        {
            return (RemoveComponent<Args2>(entity) || ...);
        }

        [[nodiscard]] inline bitset_tree<archetype<size_type>, size_type, sizeof...(Args)>& get_archetypes()
        {
            return m_archetypes;
        }

        [[nodiscard]] inline const bitset_tree<archetype<size_type>, size_type, sizeof...(Args)>& get_archetypes() const
        {
            return m_archetypes;
        }

        template <typename U>
        requires(descriptor::template contains<U>)
        [[nodiscard]] inline constexpr sparse_set<U, T>& get_sparse_set()
        {
            return std::get<sparse_set<U, T>>(m_sparse_sets);
        }

        template <typename U>
        requires(descriptor::template contains<U>)
        [[nodiscard]] inline constexpr const sparse_set<U, T>& get_sparse_set() const
        {
            return std::get<sparse_set<U, T>>(m_sparse_sets);
        }

        template <typename... Args2>
        requires(descriptor::template contains<Args2> && ...)
        [[nodiscard]] inline constexpr bitset get_bitmask()
        {
            bitset bitset;

            (bitset.set(descriptor::template index<Args2>()), ...);

            return bitset;
        }

        template <typename... Args2>
        requires(descriptor::template contains<Args2> && ...)
        [[nodiscard]] inline auto get_entity_view(archetype<size_type>& archetype)
        {
            return entity_view<ecs<descriptor>, T, Args2...>(this, archetype.entities());
        }

    private:
        inline bool update_archetype(entity entity, const bitset& oldbitset, const bitset& newbitset)
        {
            if (oldbitset == newbitset)
            {
                return true;
            }

            if (archetype<size_type>* oldArchetype = m_archetypes.get(oldbitset))
            {
                if (!oldArchetype->remove(entity.id))
                {
                    return true;
                }

                if (oldArchetype->entities().dense().size() == 0)
                {
                    m_archetypes.remove(oldbitset);
                }
            }

            if (!m_archetypes.get_or_insert(newbitset).insert(entity.id, entity))
            {
                return true;
            }

            return false;
        }

        template <std::size_t... Ns>
        inline void remove_entity_from_sparse_sets_impl(entity entity, const bitset& mask, std::index_sequence<Ns...>)
        {
            ((mask.test(Ns) ? std::get<Ns>(m_sparse_sets).Remove(entity.id) : void()), ...);
        }

        inline void remove_entity_from_sparse_sets(entity entity, const bitset& mask)
        {
            remove_entity_from_sparse_sets_impl(entity, mask, std::index_sequence_for<Args...>{});
        }

        std::tuple<sparse_set<Args, size_type>...> m_sparse_sets;

        bitset_tree<archetype<size_type>, size_type, sizeof...(Args)> m_archetypes;

        std::vector<bitset> m_entity_masks;
        std::vector<entity> m_entities;
        std::vector<std::uint32_t> m_free_list;
    };
}