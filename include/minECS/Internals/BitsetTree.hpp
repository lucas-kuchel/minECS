#pragma once

#include <bitset>
#include <cstdint>
#include <optional>
#include <vector>

namespace minECS
{
    template <typename Type, typename SizeType, std::size_t N>
    requires std::is_unsigned_v<SizeType> && (N > 0)
    class bitset_tree
    {
    public:
        using type = Type;
        using size_type = SizeType;

        static constexpr auto size = N;
        static constexpr size_type block_size = 256;

        static constexpr size_type rounded_size = (N + 7) / 8 * 8;
        static constexpr size_type num_levels = rounded_size / 8;

        using iterator = std::vector<std::pair<std::bitset<size>, type>>::iterator;
        using const_iterator = std::vector<std::pair<std::bitset<size>, type>>::const_iterator;

        bitset_tree()
        {
            m_root = m_pool.allocate();
            *m_root = {};
        }

        ~bitset_tree() = default;

        bitset_tree(const bitset_tree& other)
            : m_contiguous(other.m_contiguous)
        {
            m_root = clone_subtree(other.m_root, m_pool);
        }

        bitset_tree(bitset_tree&& other) noexcept
            : m_root(other.m_root), m_pool(std::move(other.m_pool)), m_contiguous(std::move(other.m_contiguous))
        {
            other.m_root = nullptr;
        }

        bitset_tree& operator=(const bitset_tree& other)
        {
            if (this == &other)
            {
                return *this;
            }

            bitset_tree temp(other);

            std::swap(m_root, temp.m_root);
            std::swap(m_pool, temp.m_pool);
            std::swap(m_contiguous, temp.m_contiguous);

            return *this;
        }

        bitset_tree& operator=(bitset_tree&& other) noexcept
        {
            if (this == &other)
            {
                return *this;
            }

            clear_tree(m_root);

            m_root = other.m_root;
            m_pool = std::move(other.m_pool);
            m_contiguous = std::move(other.m_contiguous);

            other.m_root = nullptr;

            return *this;
        }

        inline void remove(const std::bitset<size>& bitset)
        {
            remove_recursive(m_root, bitset, 0);
        }

        type& get_or_insert(const std::bitset<size>& bitset)
        {
            node* current = m_root;

            for (size_type level = 0; level < num_levels; level++)
            {
                std::uint8_t key = get_byte(bitset, level);

                node*& next = current->children[key];
                if (!next)
                {
                    next = m_pool.allocate();
                    *next = {};
                }

                current = next;
            }

            if (!current->archetype_index.has_value())
            {
                current->archetype_index = m_contiguous.size();
                m_contiguous.emplace_back(bitset, T{});
            }

            return m_contiguous[current->archetype_index.value()].second;
        }

        [[nodiscard]] type* get(const std::bitset<size>& bitset)
        {
            node* current = m_root;

            for (size_type level = 0; level < num_levels; level++)
            {
                std::uint8_t key = get_byte(bitset, level);

                node*& next = current->children[key];

                if (!next)
                {
                    return nullptr;
                }

                current = next;
            }

            if (!current->archetype_index.has_value())
            {
                return nullptr;
            }

            return &m_contiguous[current->archetype_index.value()].second;
        }

        [[nodiscard]] const type* get(const std::bitset<size>& bitset) const
        {
            node* current = m_root;

            for (size_type level = 0; level < num_levels; level++)
            {
                std::uint8_t key = get_byte(bitset, level);

                node*& next = current->children[key];

                if (!next)
                {
                    return nullptr;
                }

                current = next;
            }

            if (!current->archetype_index.has_value())
            {
                return nullptr;
            }

            return &m_contiguous[current->archetype_index.value()].second;
        }

        [[nodiscard]] iterator begin()
        {
            return m_contiguous.begin();
        }

        [[nodiscard]] iterator end()
        {
            return m_contiguous.end();
        }

        [[nodiscard]] const_iterator begin() const
        {
            return m_contiguous.begin();
        }

        [[nodiscard]] const_iterator end() const
        {
            return m_contiguous.end();
        }

    private:
        struct node
        {
            std::array<node*, 256> children{};
            std::optional<size_type> archetype_index;
        };

        class node_pool
        {
        public:
            node_pool()
                : m_index(block_size)
            {
            }

            ~node_pool()
            {
                for (auto* block : m_blocks)
                {
                    delete[] block;
                }
            }

            node_pool(const node_pool&) = default;
            node_pool(node_pool&&) noexcept = default;

            node_pool& operator=(const node_pool&) = default;
            node_pool& operator=(node_pool&&) noexcept = default;

            [[nodiscard]] node* allocate()
            {
                if (!m_free_list.empty())
                {
                    node* node = m_free_list.back();

                    m_free_list.pop_back();

                    return node;
                }

                if (m_index >= block_size)
                {
                    allocate_block();
                }

                return &m_blocks.back()[m_index++];
            }

            void deallocate(node* ptr)
            {
                m_free_list.push_back(ptr);
            }

        private:
            void allocate_block()
            {
                m_blocks.push_back(new node[block_size]);

                m_index = 0;
            }

            std::vector<node*> m_blocks;
            std::vector<node*> m_free_list;

            size_type m_index;
        };

        void clear_tree(node*& current)
        {
            if (!current)
            {
                return;
            }

            for (auto*& child : current->children)
            {
                if (child)
                {
                    clear_tree(child);

                    m_pool.deallocate(child);

                    child = nullptr;
                }
            }

            m_pool.deallocate(current);

            current = nullptr;
        }

        [[nodiscard]] static std::uint8_t get_byte(const std::bitset<size>& bs, size_type byte_index)
        {
            std::uint8_t result = 0;

            for (size_type i = 0; i < 8; i++)
            {
                size_type bit_index = byte_index * 8 + i;

                if (bit_index < size && bs[bit_index])
                {
                    result |= (1 << i);
                }
            }

            return result;
        }

        bool remove_recursive(node*& current, const std::bitset<size>& bitset, size_type level)
        {
            if (!current)
            {
                return false;
            }

            if (level == num_levels)
            {
                current->archetype_index.reset();
            }
            else
            {
                std::uint8_t key = get_byte(bitset, level);
                node*& next = current->children[key];

                if (remove_recursive(next, bitset, level + 1))
                {
                    m_pool.deallocate(next);
                    next = nullptr;
                }
            }

            if (current->archetype_index.has_value())
            {
                return false;
            }

            for (auto* child : current->children)
            {
                if (child)
                {
                    return false;
                }
            }

            return true;
        }

        [[nodiscard]] node* clone_subtree(const node* source, node_pool& pool)
        {
            if (!source)
            {
                return nullptr;
            }

            node* clone = pool.allocate();
            clone->archetype_index = source->archetype_index;

            for (size_type i = 0; i < 256; i++)
            {
                if (source->children[i])
                {
                    clone->children[i] = clone_subtree(source->children[i], pool);
                }
            }

            return clone;
        }

        node* m_root;
        node_pool m_pool;

        std::vector<std::pair<std::bitset<size>, type>> m_contiguous;
    };
}