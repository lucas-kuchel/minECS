#pragma once

#include <minecs/internals/sparse_set.hpp>

#include <algorithm>

namespace Mosaic::Containers
{
    template <typename T>
    inline bool sparse_set<T>::Insert(std::uint32_t index, const T& element)
    {
        if (index >= mSparse.size())
        {
            mSparse.resize(index + 1, DEAD_INDEX);
        }

        if (mSparse[index] != DEAD_INDEX)
        {
            return false;
        }

        mSparse[index] = mDense.size();

        mDense.push_back(element);
        mDenseToIndex.push_back(index);

        return true;
    }

    template <typename T>
    inline bool sparse_set<T>::Remove(std::uint32_t index)
    {
        if (index >= mSparse.size())
        {
            return false;
        }

        if (mSparse[index] == DEAD_INDEX)
        {
            return false;
        }

        std::uint32_t lastIndex = static_cast<std::uint32_t>(mDense.size() - 1);
        std::uint32_t lastEntity = mDenseToIndex[lastIndex];
        std::uint32_t denseIndex = mSparse[index];

        if (denseIndex != lastIndex)
        {
            mDense[denseIndex] = std::move(mDense[lastIndex]);
            mDenseToIndex[denseIndex] = lastEntity;

            mSparse[lastEntity] = denseIndex;
        }

        mSparse[index] = DEAD_INDEX;

        mDense.pop_back();
        mDenseToIndex.pop_back();

        return true;
    }

    template <typename T>
    inline std::optional<T&> sparse_set<T>::Get(std::uint32_t index)
    {
        if (index >= mSparse.size())
        {
            return std::nullopt;
        }

        if (mSparse[index] == DEAD_INDEX)
        {
            return std::nullopt;
        }

        return mDense[mSparse[index]];
    }

    template <typename T>
    inline std::optional<const T&> sparse_set<T>::Get(std::uint32_t index) const
    {
        if (index >= mSparse.size())
        {
            return std::nullopt;
        }

        if (mSparse[index] == DEAD_INDEX)
        {
            return std::nullopt;
        }

        return mDense[mSparse[index]];
    }

    template <typename T>
    T& sparse_set<T>::UnsafeGetFromDense(std::uint32_t index)
    {
        return mDense[index];
    }

    template <typename T>
    const T& sparse_set<T>::UnsafeGetFromDense(std::uint32_t index) const
    {
        return mDense[index];
    }

    template <typename T>
    T& sparse_set<T>::UnsafeGetFromSparse(std::uint32_t index)
    {
        return mDense[mSparse[index]];
    }

    template <typename T>
    const T& sparse_set<T>::UnsafeGetFromSparse(std::uint32_t index) const
    {
        return mDense[mSparse[index]];
    }

    template <typename T>
    inline bool sparse_set<T>::Has(std::uint32_t index) const
    {
        if (index >= mSparse.size())
        {
            return false;
        }

        if (mSparse[index] == DEAD_INDEX)
        {
            return false;
        }

        return true;
    }

    template <typename T>
    auto sparse_set<T>::begin()
    {
        return mDense.begin();
    }

    template <typename T>
    auto sparse_set<T>::begin() const
    {
        return mDense.begin();
    }

    template <typename T>
    auto sparse_set<T>::end()
    {
        return mDense.end();
    }

    template <typename T>
    auto sparse_set<T>::end() const
    {
        return mDense.end();
    }

    template <typename T>
    std::size_t sparse_set<T>::DenseSize() const
    {
        return mDense.size();
    }

    template <typename T>
    std::size_t sparse_set<T>::SparseSize() const
    {
        return mSparse.size();
    }

    template <typename T>
    bool sparse_set<T>::Empty() const
    {
        return mDense.empty();
    }
}