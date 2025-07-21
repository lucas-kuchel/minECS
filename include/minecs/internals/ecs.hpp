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

        [[nodiscard]] inline bool destroy_entity(entity entity)
        {
            if (has_entity(entity))
            {
                const bitset& bitset = m_entity_masks[entity.id];

                remove_entity_from_sparse_sets(entity, bitset);

                if (archetype<size_type>* archetype = m_archetypes.get(bitset))
                {
                    if (!archetype->remove(entity.id))
                    {
                        return false;
                    }

                    if (archetype->get_entities().get_dense().size() == 0)
                    {
                        m_archetypes.remove(bitset);
                    }
                }

                m_free_list.push_back(entity.id);
                m_entities[entity.id] = {std::numeric_limits<size_type>::max(), entity.generation};
                m_entity_masks[entity.id].reset();
            }
            else
            {
                return false;
            }

            return true;
        }

        [[nodiscard]] inline bool destroy_entities(const std::vector<entity>& entities)
        {
            for (const auto& entity : entities)
            {
                if (!destroy_entity(entity))
                {
                    return false;
                }
            }

            return true;
        }

        [[nodiscard]] inline bool has_entity(entity entity) const
        {
            return entity.id < m_entities.size() && m_entities[entity.id].generation == entity.generation && m_entities[entity.id].id != std::numeric_limits<size_type>::max();
        }

        [[nodiscard]] inline bool has_entities(const std::vector<entity>& entities) const
        {
            for (const auto& entity : entities)
            {
                if (!has_entity(entity))
                {
                    return false;
                }
            }

            return true;
        }

        template <typename U>
        requires(descriptor::template contains<U>)
        [[nodiscard]] inline bool entity_has_component(entity entity) const
        {
            if (has_entity(entity))
            {
                constexpr std::size_t index = descriptor::template index<U>();

                return m_entity_masks[entity.id].test(index);
            }

            return false;
        }

        template <typename... Args2>
        requires(descriptor::template contains<Args2> && ...)
        [[nodiscard]] inline bool entity_has_components(entity entity) const
        {
            return (entity_has_component<Args2>(entity) && ...);
        }

        template <typename U>
        requires(descriptor::template contains<U>)
        [[nodiscard]] inline bool entities_have_component(const std::vector<entity>& entities) const
        {
            for (const auto& entity : entities)
            {
                if (!entity_has_component<U>(entity))
                {
                    return false;
                }
            }

            return true;
        }

        template <typename... Args2>
        requires(descriptor::template contains<Args2> && ...)
        [[nodiscard]] inline bool entities_have_components(const std::vector<entity>& entities) const
        {
            for (const auto& entity : entities)
            {
                if (!entity_has_components<Args2...>(entity))
                {
                    return false;
                }
            }

            return true;
        }

        template <typename U>
        requires(descriptor::template contains<U>)
        [[nodiscard]] inline bool add_component(entity entity, U&& component)
        {
            if (has_entity(entity))
            {
                auto& set = std::get<sparse_set<U, T>>(m_sparse_sets);

                if (!set.insert(entity.id, std::forward<U>(component)))
                {
                    return false;
                }

                constexpr std::size_t index = descriptor::template index<U>();

                bitset& target_bitset = m_entity_masks[entity.id];
                bitset old_bitset = m_entity_masks[entity.id];

                target_bitset.set(index);

                return update_archetype(entity, old_bitset, target_bitset);
            }
            else
            {
                return false;
            }
        }

        template <typename U, typename... Args2>
        requires(sizeof...(Args2) > 0)
        [[nodiscard]] inline bool add_component(entity entity, Args2&&... args)
        {
            if (has_entity(entity))
            {
                auto& set = std::get<sparse_set<U, T>>(m_sparse_sets);

                if (!set.insert(entity.id, U(std::forward<Args2>(args)...)))
                {
                    return false;
                }

                constexpr std::size_t index = descriptor::template index<U>();

                bitset& target_bitset = m_entity_masks[entity.id];
                bitset old_bitset = m_entity_masks[entity.id];

                target_bitset.set(index);

                return update_archetype(entity, old_bitset, target_bitset);
            }
            else
            {
                return false;
            }
        }

        template <typename... Args2>
        requires(descriptor::template contains<Args2> && ...)
        [[nodiscard]] inline bool add_components(entity entity, const Args2&... components)
        {
            return (add_component<Args2>(entity, components) && ...);
        }

        template <typename U>
        requires(descriptor::template contains<U>)
        [[nodiscard]] inline bool remove_component(entity entity)
        {
            if (has_entity(entity))
            {
                auto& set = std::get<sparse_set<U, T>>(m_sparse_sets);

                if (!set.remove(entity.id))
                {
                    return false;
                }

                constexpr std::size_t index = descriptor::template index<U>();

                bitset& target_bitset = m_entity_masks[entity.id];
                bitset old_bitset = m_entity_masks[entity.id];

                target_bitset.reset(index);

                return update_archetype(entity, old_bitset, target_bitset);
            }
            else
            {
                return false;
            }
        }

        template <typename... Args2>
        requires(descriptor::template contains<Args2> && ...)
        [[nodiscard]] inline bool remove_components(entity entity)
        {
            return (remove_component<Args2>(entity) && ...);
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
        [[nodiscard]] static inline constexpr bitset get_bitmask()
        {
            bitset bitset;

            (bitset.set(descriptor::template index<Args2>()), ...);

            return bitset;
        }

        template <typename... Args2>
        requires(descriptor::template contains<Args2> && ...)
        [[nodiscard]] inline auto get_entity_view(archetype<size_type>& archetype)
        {
            return entity_view<ecs<descriptor>, T, Args2...>(this, archetype.get_entities());
        }

        template <typename... Args2>
        requires(descriptor::template contains<Args2> && ...)
        [[nodiscard]] inline const auto get_entity_view(archetype<size_type>& archetype) const
        {
            return entity_view<ecs<descriptor>, T, Args2...>(this, archetype.get_entities());
        }

    private:
        [[nodiscard]] inline bool update_archetype(entity entity, const bitset& old_bitset, const bitset& new_bitset)
        {
            if (old_bitset == new_bitset)
            {
                return false;
            }

            if (archetype<size_type>* old_archetype = m_archetypes.get(old_bitset))
            {
                if (!old_archetype->remove(entity.id))
                {
                    return false;
                }

                if (old_archetype->get_entities().get_dense().size() == 0)
                {
                    m_archetypes.remove(old_bitset);
                }
            }

            if (!m_archetypes.get_or_insert(new_bitset).insert(entity.id, entity))
            {
                return false;
            }

            return true;
        }

        template <std::size_t... Ns>
        inline void remove_entity_from_sparse_sets_impl(entity entity, const bitset& mask, std::index_sequence<Ns...>)
        {
            ((mask.test(Ns) ? std::get<Ns>(m_sparse_sets).remove(entity.id) : void()), ...);
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