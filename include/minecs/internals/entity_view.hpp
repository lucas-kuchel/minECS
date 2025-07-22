#pragma once

#include <minecs/internals/entity.hpp>
#include <minecs/internals/sparse_set.hpp>

namespace minecs
{
    template <typename T, typename U, typename... Args>
    class entity_view
    {
    public:
        using size_type = U;

        class iterator
        {
        public:
            iterator(T* ecs, sparse_set<entity, size_type>* entities, std::size_t index, std::size_t end)
                : m_ecs(ecs), m_entities(entities), m_index(index), m_end(end)
            {
            }

            auto operator*() const
            {
                entity entity = m_entities->get_dense()[m_index];

                return std::tuple<struct entity, Args&...>(
                    entity,
                    m_ecs->template get_sparse_set<Args>()
                        .get_dense()[m_ecs->template get_sparse_set<Args>()
                                         .get_sparse()[entity.id]]...);
            }

            bool operator!=(const entity_view::iterator&) const
            {
                return m_index < m_end;
            }

            entity_view::iterator& operator++()
            {
                ++m_index;

                return *this;
            }

        private:
            T* m_ecs;

            sparse_set<entity, size_type>* m_entities;

            std::size_t m_index;
            std::size_t m_end;
        };

        using const_iterator = const iterator;

        entity_view(T* ecs, sparse_set<entity, size_type>& entities)
            : m_ecs(ecs), m_entities(&entities)
        {
        }

        [[nodiscard]] iterator begin()
        {
            std::size_t end = m_entities->get_dense().size();

            return entity_view::iterator(m_ecs, m_entities, 0, end);
        }

        [[nodiscard]] iterator end()
        {
            std::size_t end = m_entities->get_dense().size();

            return entity_view::iterator(m_ecs, m_entities, end, end);
        }

        [[nodiscard]] const_iterator begin() const
        {
            std::size_t end = m_entities->get_dense().size();

            return entity_view::iterator(m_ecs, m_entities, 0, end);
        }

        [[nodiscard]] const_iterator end() const
        {
            std::size_t end = m_entities->get_dense().size();

            return entity_view::iterator(m_ecs, m_entities, end, end);
        }

    private:
        T* m_ecs;

        sparse_set<entity, size_type>* m_entities;
    };
}
