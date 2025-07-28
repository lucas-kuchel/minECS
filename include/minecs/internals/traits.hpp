#pragma once

#include <type_traits>

namespace minecs
{
    template <typename T>
    inline constexpr bool is_compatible_component_v = !std::is_pointer_v<T> &&
                                                      !std::is_reference_v<T> &&
                                                      !std::is_const_v<T> &&
                                                      !std::is_volatile_v<T> &&
                                                      !std::is_array_v<T> &&
                                                      !std::is_function_v<T> &&
                                                      !std::is_void_v<T> &&
                                                      !std::is_same_v<T, std::nullptr_t>;

    template <typename...>
    inline constexpr bool has_duplicate_types_v = false;

    template <typename First, typename... Rest>
    inline constexpr bool has_duplicate_types_v<First, Rest...> = (std::is_same_v<First, Rest> || ...) || has_duplicate_types_v<Rest...>;

    template <typename T, typename... Args>
    requires((is_compatible_component_v<Args> && ...) &&
             !has_duplicate_types_v<Args...> &&
             std::is_unsigned_v<T> && (sizeof...(Args) > 0))
    class descriptor;

    template <typename>
    struct is_descriptor : std::false_type
    {
    };

    template <typename T, typename... Args>
    struct is_descriptor<descriptor<T, Args...>> : std::true_type
    {
    };

    template <typename T>
    inline constexpr bool is_descriptor_v = is_descriptor<T>::value;

    template <typename T>
    requires is_descriptor_v<T>
    class ecs;

    template <typename T, typename... Args>
    requires is_descriptor_v<descriptor<T, Args...>>
    class ecs<descriptor<T, Args...>>;

    template <typename>
    struct is_ecs : std::false_type
    {
    };

    template <typename T, typename... Args>
    struct is_ecs<ecs<descriptor<T, Args...>>> : std::true_type
    {
    };

    template <typename T>
    inline constexpr bool is_ecs_v = is_ecs<T>::value;

    template <typename T, typename U, std::size_t N>
    requires std::is_unsigned_v<U> && (N > 0)
    class bitset_tree;

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

    template <typename T, typename U>
    requires std::is_unsigned_v<U>
    class sparse_set;

    template <typename>
    struct is_sparse_set : std::false_type
    {
    };

    template <typename T, typename U>
    struct is_sparse_set<sparse_set<T, U>> : std::true_type
    {
    };

    template <typename T>
    inline constexpr bool is_sparse_set_v = is_sparse_set<T>::value;
}