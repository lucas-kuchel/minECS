#pragma once

#include <minECS/Internals/Traits.hpp>

#include <bitset>
#include <cstdint>
#include <optional>
#include <vector>

namespace minECS
{
    template <typename T, typename TSizeType, TSizeType NBitsetSize>
    requires IsSizeType<TSizeType> && (NBitsetSize != 0)
    class BitsetTree
    {
    public:
        using Type = T;
        using SizeType = TSizeType;

        static constexpr SizeType BitsetSize = NBitsetSize;
        static constexpr SizeType BlockSize = 256;

        static constexpr SizeType RoundedSize = (BitsetSize + 7) / 8 * 8;
        static constexpr SizeType LevelCount = RoundedSize / 8;

        using iterator = std::vector<std::pair<std::bitset<BitsetSize>, Type>>::iterator;
        using const_iterator = std::vector<std::pair<std::bitset<BitsetSize>, Type>>::const_iterator;

        BitsetTree()
        {
            Root = Pool.Allocate();
            *Root = {};
        }

        ~BitsetTree() = default;

        BitsetTree(const BitsetTree& other)
            : Contiguous(other.Contiguous)
        {
            Root = CloneSubtree(other.Root, Pool);
        }

        BitsetTree(BitsetTree&& other) noexcept
            : Root(other.Root), Pool(std::move(other.Pool)), Contiguous(std::move(other.Contiguous))
        {
            other.Root = nullptr;
        }

        BitsetTree& operator=(const BitsetTree& other)
        {
            if (this == &other)
            {
                return *this;
            }

            BitsetTree temp(other);

            std::swap(Root, temp.Root);
            std::swap(Pool, temp.Pool);
            std::swap(Contiguous, temp.Contiguous);

            return *this;
        }

        BitsetTree& operator=(BitsetTree&& other) noexcept
        {
            if (this == &other)
            {
                return *this;
            }

            ClearTree(Root);

            Root = other.Root;
            Pool = std::move(other.Pool);
            Contiguous = std::move(other.Contiguous);

            other.Root = nullptr;

            return *this;
        }

        inline void Remove(const std::bitset<BitsetSize>& bitset)
        {
            RemoveRecursive(Root, bitset, 0);
        }

        Type& GetOrInsert(const std::bitset<BitsetSize>& bitset)
        {
            Node* current = Root;

            for (SizeType level = 0; level < LevelCount; level++)
            {
                std::uint8_t key = GetByte(bitset, level);

                Node*& next = current->Children[key];
                if (!next)
                {
                    next = Pool.Allocate();
                    *next = {};
                }

                current = next;
            }

            if (!current->ArchetypeIndex.has_value())
            {
                current->ArchetypeIndex = Contiguous.size();
                Contiguous.emplace_back(bitset, T{});
            }

            return Contiguous[current->ArchetypeIndex.value()].second;
        }

        [[nodiscard]] Type* Get(const std::bitset<BitsetSize>& bitset)
        {
            Node* current = Root;

            for (SizeType level = 0; level < LevelCount; level++)
            {
                std::uint8_t key = GetByte(bitset, level);

                Node*& next = current->Children[key];

                if (!next)
                {
                    return nullptr;
                }

                current = next;
            }

            if (!current->ArchetypeIndex.has_value())
            {
                return nullptr;
            }

            return &Contiguous[current->ArchetypeIndex.value()].second;
        }

        [[nodiscard]] const Type* Get(const std::bitset<BitsetSize>& bitset) const
        {
            Node* current = Root;

            for (SizeType level = 0; level < LevelCount; level++)
            {
                std::uint8_t key = GetByte(bitset, level);

                Node*& next = current->Children[key];

                if (!next)
                {
                    return nullptr;
                }

                current = next;
            }

            if (!current->ArchetypeIndex.has_value())
            {
                return nullptr;
            }

            return &Contiguous[current->ArchetypeIndex.value()].second;
        }

        [[nodiscard]] iterator begin()
        {
            return Contiguous.begin();
        }

        [[nodiscard]] iterator end()
        {
            return Contiguous.end();
        }

        [[nodiscard]] const_iterator begin() const
        {
            return Contiguous.begin();
        }

        [[nodiscard]] const_iterator end() const
        {
            return Contiguous.end();
        }

    private:
        struct Node
        {
            std::array<Node*, 256> Children{};
            std::optional<SizeType> ArchetypeIndex;
        };

        class NodePool
        {
        public:
            NodePool()
                : Index(BlockSize)
            {
            }

            ~NodePool()
            {
                for (auto* block : Blocks)
                {
                    delete[] block;
                }
            }

            NodePool(const NodePool&) = default;
            NodePool(NodePool&&) noexcept = default;

            NodePool& operator=(const NodePool&) = default;
            NodePool& operator=(NodePool&&) noexcept = default;

            [[nodiscard]] Node* Allocate()
            {
                if (!FreeList.empty())
                {
                    Node* node = FreeList.back();

                    FreeList.pop_back();

                    return node;
                }

                if (Index >= BlockSize)
                {
                    AllocateBlock();
                }

                return &Blocks.back()[Index++];
            }

            void Deallocate(Node* ptr)
            {
                FreeList.push_back(ptr);
            }

        private:
            void AllocateBlock()
            {
                Blocks.push_back(new Node[BlockSize]);

                Index = 0;
            }

            std::vector<Node*> Blocks;
            std::vector<Node*> FreeList;

            SizeType Index;
        };

        void ClearTree(Node*& current)
        {
            if (!current)
            {
                return;
            }

            for (auto*& child : current->Children)
            {
                if (child)
                {
                    ClearTree(child);

                    Pool.Deallocate(child);

                    child = nullptr;
                }
            }

            Pool.Deallocate(current);

            current = nullptr;
        }

        [[nodiscard]] static std::uint8_t GetByte(const std::bitset<BitsetSize>& bs, SizeType byteIndex)
        {
            std::uint8_t result = 0;

            for (SizeType i = 0; i < 8; i++)
            {
                SizeType bitIndex = byteIndex * 8 + i;

                if (bitIndex < BitsetSize && bs[bitIndex])
                {
                    result |= (1 << i);
                }
            }

            return result;
        }

        bool RemoveRecursive(Node*& current, const std::bitset<BitsetSize>& bitset, SizeType level)
        {
            if (!current)
            {
                return false;
            }

            if (level == LevelCount)
            {
                current->ArchetypeIndex.reset();
            }
            else
            {
                std::uint8_t key = GetByte(bitset, level);
                Node*& next = current->Children[key];

                if (RemoveRecursive(next, bitset, level + 1))
                {
                    Pool.Deallocate(next);
                    next = nullptr;
                }
            }

            if (current->ArchetypeIndex.has_value())
            {
                return false;
            }

            for (auto* child : current->Children)
            {
                if (child)
                {
                    return false;
                }
            }

            return true;
        }

        [[nodiscard]] Node* CloneSubtree(const Node* source, NodePool& pool)
        {
            if (!source)
            {
                return nullptr;
            }

            Node* clone = pool.Allocate();
            clone->ArchetypeIndex = source->ArchetypeIndex;

            for (SizeType i = 0; i < 256; i++)
            {
                if (source->Children[i])
                {
                    clone->Children[i] = CloneSubtree(source->Children[i], pool);
                }
            }

            return clone;
        }

        Node* Root;
        NodePool Pool;

        std::vector<std::pair<std::bitset<BitsetSize>, Type>> Contiguous;
    };
}