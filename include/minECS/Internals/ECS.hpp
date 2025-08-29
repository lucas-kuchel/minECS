#pragma once

#include <minECS/Internals/BitsetTree.hpp>
#include <minECS/Internals/ECSDescriptor.hpp>
#include <minECS/Internals/EntityView.hpp>
#include <minECS/Internals/SparseSet.hpp>
#include <minECS/Internals/Traits.hpp>

#include <bitset>
#include <iostream>
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
        using BitsetType = std::bitset<sizeof...(TComponents)>;
        using SizeType = TSizeType;
        using DescriptorType = ECSDescriptor<TSizeType, TComponents...>;
        using EntityType = Entity<SizeType>;
        using ArchetypeType = SparseSet<EntityType, SizeType>;

        ECS() = default;
        ~ECS() = default;

        ECS(const ECS&) = delete;
        ECS(ECS&&) noexcept = delete;

        ECS& operator=(const ECS&) = delete;
        ECS& operator=(ECS&&) noexcept = delete;

        [[nodiscard]] inline EntityType CreateBlankEntity()
        {
            if (FreeList.empty())
            {
                SizeType size = Entities.size();

                Entities.emplace_back(size, 0);
                EntityMasks.emplace_back();

                return Entities.back();
            }
            else
            {
                SizeType index = FreeList.back();
                EntityType& entity = Entities[index];
                BitsetType& bitset = EntityMasks[index];

                entity.GetGeneration()++;
                FreeList.pop_back();
                bitset.reset();

                return entity;
            }
        }

        [[nodiscard]] inline std::vector<EntityType> CreateBlankEntities(SizeType count)
        {
            std::vector<EntityType> entities;

            entities.reserve(count);

            for (SizeType i = 0; i < count; i++)
            {
                EntityType entity = CreateBlankEntity();

                entities.push_back(entity);
            }

            return entities;
        }

        template <typename... TQueried>
        requires((DescriptorType::template Contains<TQueried> && ...) && (sizeof...(TQueried) != 0))
        [[nodiscard]] inline ValueResult<EntityType> CreateEntity(TQueried&&... components)
        {
            using ResultType1 = ReferenceResult<ArchetypeType>;
            using ResultType2 = ReferenceResult<EntityType>;

            EntityType entity = CreateBlankEntity();
            SizeType& id = entity.GetID();
            BitsetType mask = MakeBitmask<TQueried...>();

            EntityMasks[id] = mask;

            ResultType1 result1 = Archetypes.Insert(mask);

            if (result1.Failed())
            {
                return ValueResult<EntityType>(entity, false);
            }

            ArchetypeType& archetype = result1.GetValue();
            ResultType2 result2 = archetype.Insert(id, entity);

            if (result2.Failed())
            {
                return ValueResult<EntityType>(entity, false);
            }

            bool result3 = (AddEntityToSparseSet<TQueried>(entity, components) && ...);

            return ValueResult<EntityType>(entity, result3);
        }

        template <typename... TQueried>
        requires((DescriptorType::template Contains<TQueried> && ...) && sizeof...(TQueried) != 0)
        inline std::vector<ValueResult<EntityType>> CreateEntities(SizeType count, TQueried&&... components)
        {
            std::vector<ValueResult<EntityType>> entities;

            entities.reserve(count);

            for (SizeType i = 0; i < count; i++)
            {
                entities.push_back(CreateEntity(std::forward<TQueried>(components)...));
            }

            return entities;
        }

        [[nodiscard]] inline bool DestroyEntity(EntityType entity)
        {
            if (HasEntity(entity))
            {
                SizeType& id = entity.GetID();
                SizeType& generation = entity.GetGeneration();

                const BitsetType& bitset = EntityMasks[id];
                ReferenceResult<ArchetypeType> archetypeResult = Archetypes.Get(bitset);

                if (!archetypeResult.Failed())
                {
                    RemoveEntityFromSparseSets(entity, bitset);

                    auto& archetype = archetypeResult.GetValue();
                    auto& entities = archetype->GetEntities();
                    auto& dense = entities.GetDense();

                    if (!archetype->Remove(id))
                    {
                        return false;
                    }

                    if (dense.size() == 0)
                    {
                        Archetypes.Remove(bitset);
                    }
                }

                FreeList.push_back(id);
                Entities[id] = {std::numeric_limits<SizeType>::max(), generation};
                EntityMasks[id].reset();
            }
            else
            {
                return false;
            }

            return true;
        }

        [[nodiscard]] inline bool DestroyEntities(const std::vector<EntityType>& entities)
        {
            bool result = true;

            for (const auto& entity : entities)
            {
                result &= DestroyEntity(entity);
            }

            return result;
        }

        [[nodiscard]] inline bool HasEntity(EntityType entity) const
        {
            SizeType& id = entity.GetID();
            SizeType& generation = entity.GetGeneration();

            return id < Entities.size() && Entities[id].GetGeneration() == generation && Entities[id].GetID() != std::numeric_limits<SizeType>::max();
        }

        [[nodiscard]] inline bool HasEntities(const std::vector<EntityType>& entities) const
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
        requires(DescriptorType::template Contains<U>)
        [[nodiscard]] inline bool EntityHasComponent(EntityType entity) const
        {
            SizeType& id = entity.GetID();

            if (HasEntity(entity))
            {
                constexpr SizeType index = DescriptorType::template Index<U>();

                return EntityMasks[id].test(index);
            }

            return false;
        }

        template <typename... TQueried>
        requires((DescriptorType::template Contains<TQueried> && ...) && sizeof...(TQueried) != 0)
        [[nodiscard]] inline bool EntityHasComponents(EntityType entity) const
        {
            return (EntityHasComponent<TQueried>(entity) && ...);
        }

        template <typename TComponent>
        requires(DescriptorType::template Contains<TComponent>)
        [[nodiscard]] inline bool EntitiesHaveComponent(const std::vector<EntityType>& entities) const
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
        requires((DescriptorType::template Contains<TQueried> && ...) && sizeof...(TQueried) != 0)
        [[nodiscard]] inline bool EntitiesHaveComponents(const std::vector<EntityType>& entities) const
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
        requires(DescriptorType::template Contains<TComponent>)
        [[nodiscard]] inline bool AddComponentToEntity(EntityType entity, TComponent&& component)
        {
            if (HasEntity(entity) && !EntityHasComponent<TComponent>(entity))
            {
                SizeType& id = entity.GetID();

                auto& set = std::get<SparseSet<TComponent, SizeType>>(SparseSets);

                constexpr SizeType index = DescriptorType::template Index<TComponent>();

                BitsetType& targetBitset = EntityMasks[id];
                BitsetType oldBitset = EntityMasks[id];

                targetBitset.set(index);

                bool archetypeResult = UpdateArchetype(entity, oldBitset, targetBitset);

                if (archetypeResult)
                {
                    auto result = set.Insert(id, std::forward<TComponent>(component));

                    if (result.Failed())
                    {
                        return false;
                    }
                }

                return archetypeResult;
            }

            std::cerr << "Failed to give entity " << entity.GetID() << " component\n";

            return false;
        }

        template <typename TComponent>
        requires(DescriptorType::template Contains<TComponent>)
        [[nodiscard]] inline bool AddComponentToEntities(const std::vector<EntityType>& entities, TComponent&& component)
        {
            bool result = true;

            for (auto& entity : entities)
            {
                result &= AddComponentToEntity(entity, std::forward<TComponent>(component));
            }

            return result;
        }

        template <typename... TQueried>
        requires((DescriptorType::template Contains<TQueried> && ...) && sizeof...(TQueried) != 0)
        [[nodiscard]] inline bool AddComponentsToEntity(EntityType entity, TQueried&&... components)
        {
            return (AddComponentToEntity<TQueried>(entity, std::forward<TQueried>(components)) && ...);
        }

        template <typename... TQueried>
        requires((DescriptorType::template Contains<TQueried> && ...) && sizeof...(TQueried) != 0)
        [[nodiscard]] inline bool AddComponentsToEntities(const std::vector<EntityType>& entities, TQueried&&... components)
        {
            bool result = true;

            for (auto& entity : entities)
            {
                result &= AddComponentsToEntity<TQueried...>(entity, std::forward<TQueried>(components)...);
            }

            return result;
        }

        template <typename TComponent>
        requires(DescriptorType::template Contains<TComponent>)
        [[nodiscard]] inline bool RemoveComponentFromEntity(EntityType entity)
        {
            if (HasEntity(entity))
            {
                auto& set = std::get<SparseSet<TComponent, SizeType>>(SparseSets);

                SizeType& id = entity.GetID();

                constexpr SizeType index = DescriptorType::template Index<TComponent>();

                BitsetType& targetBitset = EntityMasks[id];
                BitsetType oldBitset = EntityMasks[id];

                targetBitset.reset(index);

                bool archetypeResult = UpdateArchetype(entity, oldBitset, targetBitset);

                if (archetypeResult)
                {
                    set.Remove(id);
                }

                return archetypeResult;
            }
            else
            {
                return false;
            }
        }

        template <typename TComponent>
        requires(DescriptorType::template Contains<TComponent>)
        [[nodiscard]] inline bool RemoveComponentFromEntities(const std::vector<EntityType>& entities)
        {
            bool result = true;

            for (auto& entity : entities)
            {
                result &= RemoveComponentFromEntity<TComponent>(entity);
            }

            return result;
        }

        template <typename... TQueried>
        requires((DescriptorType::template Contains<TQueried> && ...) && sizeof...(TQueried) != 0)
        [[nodiscard]] inline bool RemoveComponentsFromEntity(EntityType entity)
        {
            return (RemoveComponentFromEntity<TQueried>(entity) && ...);
        }

        template <typename... TQueried>
        requires((DescriptorType::template Contains<TQueried> && ...) && sizeof...(TQueried) != 0)
        [[nodiscard]] inline bool RemoveComponentsFromEntity(const std::vector<EntityType>& entities)
        {
            bool ReferenceResult = true;

            for (auto& entity : entities)
            {
                ReferenceResult &= RemoveComponentsFromEntity<TQueried...>(entity);
            }

            return ReferenceResult;
        }

        [[nodiscard]] inline BitsetTree<ArchetypeType, SizeType, sizeof...(TComponents)>& GetArchetypess()
        {
            return Archetypes;
        }

        [[nodiscard]] inline const BitsetTree<ArchetypeType, SizeType, sizeof...(TComponents)>& GetArchetypes() const
        {
            return Archetypes;
        }

        [[nodiscard]] ReferenceResult<ArchetypeType> GetArchetype(const BitsetType& mask)
        {
            return Archetypes.Get(mask);
        }

        [[nodiscard]] const ReferenceResult<ArchetypeType> GetArchetype(const BitsetType& mask) const
        {
            return Archetypes.Get(mask);
        }

        template <typename TComponent>
        requires(DescriptorType::template Contains<TComponent>)
        [[nodiscard]] inline constexpr SparseSet<TComponent, SizeType>& GetSparseSet()
        {
            return std::get<SparseSet<TComponent, SizeType>>(SparseSets);
        }

        template <typename TComponent>
        requires(DescriptorType::template Contains<TComponent>)
        [[nodiscard]] inline constexpr const SparseSet<TComponent, SizeType>& GetSparseSet() const
        {
            return std::get<SparseSet<TComponent, SizeType>>(SparseSets);
        }

        template <typename... TQueried>
        requires((DescriptorType::template Contains<TQueried> && ...) && sizeof...(TQueried) != 0)
        [[nodiscard]] static inline constexpr BitsetType MakeBitmask()
        {
            BitsetType bitset;

            (bitset.set(DescriptorType::template Index<TQueried>()), ...);

            return bitset;
        }

        template <typename... TQueried>
        requires(DescriptorType::template Contains<TQueried> && ...)
        [[nodiscard]] inline auto GetEntityView(ArchetypeType& archetype)
        {
            return EntityView<ECS<DescriptorType>, SizeType, TQueried...>(this, archetype);
        }

        template <typename... TQueried>
        requires(DescriptorType::template Contains<TQueried> && ...)
        [[nodiscard]] inline const auto GetEntityView(ArchetypeType& archetype) const
        {
            return EntityView<ECS<DescriptorType>, SizeType, TQueried...>(this, archetype);
        }

    private:
        [[nodiscard]] inline bool UpdateArchetype(EntityType entity, const BitsetType& oldBitset, const BitsetType& newBitset)
        {
            using ResultType = ReferenceResult<ArchetypeType>;

            if (oldBitset == newBitset)
            {
                std::cerr << "Failed to insert entity " << entity.GetID() << " into archetype: same bitset\n";

                return false;
            }

            ResultType result = Archetypes.Get(oldBitset);

            if (!result.Failed())
            {
                ArchetypeType& oldArchetype = result.GetValue();

                if (!oldArchetype.Remove(entity.GetID()))
                {
                    std::cerr << "Failed to remove entity " << entity.GetID() << " from old archetype\n";

                    return false;
                }

                auto& dense = oldArchetype.GetDense();

                if (dense.size() == 0)
                {
                    Archetypes.Remove(oldBitset);
                }
            }

            ResultType insertResult = Archetypes.Insert(newBitset);

            if (insertResult.Failed())
            {
                std::cerr << "Failed to insert new archetype\n";

                return false;
            }

            auto& archetype = insertResult.GetValue();
            auto& id = entity.GetID();
            auto archetypeInsertResult = archetype.Insert(id, entity);

            if (archetypeInsertResult.Failed())
            {
                std::cerr << "Failed to add entity " << entity.GetID() << " to new archetype\n";
            }

            return !archetypeInsertResult.Failed();
        }

        template <std::size_t... Ns>
        inline void RemoveEntityFromSparseSetsImplementation(EntityType entity, const BitsetType& mask, std::index_sequence<Ns...>)
        {
            ((mask.test(Ns) ? std::get<Ns>(SparseSets).Remove(entity.GetID()) : void()), ...);
        }

        inline void RemoveEntityFromSparseSets(EntityType entity, const BitsetType& mask)
        {
            RemoveEntityFromSparseSetsImplementation(entity, mask, std::index_sequence_for<TComponents...>{});
        }

        template <typename T, typename U>
        inline bool AddEntityToSparseSet(EntityType& entity, U&& component)
        {
            using SparseSetType = SparseSet<T, SizeType>;
            using ResultType = ReferenceResult<typename SparseSetType::Type>;

            SparseSetType& sparseSet = std::get<SparseSetType>(SparseSets);
            ResultType result = sparseSet.Insert(entity.GetID(), std::forward<U>(component));

            return !result.Failed();
        }

        std::tuple<SparseSet<TComponents, SizeType>...> SparseSets;

        BitsetTree<ArchetypeType, SizeType, sizeof...(TComponents)> Archetypes;

        std::vector<BitsetType> EntityMasks;
        std::vector<EntityType> Entities;
        std::vector<SizeType> FreeList;
    };
}
