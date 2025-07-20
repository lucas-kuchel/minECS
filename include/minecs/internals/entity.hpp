#pragma once

#include <cstdint>

namespace minecs
{
    struct entity
    {
        std::uint32_t id;
        std::uint32_t generation;
    };
}