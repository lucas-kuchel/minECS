#pragma once

#include <minECS/Internals/Archetype.hpp>
#include <minECS/Internals/BitsetTree.hpp>
#include <minECS/Internals/ECSDescriptor.hpp>
#include <minECS/Internals/EntityView.hpp>
#include <minECS/Internals/SparseSet.hpp>
#include <minECS/Internals/Traits.hpp>

#include <bitset>
#include <tuple>
#include <vector>

namespace minECS
{
    template <typename T>
    requires IsDescriptor<T>
    class ecs;

    template <typename T, typename... Args>
    requires IsDescriptor<descriptor<T, Args...>>
    class ecs<descriptor<T, Args...>>
    {
    public:
        using bitset = std::bitset<sizeof...(Args)>;

        using size_type = T;
        using Descriptor = ECSDescriptor<T, Args...>;

        ecs() = default;
        ~ecs() = default;

        ecs(const ecs&) = delete;
        ecs(ecs&&) noexcept = delete;

        ecs& operator=(const ecs&) = delete;
        ecs& operator=(ecs&&) noexcept = delete;

        [[nodiscard]] inline entity<T> create_blank_entity()
        {
            if (m_free_list.empty())
            {
                m_entities.push_back({static_cast<size_type>(m_entities.size()), 0});
                m_entity_masks.push_back(bitset{});

                return m_entities.back();
            }
            else
            {
                std::uint32_t index = m_free_list.back();

                m_free_list.pop_back();
                m_entities[index].get_generation()++;

                m_entity_masks[index].reset();

                return m_entities[index];
            }
        }

        [[nodiscard]] inline std::vector<entity<T>> create_blank_entities(std::size_t count)
        {
            std::vector<entity<T>> entities;

            entities.reserve(count);

            for (std::size_t i = 0; i < count; i++)
            {
                entities.push_back(create_blank_entity());
            }

            return entities;
        }

        template <typename... Args2>
        requires((descriptor::template contains<Args2> && ...) && sizeof...(Args2) != 0)
        inline entity<T> create_entity(Args2&&... components)
        {
            entity<T> e = create_blank_entity();

            auto mask = make_bitmask<Args2...>();

            m_entity_masks[e.get_id()] = mask;

            m_archetypes.get_or_insert(mask).insert(e.get_id(), e);

            (std::get<sparse_set<Args2, size_type>>(m_sparse_sets).insert(e.get_id(), std::forward<Args2>(components)), ...);

            return e;
        }

        template <typename... Args2>
        requires((descriptor::template contains<Args2> && ...) && sizeof...(Args2) != 0)
        inline std::vector<entity<T>> create_entities(std::size_t count, Args2&&... components)
        {
            std::vector<entity<T>> entities;

            entities.reserve(count);

            for (std::size_t i = 0; i < count; i++)
            {
                entities.push_back(create_entity(std::forward<Args2>(components)...));
            }

            return entities;
        }

        [[nodiscard]] inline bool destroy_entity(entity<T> entity)
        {
            if (has_entity(entity))
            {
                const bitset& bitset = m_entity_masks[entity.get_id()];

                remove_entity_from_sparse_sets(entity, bitset);

                if (archetype<size_type>* archetype = m_archetypes.get(bitset))
                {
                    if (!archetype->remove(entity.get_id()))
                    {
                        return false;
                    }

                    if (archetype->get_entities().get_dense().size() == 0)
                    {
                        m_archetypes.remove(bitset);
                    }
                }

                m_free_list.push_back(entity.get_id());
                m_entities[entity.get_id()] = {std::numeric_limits<size_type>::max(), entity.get_generation()};
                m_entity_masks[entity.get_id()].reset();
            }
            else
            {
                return false;
            }

            return true;
        }

        [[nodiscard]] inline bool destroy_entities(const std::vector<entity<T>>& entities)
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

        [[nodiscard]] inline bool has_entity(entity<T> entity) const
        {
            return entity.get_id() < m_entities.size() &&
                   m_entities[entity.get_id()].get_generation() == entity.get_generation() &&
                   m_entities[entity.get_id()].get_id() != std::numeric_limits<size_type>::max();
        }

        [[nodiscard]] inline bool has_entities(const std::vector<entity<T>>& entities) const
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
        [[nodiscard]] inline bool entity_has_component(entity<T> entity) const
        {
            if (has_entity(entity))
            {
                constexpr std::size_t index = descriptor::template index<U>();

                return m_entity_masks[entity.get_id()].test(index);
            }

            return false;
        }

        template <typename... Args2>
        requires((descriptor::template contains<Args2> && ...) && sizeof...(Args2) != 0)
        [[nodiscard]] inline bool entity_has_components(entity<T> entity) const
        {
            return (entity_has_component<Args2>(entity) && ...);
        }

        template <typename U>
        requires(descriptor::template contains<U>)
        [[nodiscard]] inline bool entities_have_component(const std::vector<entity<T>>& entities) const
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
        requires((descriptor::template contains<Args2> && ...) && sizeof...(Args2) != 0)
        [[nodiscard]] inline bool entities_have_components(const std::vector<entity<T>>& entities) const
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
        [[nodiscard]] inline bool add_component_to_entity(entity<T> entity, U&& component)
        {
            if (has_entity(entity))
            {
                auto& set = std::get<sparse_set<U, T>>(m_sparse_sets);

                if (!set.insert(entity.get_id(), std::forward<U>(component)))
                {
                    return false;
                }

                constexpr std::size_t index = descriptor::template index<U>();

                bitset& target_bitset = m_entity_masks[entity.get_id()];
                bitset old_bitset = m_entity_masks[entity.get_id()];

                target_bitset.set(index);

                return update_archetype(entity, old_bitset, target_bitset);
            }
            else
            {
                return false;
            }
        }

        template <typename U>
        requires(descriptor::template contains<U>)
        [[nodiscard]] inline bool add_component_to_entities(const std::vector<entity<T>>& entities, U&& component)
        {
            bool result = true;

            for (auto& entity : entities)
            {
                result &= add_component_to_entity(entity, std::forward<U>(component));
            }

            return result;
        }

        template <typename... Args2>
        requires((descriptor::template contains<Args2> && ...) && sizeof...(Args2) != 0)
        [[nodiscard]] inline bool add_components_to_entity(entity<T> entity, const Args2&... components)
        {
            return (add_component_to_entity<Args2>(entity, components) && ...);
        }

        template <typename... Args2>
        requires((descriptor::template contains<Args2> && ...) && sizeof...(Args2) != 0)
        [[nodiscard]] inline bool add_components_to_entities(const std::vector<entity<T>>& entities, const Args2&... components)
        {
            bool result = true;

            for (auto& entity : entities)
            {
                result &= add_components_to_entity<Args2...>(entity, components...);
            }

            return result;
        }

        template <typename U>
        requires(descriptor::template contains<U>)
        [[nodiscard]] inline bool remove_component_from_entity(entity<T> entity)
        {
            if (has_entity(entity))
            {
                auto& set = std::get<sparse_set<U, T>>(m_sparse_sets);

                if (!set.remove(entity.get_id()))
                {
                    return false;
                }

                constexpr std::size_t index = descriptor::template index<U>();

                bitset& target_bitset = m_entity_masks[entity.get_id()];
                bitset old_bitset = m_entity_masks[entity.get_id()];

                target_bitset.reset(index);

                return update_archetype(entity, old_bitset, target_bitset);
            }
            else
            {
                return false;
            }
        }

        template <typename U>
        requires(descriptor::template contains<U>)
        [[nodiscard]] inline bool remove_component_from_entities(const std::vector<entity<T>>& entities)
        {
            bool result = true;

            for (auto& entity : entities)
            {
                result &= remove_component_from_entity<U>(entity);
            }

            return result;
        }

        template <typename... Args2>
        requires((descriptor::template contains<Args2> && ...) && sizeof...(Args2) != 0)
        [[nodiscard]] inline bool remove_components_from_entity(entity<T> entity)
        {
            return (remove_component_from_entity<Args2>(entity) && ...);
        }

        template <typename... Args2>
        requires((descriptor::template contains<Args2> && ...) && sizeof...(Args2) != 0)
        [[nodiscard]] inline bool remove_components_from_entities(const std::vector<entity<T>>& entities)
        {
            bool result = true;

            for (auto& entity : entities)
            {
                result &= remove_components_from_entity<Args2...>(entity);
            }

            return result;
        }

        [[nodiscard]] inline bitset_tree<archetype<size_type>, size_type, sizeof...(Args)>& get_archetypes()
        {
            return m_archetypes;
        }

        [[nodiscard]] inline const bitset_tree<archetype<size_type>, size_type, sizeof...(Args)>& get_archetypes() const
        {
            return m_archetypes;
        }

        [[nodiscard]] archetype<size_type>* get_archetype_by_mask(const bitset& mask)
        {
            return m_archetypes.get(mask);
        }

        [[nodiscard]] const archetype<size_type>* get_archetype_by_mask(const bitset& mask) const
        {
            return m_archetypes.get(mask);
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
        requires((descriptor::template contains<Args2> && ...) && sizeof...(Args2) != 0)
        [[nodiscard]] static inline constexpr bitset make_bitmask()
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
        [[nodiscard]] inline bool update_archetype(entity<T> entity, const bitset& old_bitset, const bitset& new_bitset)
        {
            if (old_bitset == new_bitset)
            {
                return false;
            }

            if (archetype<size_type>* old_archetype = m_archetypes.get(old_bitset))
            {
                if (!old_archetype->remove(entity.get_id()))
                {
                    return false;
                }

                if (old_archetype->get_entities().get_dense().size() == 0)
                {
                    m_archetypes.remove(old_bitset);
                }
            }

            if (!m_archetypes.get_or_insert(new_bitset).insert(entity.get_id(), entity))
            {
                return false;
            }

            return true;
        }

        template <std::size_t... Ns>
        inline void remove_entity_from_sparse_sets_impl(entity<T> entity, const bitset& mask, std::index_sequence<Ns...>)
        {
            ((mask.test(Ns) ? std::get<Ns>(m_sparse_sets).remove(entity.get_id()) : void()), ...);
        }

        inline void remove_entity_from_sparse_sets(entity<T> entity, const bitset& mask)
        {
            remove_entity_from_sparse_sets_impl(entity, mask, std::index_sequence_for<Args...>{});
        }

        std::tuple<sparse_set<Args, size_type>...> m_sparse_sets;

        bitset_tree<archetype<size_type>, size_type, sizeof...(Args)> m_archetypes;

        std::vector<bitset> m_entity_masks;
        std::vector<entity<T>> m_entities;
        std::vector<T> m_free_list;
    };
}
