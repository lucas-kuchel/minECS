#pragma once

#include <minECS/Internals/Traits.hpp>

#include <limits>
#include <vector>

namespace minECS
{
    template <typename T, typename TSizeType>
    requires IsSizeType<TSizeType>
    class SparseSet
    {
    public:
        using Type = T;
        using SizeType = TSizeType;

        using Iterator = typename std::vector<Type>::iterator;
        using ConstIterator = typename std::vector<Type>::const_iterator;

        inline SparseSet() = default;
        inline ~SparseSet() = default;

        inline SparseSet(const SparseSet&) = default;
        inline SparseSet& operator=(const SparseSet&) = default;

        inline SparseSet(SparseSet&&) noexcept = default;
        inline SparseSet& operator=(SparseSet&&) noexcept = default;

        static constexpr SizeType DeadIndex = std::numeric_limits<SizeType>::max();

        [[nodiscard]] inline bool Insert(SizeType index, const T& element)
        {
            if (index >= Sparse.size())
            {
                Sparse.resize(index + 1, DeadIndex);
            }

            if (Sparse[index] != DeadIndex)
            {
                return false;
            }

            Sparse[index] = Dense.size();

            Dense.push_back(element);
            ReverseMapping.push_back(index);

            return true;
        }

        template <typename... Args>
        [[nodiscard]] inline bool Insert(SizeType index, Args&&... args)
        {
            if (index >= Sparse.size())
            {
                Sparse.resize(index + 1, DeadIndex);
            }

            if (Sparse[index] != DeadIndex)
            {
                return false;
            }

            Sparse[index] = Dense.size();

            Dense.emplace_back(std::forward<Args>(args)...);
            ReverseMapping.push_back(index);

            return true;
        }

        [[nodiscard]] inline bool Remove(SizeType index)
        {
            if (index >= Sparse.size())
            {
                return false;
            }

            if (Sparse[index] == DeadIndex)
            {
                return false;
            }

            SizeType denseIndex = Sparse[index];
            SizeType lastIndex = static_cast<SizeType>(Dense.size() - 1);

            if (denseIndex == lastIndex)
            {
                Sparse[index] = DeadIndex;

                Dense.pop_back();
                ReverseMapping.pop_back();

                return true;
            }

            SizeType lastEntity = ReverseMapping[lastIndex];

            Dense[denseIndex] = std::move(Dense[lastIndex]);
            ReverseMapping[denseIndex] = lastEntity;
            Sparse[lastEntity] = denseIndex;

            Sparse[index] = DeadIndex;

            Dense.pop_back();
            ReverseMapping.pop_back();

            return true;
        }

        [[nodiscard]] inline Type* Get(SizeType index)
        {
            if (index >= Sparse.size())
            {
                return nullptr;
            }

            if (Sparse[index] == DeadIndex)
            {
                return nullptr;
            }

            return &Dense[Sparse[index]];
        }

        [[nodiscard]] inline const Type* Get(SizeType index) const
        {
            if (index >= Sparse.size())
            {
                return nullptr;
            }

            if (Sparse[index] == DeadIndex)
            {
                return nullptr;
            }

            return &Dense[Sparse[index]];
        }

        [[nodiscard]] inline bool Contains(SizeType index) const
        {
            if (index >= Sparse.size())
            {
                return false;
            }

            if (Sparse[index] == DeadIndex)
            {
                return false;
            }

            return true;
        }

        [[nodiscard]] inline Iterator begin() noexcept
        {
            return Dense.begin();
        }

        [[nodiscard]] inline ConstIterator begin() const noexcept
        {
            return Dense.begin();
        }

        [[nodiscard]] inline Iterator end() noexcept
        {
            return Dense.end();
        }

        [[nodiscard]] inline ConstIterator end() const noexcept
        {
            return Dense.end();
        }

        [[nodiscard]] inline ConstIterator cbegin() noexcept
        {
            return Dense.cbegin();
        }

        [[nodiscard]] inline ConstIterator cend() const noexcept
        {
            return Dense.cend();
        }

        [[nodiscard]] inline std::vector<Type>& GetDense()
        {
            return Dense;
        }

        [[nodiscard]] inline const std::vector<Type>& GetDense() const
        {
            return Dense;
        }

        [[nodiscard]] inline std::vector<TSizeType>& GetSparse()
        {
            return Sparse;
        }

        [[nodiscard]] inline const std::vector<TSizeType>& GetSparse() const
        {
            return Sparse;
        }

        [[nodiscard]] inline SizeType Size() const
        {
            return Dense.size();
        }

        [[nodiscard]] inline bool Empty() const
        {
            return Dense.empty();
        }

        inline void Clear()
        {
            Dense.clear();
            Sparse.clear();
            ReverseMapping.clear();
        }

        inline void ShrinkToFit()
        {
            Dense.shrink_to_fit();
            Sparse.shrink_to_fit();
            ReverseMapping.shrink_to_fit();
        }

    private:
        std::vector<Type> Dense;
        std::vector<SizeType> Sparse;
        std::vector<SizeType> ReverseMapping;
    };
}