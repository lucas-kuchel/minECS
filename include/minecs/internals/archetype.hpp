#pragma once

#include <minecs/internals/entity.hpp>
#include <minecs/internals/sparse_set.hpp>
#include <minecs/internals/ecs_descriptor.hpp>

namespace minecs
{
    template <typename T>
    requires std::is_unsigned_v<T>
    class archetype
    {
    public:
        using size_type = T;
        using entity_type = entity<T>;

        using iterator = typename sparse_set<entity_type, size_type>::iterator;
        using const_iterator = typename sparse_set<entity_type, size_type>::const_iterator;

        archetype() = default;
        ~archetype() = default;

        archetype(const archetype&) = delete;
        archetype(archetype&&) noexcept = default;

        archetype& operator=(const archetype&) = delete;
        archetype& operator=(archetype&&) noexcept = default;

        [[nodiscard]] bool insert(size_type id, entity<T> entity)
        {
            return m_entities.insert(id, entity);
        }

        [[nodiscard]] bool remove(size_type id)
        {
            return m_entities.remove(id);
        }

        [[nodiscard]] sparse_set<entity<T>, size_type>& get_entities()
        {
            return m_entities;
        }

        [[nodiscard]] const sparse_set<entity<T>, size_type>& get_entities() const
        {
            return m_entities;
        }

    private:
        sparse_set<entity<T>, size_type> m_entities;
    };
}