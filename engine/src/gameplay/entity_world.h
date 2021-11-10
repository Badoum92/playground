#pragma once
#include <exo/collections/set.h>

#include "gameplay/system.h"
#include "gameplay/system_registry.h"

struct Entity;

struct EntityWorld
{
public:
    void update(double delta_t);

    Entity* create_entity();
    void destroy_entity(Entity *entity);

    template<std::derived_from<GlobalSystem> System, typename ...Args>
    void create_system(Args &&...args)
    {
        System *new_system = new System(std::forward<Args>(args)...);
        create_system_internal(static_cast<GlobalSystem*>(new_system));
    }

    const SystemRegistry &get_system_registry() const { return system_registry; }
    SystemRegistry &get_system_registry() { return system_registry; }
private:
    void create_system_internal(GlobalSystem *system);
    void destroy_system_internal(GlobalSystem *system);

    Set<Entity*> entities;
    SystemRegistry system_registry;
};
