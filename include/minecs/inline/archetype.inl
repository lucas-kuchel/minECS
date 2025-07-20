#pragma once

#include <minecs/internals/archetype.hpp>

namespace minecs
{
    bool archetype::insert(std::uint32_t id, entity entity)
    {
        return m_entities.insert(id, entity);
    }

    bool archetype::remove(std::uint32_t id)
    {
        return m_entities.remove(id);
    }

    sparse_set<entity>& archetype::get_entities()
    {
        return m_entities;
    }
}