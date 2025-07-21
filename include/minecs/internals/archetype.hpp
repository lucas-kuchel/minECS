#pragma once

#include <minecs/internals/entity.hpp>
#include <minecs/internals/sparse_set.hpp>

namespace minecs
{
    template <typename T>
    requires std::is_integral_v<T>
    class archetype
    {
    public:
        using size_type = T;

        archetype() = default;
        ~archetype() = default;

        archetype(const archetype&) = delete;
        archetype(archetype&&) noexcept = default;

        archetype& operator=(const archetype&) = delete;
        archetype& operator=(archetype&&) noexcept = default;

        [[nodiscard]] bool insert(size_type id, entity entity)
        {
            return m_entities.insert(id, entity);
        }

        [[nodiscard]] bool remove(size_type id)
        {
            return m_entities.remove(id);
        }

        [[nodiscard]] sparse_set<entity, size_type>& get_entities()
        {
            return m_entities;
        }

    private:
        sparse_set<entity, size_type> m_entities;
    };
}