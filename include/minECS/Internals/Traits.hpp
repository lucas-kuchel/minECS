#pragma once

#include <type_traits>

namespace minECS
{
    template <typename TComponent>
    inline constexpr bool CanBeComponent = !std::is_pointer_v<TComponent> &&
                                           !std::is_reference_v<TComponent> &&
                                           !std::is_const_v<TComponent> &&
                                           !std::is_volatile_v<TComponent> &&
                                           !std::is_array_v<TComponent> &&
                                           !std::is_function_v<TComponent> &&
                                           !std::is_void_v<TComponent> &&
                                           !std::is_same_v<TComponent, std::nullptr_t>;

    template <typename... TComponents>
    inline constexpr bool CanBeComponents = (CanBeComponent<TComponents> && ...);

    template <typename TSizeType>
    inline constexpr bool IsSizeType = std::is_unsigned_v<TSizeType> && !(sizeof(TSizeType) == sizeof(char));

    template <typename...>
    inline constexpr bool ComponentsAreUnique = true;

    template <typename TComponent, typename... TOthers>
    inline constexpr bool ComponentsAreUnique<TComponent, TOthers...> = (!std::is_same_v<TComponent, TOthers> && ...) && ComponentsAreUnique<TOthers...>;

    template <typename TSizeType, typename... TComponents>
    requires CanBeComponents<TComponents...> && IsSizeType<TSizeType> && ComponentsAreUnique<TComponents...>
    class Descriptor;

    template <typename>
    inline constexpr bool IsDescriptor = false;

    template <typename TSizeType, typename... TComponents>
    inline constexpr bool IsDescriptor<Descriptor<TSizeType, TComponents...>> = true;

    template <typename TDescriptor>
    requires IsDescriptor<TDescriptor>
    class ECS;

    template <typename TSizeType, typename... TComponents>
    class ECS<Descriptor<TSizeType, TComponents...>>;

    template <typename>
    inline constexpr bool IsECS = false;

    template <typename TDescriptor>
    requires IsDescriptor<TDescriptor>
    inline constexpr bool IsECS<ECS<TDescriptor>> = true;

    template <typename T, typename TSizeType, TSizeType NBitsetSize>
    requires IsSizeType<TSizeType> && (NBitsetSize > 0)
    class BitsetTree;

    template <typename>
    inline constexpr bool IsBitsetTree = false;

    template <typename T, typename TSizeType, TSizeType NBitsetSize>
    requires IsSizeType<TSizeType> && (NBitsetSize > 0)
    inline constexpr bool IsBitsetTree<BitsetTree<T, TSizeType, NBitsetSize>> = true;

    template <typename, typename TSizeType>
    requires IsSizeType<TSizeType>
    class SparseSet;

    template <typename>
    inline constexpr bool IsSparseSet = false;

    template <typename T, typename TSizeType>
    requires IsSizeType<TSizeType>
    inline constexpr bool IsSparseSet<SparseSet<T, TSizeType>> = true;
}