#pragma once

#include <type_traits>

namespace minecs
{
    template <typename T>
    requires std::is_unsigned_v<T>
    class entity
    {
    public:
        using size_type = T;

        entity(size_type id = 0, size_type generation = 0)
            : m_id(id), m_generation(generation)
        {
        }

        [[nodiscard]] size_type& get_id()
        {
            return m_id;
        }

        [[nodiscard]] const size_type& get_id() const
        {
            return m_id;
        }

        [[nodiscard]] size_type& get_generation()
        {
            return m_generation;
        }

        [[nodiscard]] const size_type& get_generation() const
        {
            return m_generation;
        }

        bool operator==(const entity& other) const
        {
            return m_id == other.m_id && m_generation == other.m_generation;
        }

        bool operator!=(const entity& other) const
        {
            return !(*this == other);
        }

    private:
        T m_id;
        T m_generation;
    };
}