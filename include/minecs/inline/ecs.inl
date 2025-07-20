#pragma once

#include <Mosaic/ECS/ECS.hpp>

namespace Mosaic::ECS
{
    template <typename... SupportedComponents>
    requires IsECSDescriptor<ECSDescriptor<SupportedComponents...>>
    ECS<ECSDescriptor<SupportedComponents...>>::ECS(const Console::Console& console)
        : mConsole(console)
    {
    }

    template <typename... SupportedComponents>
    requires IsECSDescriptor<ECSDescriptor<SupportedComponents...>>
    entity ECS<ECSDescriptor<SupportedComponents...>>::CreateEmptyEntity()
    {
        if (mFreeList.empty())
        {
            mEntities.push_back({static_cast<std::uint32_t>(mEntities.size()), 0});
            mEntityMasks.push_back(Bitset{});

            return mEntities.back();
        }
        else
        {
            std::uint32_t index = mFreeList.back();

            mFreeList.pop_back();
            mEntities[index].Generation++;

            return mEntities[index];
        }
    }

    template <typename... SupportedComponents>
    requires IsECSDescriptor<ECSDescriptor<SupportedComponents...>>
    std::vector<entity> ECS<ECSDescriptor<SupportedComponents...>>::CreateEmptyEntities(std::size_t count)
    {
        std::vector<entity> entities;
        entities.reserve(count);

        for (std::size_t i = 0; i < count; i++)
        {
            entities.push_back(CreateEntity());
        }

        return entities;
    }

    template <typename... SupportedComponents>
    requires IsECSDescriptor<ECSDescriptor<SupportedComponents...>>
    template <typename... Args2>
    requires(ECSDescriptor<SupportedComponents...>::template Contains<Args2> && ...)
    entity ECS<ECSDescriptor<SupportedComponents...>>::CreateEntity(Args2&&... components)
    {
        entity entity = CreateEmptyEntity();

        AddComponents<Args2...>(entity, std::forward<Args2>(components)...);

        return entity;
    }

    template <typename... SupportedComponents>
    requires IsECSDescriptor<ECSDescriptor<SupportedComponents...>>
    template <typename... Args2>
    requires(ECSDescriptor<SupportedComponents...>::template Contains<Args2> && ...)
    std::vector<entity> ECS<ECSDescriptor<SupportedComponents...>>::CreateEntities(std::size_t count, Args2&&... components)
    {
        std::vector<entity> entities;
        entities.reserve(count);

        for (std::size_t i = 0; i < count; i++)
        {
            entities.push_back(CreateEntity(std::forward<Args2>(components)...));
        }

        return entities;
    }

    template <typename... SupportedComponents>
    requires IsECSDescriptor<ECSDescriptor<SupportedComponents...>>
    void ECS<ECSDescriptor<SupportedComponents...>>::DestroyEntity(entity entity)
    {
        if (EntityExists(entity))
        {
            const Bitset& bitset = mEntityMasks[entity.ID];

            RemoveEntityFromsparse_sets(entity, bitset);

            if (archetype* archetype = mArchetypes.Get(bitset))
            {
                if (!archetype->Remove(entity.ID))
                {
                    mConsole.LogWarning("entity(ID: {}, Generation: {}) does not exist in this archetype", entity.ID, entity.Generation);
                }

                if (archetype->Size() == 0)
                {
                    mArchetypes.Delete(bitset);
                }
            }

            mFreeList.push_back(entity.ID);
            mEntities[entity.ID] = {std::numeric_limits<std::uint32_t>::max(), entity.Generation};
            mEntityMasks[entity.ID].reset();
        }
        else
        {
            mConsole.LogWarning("entity(ID: {}, Generation: {}) does not exist", entity.ID, entity.Generation);
        }
    }

    template <typename... SupportedComponents>
    requires IsECSDescriptor<ECSDescriptor<SupportedComponents...>>
    void ECS<ECSDescriptor<SupportedComponents...>>::DestroyEntities(const std::vector<entity>& entities)
    {
        for (const auto& entity : entities)
        {
            DestroyEntity(entity);
        }
    }

    template <typename... SupportedComponents>
    requires IsECSDescriptor<ECSDescriptor<SupportedComponents...>>
    bool ECS<ECSDescriptor<SupportedComponents...>>::EntityExists(entity entity) const
    {
        return entity.ID < mEntities.size() && mEntities[entity.ID].Generation == entity.Generation && mEntities[entity.ID].ID != std::numeric_limits<std::uint32_t>::max();
    }

    template <typename... SupportedComponents>
    requires IsECSDescriptor<ECSDescriptor<SupportedComponents...>>
    template <typename U>
    requires(ECSDescriptor<SupportedComponents...>::template Contains<U>)
    bool ECS<ECSDescriptor<SupportedComponents...>>::HasComponent(entity entity) const
    {
        if (EntityExists(entity))
        {
            constexpr std::size_t index = ECSDescriptor<SupportedComponents...>::template Index<U>();

            return mEntityMasks[entity.ID].test(index);
        }

        return false;
    }

    template <typename... SupportedComponents>
    requires IsECSDescriptor<ECSDescriptor<SupportedComponents...>>
    template <typename U>
    requires(ECSDescriptor<SupportedComponents...>::template Contains<U>)
    void ECS<ECSDescriptor<SupportedComponents...>>::AddComponent(entity entity, const U& component)
    {
        if (EntityExists(entity))
        {
            auto& sparse_set = std::get<Containers::sparse_set<U>>(msparse_sets);

            if (!sparse_set.Insert(entity.ID, component))
            {
                mConsole.LogWarning("entity(ID: {}, Generation: {}) already has a component of type {}", entity.ID, entity.Generation, Types::Reflection::TypeInfo<U>::Name);

                return;
            }

            constexpr std::size_t index = ECSDescriptor<SupportedComponents...>::template Index<U>();

            Bitset& bitset = mEntityMasks[entity.ID];
            Bitset oldBitset = bitset;

            bitset.set(index);

            UpdateArchetype(entity, oldBitset, bitset);
        }
        else
        {
            mConsole.LogWarning("entity(ID: {}, Generation: {}) does not exist", entity.ID, entity.Generation);
        }
    }

    template <typename... SupportedComponents>
    requires IsECSDescriptor<ECSDescriptor<SupportedComponents...>>
    template <typename... Args2>
    requires(ECSDescriptor<Args2...>::template Contains<Args2> && ...)
    void ECS<ECSDescriptor<SupportedComponents...>>::AddComponents(entity entity, const Args2&... components)
    {
        if (EntityExists(entity))
        {
            (AddComponent<Args2>(entity, components), ...);
        }
        else
        {
            mConsole.LogWarning("entity(ID: {}, Generation: {}) does not exist", entity.ID, entity.Generation);
        }
    }

    template <typename... SupportedComponents>
    requires IsECSDescriptor<ECSDescriptor<SupportedComponents...>>
    template <typename U>
    requires(ECSDescriptor<SupportedComponents...>::template Contains<U>)
    void ECS<ECSDescriptor<SupportedComponents...>>::RemoveComponent(entity entity)
    {
        if (EntityExists(entity))
        {
            auto& sparse_set = std::get<Containers::sparse_set<U>>(msparse_sets);

            if (!sparse_set.Remove(entity.ID))
            {
                mConsole.LogWarning("entity(ID: {}, Generation: {}) has no component of type {}", entity.ID, entity.Generation, Types::Reflection::TypeInfo<U>::Name);

                return;
            }

            constexpr std::size_t index = ECSDescriptor<SupportedComponents...>::template Index<U>();

            Bitset& bitset = mEntityMasks[entity.ID];
            Bitset oldBitset = bitset;

            bitset.reset(index);

            UpdateArchetype(entity, oldBitset, bitset);
        }
        else
        {
            mConsole.LogWarning("entity(ID: {}, Generation: {}) does not exist", entity.ID, entity.Generation);
        }
    }

    template <typename... SupportedComponents>
    requires IsECSDescriptor<ECSDescriptor<SupportedComponents...>>
    template <typename... Args2>
    requires(ECSDescriptor<Args2...>::template Contains<Args2> && ...)
    void ECS<ECSDescriptor<SupportedComponents...>>::RemoveComponents(entity entity)
    {
        if (EntityExists(entity))
        {
            (RemoveComponent<Args2>(entity), ...);
        }
        else
        {
            mConsole.LogWarning("entity(ID: {}, Generation: {}) does not exist", entity.ID, entity.Generation);
        }
    }

    template <typename... SupportedComponents>
    requires IsECSDescriptor<ECSDescriptor<SupportedComponents...>>
    [[nodiscard]] inline Containers::bitset_tree<sizeof...(SupportedComponents), archetype>& ECS<ECSDescriptor<SupportedComponents...>>::All()
    {
        return mArchetypes;
    }

    template <typename... SupportedComponents>
    requires IsECSDescriptor<ECSDescriptor<SupportedComponents...>>
    template <typename... Args2>
    requires(ECSDescriptor<SupportedComponents...>::template Contains<Args2> && ...)
    constexpr ECS<ECSDescriptor<SupportedComponents...>>::Bitset ECS<ECSDescriptor<SupportedComponents...>>::Mask()
    {
        Bitset bitset;

        (bitset.set(ECSDescriptor<SupportedComponents...>::template Index<Args2>()), ...);

        return bitset;
    }

    template <typename... SupportedComponents>
    requires IsECSDescriptor<ECSDescriptor<SupportedComponents...>>
    template <typename... Args2>
    requires(ECSDescriptor<SupportedComponents...>::template Contains<Args2> && ...)
    auto ECS<ECSDescriptor<SupportedComponents...>>::View(archetype& archetype)
    {
        return entity_view<ECS<ECSDescriptor<SupportedComponents...>>, Args2...>(this, archetype.GetEntities());
    }

    template <typename... SupportedComponents>
    requires IsECSDescriptor<ECSDescriptor<SupportedComponents...>>
    template <typename U>
    U& ECS<ECSDescriptor<SupportedComponents...>>::UnsafeGetComponent(entity entity)
    {
        auto& sparse_set = std::get<Containers::sparse_set<U>>(msparse_sets);

        return sparse_set.UnsafeGetFromSparse(entity.ID);
    }

    template <typename... SupportedComponents>
    requires IsECSDescriptor<ECSDescriptor<SupportedComponents...>>
    void ECS<ECSDescriptor<SupportedComponents...>>::UpdateArchetype(entity entity, const Bitset& oldBitset, const Bitset& newBitset)
    {
        if (oldBitset == newBitset)
        {
            return;
        }

        if (archetype* oldArchetype = mArchetypes.Get(oldBitset))
        {
            if (!oldArchetype->Remove(entity.ID))
            {
                mConsole.LogWarning("entity(ID: {}, Generation: {}) does not exist in this archetype", entity.ID, entity.Generation);
            }

            if (oldArchetype->Size() == 0)
            {
                mArchetypes.Delete(oldBitset);
            }
        }

        if (!mArchetypes.GetOrCreate(newBitset).Insert(entity.ID, entity))
        {
            mConsole.LogWarning("entity(ID: {}, Generation: {}) already exists in the new archetype", entity.ID, entity.Generation);
        }
    }

    template <typename... SupportedComponents>
    requires IsECSDescriptor<ECSDescriptor<SupportedComponents...>>
    template <std::size_t... Indices>
    void ECS<ECSDescriptor<SupportedComponents...>>::RemoveEntityFromsparse_setsImpl(entity entity, const Bitset& mask, std::index_sequence<Indices...>)
    {
        ((mask.test(Indices) ? std::get<Indices>(msparse_sets).Remove(entity.ID) : void()), ...);
    }

    template <typename... SupportedComponents>
    requires IsECSDescriptor<ECSDescriptor<SupportedComponents...>>
    void ECS<ECSDescriptor<SupportedComponents...>>::RemoveEntityFromsparse_sets(entity entity, const Bitset& mask)
    {
        RemoveEntityFromsparse_setsImpl(entity, mask, std::index_sequence_for<SupportedComponents...>{});
    }
}