#pragma once

#include <bitset>
#include <optional>
#include <vector>

namespace minecs
{
    template <typename T, typename U, std::size_t N>
    requires std::is_arithmetic_v<U>
    class bitset_tree
    {
    public:
        using type = T;
        using size_type = U;

        static constexpr auto size = N;
        static constexpr size_type block_size = 1024;

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

        bitset_tree&
        operator=(const bitset_tree& other)
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

            remove(std::bitset<size>());

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

        [[nodiscard]] type& get_or_insert(const std::bitset<size>& bitset)
        {
            node* node = m_root;

            for (std::size_t i = 0; i < size; i++)
            {
                struct node*& next = bitset[i] ? node->one : node->zero;

                if (!next)
                {
                    next = m_pool.allocate();
                    *next = {};
                }

                node = next;
            }

            if (!node->archetype_index.has_value())
            {
                node->archetype_index = m_contiguous.size();
                m_contiguous.push_back(std::make_pair(bitset, T()));
            }

            return m_contiguous[node->archetype_index.value()].second;
        }

        [[nodiscard]] type* get(const std::bitset<size>& bitset)
        {
            node* node = m_root;

            for (std::size_t i = 0; i < size; i++)
            {
                node = bitset[i] ? node->one : node->zero;

                if (!node)
                {
                    return nullptr;
                }
            }

            if (!node->archetype_index.has_value())
            {
                return nullptr;
            }

            return &m_contiguous[node->archetype_index.value()].second;
        }

        [[nodiscard]] const type* get(const std::bitset<size>& bitset) const
        {
            node* node = m_root;

            for (std::size_t i = 0; i < size; i++)
            {
                node = bitset[i] ? node->one : node->zero;

                if (!node)
                {
                    return nullptr;
                }
            }

            if (!node->archetype_index.has_value())
            {
                return nullptr;
            }

            return &m_contiguous[node->archetype_index.value()].second;
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
            node* zero = nullptr;
            node* one = nullptr;

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

        bool remove_recursive(node*& node, const std::bitset<size>& bitset, size_type depth)
        {
            if (!node)
            {
                return false;
            }

            if (depth == size)
            {
                node->archetype_index.reset();

                return node->zero == nullptr && node->one == nullptr;
            }

            struct node*& next = bitset[depth] ? node->one : node->zero;

            if (remove_recursive(next, bitset, depth + 1))
            {
                m_pool.deallocate(next);

                next = nullptr;
            }

            return node->archetype_index == std::nullopt && node->zero == nullptr && node->one == nullptr;
        }

        [[nodiscard]] node* clone_subtree(const node* source, node_pool& pool)
        {
            if (!source)
            {
                return nullptr;
            }

            node* clone = pool.allocate();
            *clone = *source;

            clone->zero = clone_subtree(source->zero, pool);
            clone->one = clone_subtree(source->one, pool);

            return clone;
        }

        node* m_root;
        node_pool m_pool;

        std::vector<std::pair<std::bitset<size>, type>> m_contiguous;
    };

    template <typename>
    struct is_bitset_tree : std::false_type
    {
    };

    template <typename T, typename U, std::size_t N>
    struct is_bitset_tree<bitset_tree<T, U, N>> : std::true_type
    {
    };

    template <typename T>
    inline constexpr bool is_bitset_tree_v = is_bitset_tree<T>::value;
}