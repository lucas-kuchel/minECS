#pragma once

#include <minecs/internals/traits.hpp>

#include <array>
#include <limits>
#include <type_traits>

namespace minecs
{
    template <typename T, typename... Args>
    requires((is_compatible_component_v<Args> && ...) &&
             !has_duplicate_types_v<Args...> &&
             std::is_unsigned_v<T> && (sizeof...(Args) > 0))
    class descriptor
    {
    public:
        using size_type = T;

        descriptor() = delete;

        template <typename U>
        static constexpr bool contains = (std::is_same_v<U, Args> || ...);

        template <typename U>
        requires contains<U>
        static constexpr std::size_t index()
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
}