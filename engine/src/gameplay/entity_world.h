#pragma once
#include <exo/collections/set.h>

#include "gameplay/system.h"
#include "gameplay/system_registry.h"

#include <string_view>

struct Entity;

struct EntityWorld
{
public:
    void update(double delta_t);

    Entity* create_entity(std::string_view name = "Unnamed");
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

    void display_entity_tree_rec(Entity *entity, Entity* &selected);
    void display_ui();

    Set<Entity*> entities;
    SystemRegistry system_registry;
};
