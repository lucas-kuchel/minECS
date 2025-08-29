#pragma once

#include <minECS/Internals/Entity.hpp>
#include <minECS/Internals/SparseSet.hpp>
#include <minECS/Internals/Traits.hpp>

namespace minECS
{
    template <typename TECS, typename TSizeType, typename... TComponents>
    requires IsECS<TECS> && IsSizeType<TSizeType> && ComponentsAreUnique<TComponents...> && (TECS::DescriptorType::template Contains<TComponents> && ...)
    class EntityView
    {
    public:
        using SizeType = TSizeType;

        class Iterator
        {
        public:
            Iterator(TECS* ecs, SparseSet<Entity<SizeType>, SizeType>* entities, SizeType index, SizeType end)
                : ECS(ecs), Entities(entities), Index(index), End(end)
            {
            }

            auto operator*() const
            {
                Entity<SizeType> targetEntity = Entities->GetDense()[Index];

                return std::tuple<Entity<SizeType>, TComponents&...>(targetEntity, ECS->template GetSparseSet<TComponents>().GetDense()[ECS->template GetSparseSet<TComponents>().GetSparse()[targetEntity.GetID()]]...);
            }

            bool operator!=(const EntityView::Iterator&) const
            {
                return Index < End;
            }

            EntityView::Iterator& operator++()
            {
                ++Index;

                return *this;
            }

        private:
            TECS* ECS;

            SparseSet<Entity<SizeType>, SizeType>* Entities;

            SizeType Index;
            SizeType End;
        };

        using ConstIterator = const Iterator;

        EntityView(TECS* ecs, SparseSet<Entity<SizeType>, SizeType>& entities)
            : ECS(ecs), Entities(&entities), End(entities.GetDense().size())
        {
        }

        [[nodiscard]] Iterator begin()
        {
            return EntityView::Iterator(ECS, Entities, 0, End);
        }

        [[nodiscard]] Iterator end()
        {
            return EntityView::Iterator(ECS, Entities, End, End);
        }

        [[nodiscard]] ConstIterator cbegin() const
        {
            return EntityView::Iterator(ECS, Entities, 0, End);
        }

        [[nodiscard]] ConstIterator cend() const
        {
            return EntityView::Iterator(ECS, Entities, End, End);
        }

    private:
        TECS* ECS;

        SizeType End;

        SparseSet<Entity<SizeType>, SizeType>* Entities;
    };
}
