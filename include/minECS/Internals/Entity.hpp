#pragma once

#include <minECS/Internals/Traits.hpp>

namespace minECS
{
    template <typename TSizeType>
    requires IsSizeType<TSizeType>
    class Entity
    {
    public:
        using SizeType = TSizeType;

        Entity(SizeType id = 0, SizeType generation = 0)
            : ID(id), Generation(generation)
        {
        }

        [[nodiscard]] SizeType& GetID()
        {
            return ID;
        }

        [[nodiscard]] const SizeType& GetID() const
        {
            return ID;
        }

        [[nodiscard]] SizeType& GetGeneration()
        {
            return Generation;
        }

        [[nodiscard]] const SizeType& GetGeneration() const
        {
            return Generation;
        }

        bool operator==(const Entity& other) const
        {
            return ID == other.ID && Generation == other.Generation;
        }

        bool operator!=(const Entity& other) const
        {
            return !(*this == other);
        }

    private:
        SizeType ID;
        SizeType Generation;
    };
}