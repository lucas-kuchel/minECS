#pragma once

#include <array>
#include <limits>
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
    requires((is_compatible_component_v<Args> && ...) && !has_duplicate_types_v<Args...> && std::is_integral_v<T>)
    class ecs_descriptor
    {
    public:
        using size_type = T;

        ecs_descriptor() = delete;

        template <typename U>
        static constexpr bool contains = (std::is_same_v<U, Args> || ...);

        template <typename U>
        requires contains<U>
        static consteval std::size_t index()
        {
            constexpr std::array<bool, sizeof...(Args)> match_flags = {std::is_same_v<U, Args>...};

            for (std::size_t i = 0; i < match_flags.size(); i++)
            {
                if (match_flags[i])
                {
                    return i;
                }
            }

            return std::numeric_limits<std::size_t>::max();
        }
    };

    template <typename>
    struct is_ecs_descriptor : std::false_type
    {
    };

    template <typename T, typename... Args>
    struct is_ecs_descriptor<ecs_descriptor<T, Args...>> : std::true_type
    {
    };

    template <typename T>
    inline constexpr bool is_ecs_descriptor_v = is_ecs_descriptor<T>::value;
}