#pragma once

#include <Mosaic/ECS/entity_view.hpp>

namespace Mosaic::ECS
{
    template <typename ECS, typename... Args2>
    entity_view::iterator<ECS, Args2...>::entity_view::iterator(ECS* registry, Containers::sparse_set<entity>* entities, std::size_t index, std::size_t end)
        : mRegistry(registry), mEntities(entities), mIndex(index), mEnd(end)
    {
    }

    template <typename ECS, typename... Args2>
    auto entity_view::iterator<ECS, Args2...>::operator*() const
    {
        entity entity = mEntities->UnsafeGetFromDense(mIndex);

        return std::tuple<entity, Args2&...>(entity, mRegistry->template UnsafeGetComponent<Args2>(entity)...);
    }

    template <typename ECS, typename... Args2>
    bool entity_view::iterator<ECS, Args2...>::operator!=(const entity_view::iterator<ECS, Args2...>&) const
    {
        return mIndex < mEnd;
    }

    template <typename ECS, typename... Args2>
    entity_view::iterator<ECS, Args2...>& entity_view::iterator<ECS, Args2...>::operator++()
    {
        ++mIndex;

        return *this;
    }

    template <typename ECS, typename... Args2>
    entity_view<ECS, Args2...>::entity_view(ECS* registry, Containers::sparse_set<entity>& entities)
        : mRegistry(registry), mEntities(&entities)
    {
    }

    template <typename ECS, typename... Args2>
    auto entity_view<ECS, Args2...>::begin()
    {
        std::size_t end = mEntities->DenseSize();

        return entity_view::iterator<ECS, Args2...>(mRegistry, mEntities, 0, end);
    }

    template <typename ECS, typename... Args2>
    auto entity_view<ECS, Args2...>::end()
    {
        std::size_t end = mEntities->DenseSize();

        return entity_view::iterator<ECS, Args2...>(mRegistry, mEntities, end, end);
    }

    template <typename ECS, typename... Args2>
    auto entity_view<ECS, Args2...>::begin() const
    {
        std::size_t end = mEntities->DenseSize();

        return entity_view::iterator<ECS, Args2...>(mRegistry, mEntities, 0, end);
    }

    template <typename ECS, typename... Args2>
    auto entity_view<ECS, Args2...>::end() const
    {
        std::size_t end = mEntities->DenseSize();

        return entity_view::iterator<ECS, Args2...>(mRegistry, mEntities, end, end);
    }
}