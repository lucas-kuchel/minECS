#pragma once

#include <minECS/Internals/ECSDescriptor.hpp>
#include <minECS/Internals/Entity.hpp>
#include <minECS/Internals/SparseSet.hpp>

namespace minECS
{
    template <typename TSizeType>
    class Archetype
    {
    public:
        using SizeType = TSizeType;

        using Iterator = typename SparseSet<Entity<SizeType>, SizeType>::Iterator;
        using ConstIterator = typename SparseSet<Entity<SizeType>, SizeType>::ConstIterator;

        Archetype() = default;
        ~Archetype() = default;

        Archetype(const Archetype&) = delete;
        Archetype(Archetype&&) noexcept = default;

        Archetype& operator=(const Archetype&) = delete;
        Archetype& operator=(Archetype&&) noexcept = default;

        [[nodiscard]] inline bool Insert(SizeType index, Entity<SizeType> entity)
        {
            return Entities.Insert(index, entity);
        }

        [[nodiscard]] inline bool Remove(SizeType id)
        {
            return Entities.Remove(id);
        }

        [[nodiscard]] inline SparseSet<Entity<SizeType>, SizeType>& GetEntities()
        {
            return Entities;
        }

        [[nodiscard]] inline const SparseSet<Entity<SizeType>, SizeType>& GetEntities() const
        {
            return Entities;
        }

        [[nodiscard]] inline Iterator begin()
        {
            return Entities.begin();
        }

        [[nodiscard]] inline Iterator end()
        {
            return Entities.end();
        }

        [[nodiscard]] inline ConstIterator begin() const
        {
            return Entities.begin();
        }

        [[nodiscard]] inline ConstIterator end() const
        {
            return Entities.end();
        }

        [[nodiscard]] inline ConstIterator cbegin() const
        {
            return Entities.cbegin();
        }

        [[nodiscard]] inline ConstIterator cend() const
        {
            return Entities.cend();
        }

    private:
        SparseSet<Entity<SizeType>, SizeType> Entities;
    };
}