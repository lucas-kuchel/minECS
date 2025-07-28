#pragma once

#include <minECS/Internals/Traits.hpp>

#include <array>
#include <limits>

namespace minECS
{
    template <typename TSizeType, typename... TComponents>
    requires ComponentsAreUnique<TComponents...> && CanBeComponents<TComponents...> && IsSizeType<TSizeType>
    class ECSDescriptor
    {
    public:
        using SizeType = TSizeType;

        ECSDescriptor() = delete;

        template <typename TComponent>
        static constexpr bool Contains = (std::is_same_v<TComponent, TComponents> || ...);

        template <typename TComponent>
        requires Contains<TComponent>
        static constexpr std::size_t Index()
        {
            constexpr std::array<bool, sizeof...(TComponents)> matchFlags = {std::is_same_v<TComponent, TComponents>...};

            for (std::size_t i = 0; i < matchFlags.size(); i++)
            {
                if (matchFlags[i])
                {
                    return i;
                }
            }

            return std::numeric_limits<SizeType>::max();
        }
    };
}