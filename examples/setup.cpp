#include <minecs/minecs.hpp>

using ecs_descriptor = minecs::ecs_descriptor<std::uint32_t, int, float, double>;

using ecs = minecs::ecs<ecs_descriptor>;

int main()
{
    minecs::ecs<ecs_descriptor> a;

    constexpr auto bitmask2 = a.get_bitmask<int>();
}