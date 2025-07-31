#pragma once

#include <minECS/Internals/Archetype.hpp>
#include <minECS/Internals/BitsetTree.hpp>
#include <minECS/Internals/ECSDescriptor.hpp>
#include <minECS/Internals/EntityView.hpp>
#include <minECS/Internals/SparseSet.hpp>
#include <minECS/Internals/Traits.hpp>

#include <bitset>
#include <tuple>
#include <vector>

namespace minECS
{
    template <typename SizeType>
    requires IsDescriptor<SizeType>
    class ECS;

    template <typename TSizeType, typename... TComponents>
    class ECS<ECSDescriptor<TSizeType, TComponents...>>
    {
    public:
        using Bitset = std::bitset<sizeof...(TComponents)>;
        using SizeType = TSizeType;
        using Descriptor = ECSDescriptor<TSizeType, TComponents...>;
        using EntityType = Entity<SizeType>;

        ECS() = default;
        ~ECS() = default;

        ECS(const ECS&) = delete;
        ECS(ECS&&) noexcept = delete;

        ECS& operator=(const ECS&) = delete;
        ECS& operator=(ECS&&) noexcept = delete;

        [[nodiscard]] inline Entity<SizeType> CreateBlankEntity()
        {
            if (FreeList.empty())
            {
                Entities.push_back({static_cast<SizeType>(Entities.size()), 0});
                EntityMasks.push_back(Bitset{});

                return Entities.back();
            }
            else
            {
                std::uint32_t index = FreeList.back();

                FreeList.pop_back();
                Entities[index].GetGeneration()++;

                EntityMasks[index].reset();

                return Entities[index];
            }
        }

        [[nodiscard]] inline std::vector<Entity<SizeType>> CreateBlankEntities(SizeType count)
        {
            std::vector<Entity<SizeType>> entities;

            entities.reserve(count);

            for (SizeType i = 0; i < count; i++)
            {
                entities.push_back(CreateBlankEntity());
            }

            return entities;
        }

        template <typename... TQueried>
        requires((Descriptor::template Contains<TQueried> && ...) && (sizeof...(TQueried) != 0))
        inline std::pair<bool, Entity<SizeType>> CreateEntity(TQueried&&... components)
        {
            Entity<SizeType> entity = CreateBlankEntity();

            auto mask = MakeBitmask<TQueried...>();

            EntityMasks[entity.GetID()] = mask;

            bool result = Archetypes.GetOrInsert(mask).Insert(entity.GetID(), entity);

            result &= (std::get<SparseSet<TQueried, SizeType>>(SparseSets).Insert(entity.GetID(), std::forward<TQueried>(components)) && ...);

            return {result, entity};
        }

        template <typename... TQueried>
        requires((Descriptor::template Contains<TQueried> && ...) && sizeof...(TQueried) != 0)
        inline std::vector<std::pair<bool, Entity<SizeType>>> CreateEntities(SizeType count, TQueried&&... components)
        {
            std::vector<std::pair<bool, Entity<SizeType>>> entities;

            entities.reserve(count);

            for (SizeType i = 0; i < count; i++)
            {
                entities.push_back(CreateEntity(std::forward<TQueried>(components)...));
            }

            return entities;
        }

        [[nodiscard]] inline bool DestroyEntity(Entity<SizeType> entity)
        {
            if (HasEntity(entity))
            {
                const Bitset& bitset = EntityMasks[entity.GetID()];

                RemoveEntityFromSparseSets(entity, bitset);

                if (Archetype<SizeType>* archetype = Archetypes.get(bitset))
                {
                    if (!archetype->Remove(entity.GetID()))
                    {
                        return false;
                    }

                    if (archetype->GetEntities().GetDense().size() == 0)
                    {
                        Archetypes.Remove(bitset);
                    }
                }

                FreeList.push_back(entity.GetID());
                Entities[entity.GetID()] = {std::numeric_limits<SizeType>::max(), entity.GetGeneration()};
                EntityMasks[entity.GetID()].reset();
            }
            else
            {
                return false;
            }

            return true;
        }

        [[nodiscard]] inline bool DestroyEntities(const std::vector<Entity<SizeType>>& entities)
        {
            bool result = true;

            for (const auto& entity : entities)
            {
                result &= DestroyEntity(entity);
            }

            return result;
        }

        [[nodiscard]] inline bool HasEntity(Entity<SizeType> entity) const
        {
            return entity.GetID() < Entities.size() &&
                   Entities[entity.GetID()].GetGeneration() == entity.GetGeneration() &&
                   Entities[entity.GetID()].GetID() != std::numeric_limits<SizeType>::max();
        }

        [[nodiscard]] inline bool HasEntities(const std::vector<Entity<SizeType>>& entities) const
        {
            for (const auto& entity : entities)
            {
                if (!HasEntity(entity))
                {
                    return false;
                }
            }

            return true;
        }

        template <typename U>
        requires(Descriptor::template Contains<U>)
        [[nodiscard]] inline bool EntityHasComponent(Entity<SizeType> entity) const
        {
            if (HasEntity(entity))
            {
                constexpr SizeType index = Descriptor::template Index<U>();

                return EntityMasks[entity.GetID()].test(index);
            }

            return false;
        }

        template <typename... TQueried>
        requires((Descriptor::template Contains<TQueried> && ...) && sizeof...(TQueried) != 0)
        [[nodiscard]] inline bool EntityHasComponents(Entity<SizeType> entity) const
        {
            return (EntityHasComponent<TQueried>(entity) && ...);
        }

        template <typename TComponent>
        requires(Descriptor::template Contains<TComponent>)
        [[nodiscard]] inline bool EntitiesHaveComponent(const std::vector<Entity<SizeType>>& entities) const
        {
            for (const auto& entity : entities)
            {
                if (!EntityHasComponent<TComponent>(entity))
                {
                    return false;
                }
            }

            return true;
        }

        template <typename... TQueried>
        requires((Descriptor::template Contains<TQueried> && ...) && sizeof...(TQueried) != 0)
        [[nodiscard]] inline bool EntitiesHaveCompoennts(const std::vector<Entity<SizeType>>& entities) const
        {
            for (const auto& entity : entities)
            {
                if (!EntityHasComponents<TQueried...>(entity))
                {
                    return false;
                }
            }

            return true;
        }

        template <typename TComponent>
        requires(Descriptor::template Contains<TComponent>)
        [[nodiscard]] inline bool AddComponentToEntity(Entity<SizeType> entity, TComponent&& component)
        {
            if (HasEntity(entity))
            {
                auto& set = std::get<SparseSet<TComponent, SizeType>>(SparseSets);

                if (!set.Insert(entity.GetID(), std::forward<TComponent>(component)))
                {
                    return false;
                }

                constexpr std::size_t index = Descriptor::template Index<TComponent>();

                Bitset& targetBitset = EntityMasks[entity.GetID()];
                Bitset oldBitset = EntityMasks[entity.GetID()];

                targetBitset.set(index);

                return update_archetype(entity, oldBitset, targetBitset);
            }
            else
            {
                return false;
            }
        }

        template <typename TComponent>
        requires(Descriptor::template Contains<TComponent>)
        [[nodiscard]] inline bool AddComponentToEntities(const std::vector<Entity<SizeType>>& entities, TComponent&& component)
        {
            bool result = true;

            for (auto& entity : entities)
            {
                result &= AddComponentToEntity(entity, std::forward<TComponent>(component));
            }

            return result;
        }

        template <typename... TQueried>
        requires((Descriptor::template Contains<TQueried> && ...) && sizeof...(TQueried) != 0)
        [[nodiscard]] inline bool AddComponentsToEntity(Entity<SizeType> entity, const TQueried&... components)
        {
            return (AddComponentToEntity<TQueried>(entity, components) && ...);
        }

        template <typename... TQueried>
        requires((Descriptor::template Contains<TQueried> && ...) && sizeof...(TQueried) != 0)
        [[nodiscard]] inline bool AddComponentsToEntities(const std::vector<Entity<SizeType>>& entities, const TQueried&... components)
        {
            bool result = true;

            for (auto& entity : entities)
            {
                result &= AddComponentsToEntity<TQueried...>(entity, components...);
            }

            return result;
        }

        template <typename TComponent>
        requires(Descriptor::template Contains<TComponent>)
        [[nodiscard]] inline bool RemoveComponentFromEntity(Entity<SizeType> entity)
        {
            if (HasEntity(entity))
            {
                auto& set = std::get<SparseSet<TComponent, SizeType>>(SparseSets);

                if (!set.Remove(entity.GetID()))
                {
                    return false;
                }

                constexpr std::size_t index = Descriptor::template Index<TComponent>();

                Bitset& targetBitset = EntityMasks[entity.GetID()];
                Bitset oldBitset = EntityMasks[entity.GetID()];

                targetBitset.reset(index);

                return UpdateArchetype(entity, oldBitset, targetBitset);
            }
            else
            {
                return false;
            }
        }

        template <typename TComponent>
        requires(Descriptor::template Contains<TComponent>)
        [[nodiscard]] inline bool RemoveComponentFromEntities(const std::vector<Entity<SizeType>>& entities)
        {
            bool result = true;

            for (auto& entity : entities)
            {
                result &= RemoveComponentFromEntity<TComponent>(entity);
            }

            return result;
        }

        template <typename... TQueried>
        requires((Descriptor::template Contains<TQueried> && ...) && sizeof...(TQueried) != 0)
        [[nodiscard]] inline bool RemoveComponentsFromEntity(Entity<SizeType> entity)
        {
            return (RemoveComponentFromEntity<TQueried>(entity) && ...);
        }

        template <typename... TQueried>
        requires((Descriptor::template Contains<TQueried> && ...) && sizeof...(TQueried) != 0)
        [[nodiscard]] inline bool RemoveComponentsFromEntity(const std::vector<Entity<SizeType>>& entities)
        {
            bool result = true;

            for (auto& entity : entities)
            {
                result &= RemoveComponentsFromEntity<TQueried...>(entity);
            }

            return result;
        }

        [[nodiscard]] inline BitsetTree<Archetype<SizeType>, SizeType, sizeof...(TComponents)>& GetArchetypess()
        {
            return Archetypes;
        }

        [[nodiscard]] inline const BitsetTree<Archetype<SizeType>, SizeType, sizeof...(TComponents)>& GetArchetypes() const
        {
            return Archetypes;
        }

        [[nodiscard]] Archetype<SizeType>* GetArchetype(const Bitset& mask)
        {
            return Archetypes.Get(mask);
        }

        [[nodiscard]] const Archetype<SizeType>* GetArchetype(const Bitset& mask) const
        {
            return Archetypes.Get(mask);
        }

        template <typename TComponent>
        requires(Descriptor::template Contains<TComponent>)
        [[nodiscard]] inline constexpr SparseSet<TComponent, SizeType>& GetSparseSet()
        {
            return std::get<SparseSet<TComponent, SizeType>>(SparseSets);
        }

        template <typename TComponent>
        requires(Descriptor::template Contains<TComponent>)
        [[nodiscard]] inline constexpr const SparseSet<TComponent, SizeType>& GetSparseSet() const
        {
            return std::get<SparseSet<TComponent, SizeType>>(SparseSets);
        }

        template <typename... TQueried>
        requires((Descriptor::template Contains<TQueried> && ...) && sizeof...(TQueried) != 0)
        [[nodiscard]] static inline constexpr Bitset MakeBitmask()
        {
            Bitset bitset;

            (bitset.set(Descriptor::template Index<TQueried>()), ...);

            return bitset;
        }

        template <typename... TQueried>
        requires(Descriptor::template Contains<TQueried> && ...)
        [[nodiscard]] inline auto GetEntityView(Archetype<SizeType>& archetype)
        {
            return EntityView<ECS<Descriptor>, SizeType, TQueried...>(this, archetype.GetEntities());
        }

        template <typename... TQueried>
        requires(Descriptor::template Contains<TQueried> && ...)
        [[nodiscard]] inline const auto GetEntityView(Archetype<SizeType>& archetype) const
        {
            return EntityView<ECS<Descriptor>, SizeType, TQueried...>(this, archetype.GetEntities());
        }

    private:
        [[nodiscard]] inline bool UpdateArchetype(Entity<SizeType> entity, const Bitset& oldBitset, const Bitset& newBitset)
        {
            if (oldBitset == newBitset)
            {
                return false;
            }

            if (Archetype<SizeType>* oldArchetype = Archetypes.Get(oldBitset))
            {
                if (!oldArchetype->Remove(entity.GetID()))
                {
                    return false;
                }

                if (oldArchetype->GetEntities().GetDense().size() == 0)
                {
                    Archetypes.Remove(oldBitset);
                }
            }

            if (!Archetypes.GetOrInsert(newBitset).Insert(entity.GetID(), entity))
            {
                return false;
            }

            return true;
        }

        template <std::size_t... Ns>
        inline void RemoveEntityFromSparseSetsImpl(Entity<SizeType> entity, const Bitset& mask, std::index_sequence<Ns...>)
        {
            ((mask.test(Ns) ? std::get<Ns>(SparseSets).Remove(entity.GetID()) : void()), ...);
        }

        inline void RemoveEntityFromSparseSets(Entity<SizeType> entity, const Bitset& mask)
        {
            RemoveEntityFromSparseSetsImpl(entity, mask, std::index_sequence_for<TComponents...>{});
        }

        std::tuple<SparseSet<TComponents, SizeType>...> SparseSets;

        BitsetTree<Archetype<SizeType>, SizeType, sizeof...(TComponents)> Archetypes;

        std::vector<Bitset> EntityMasks;
        std::vector<Entity<SizeType>> Entities;
        std::vector<SizeType> FreeList;
    };
}
