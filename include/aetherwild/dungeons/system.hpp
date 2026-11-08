#pragma once
#include "aetherwild/domain.hpp"
namespace aw {
    class DungeonSystem {
        public: static Id enter(World&,Player&,const Id& dungeon,const Catalog&);
        static void interactObjective(World&,Player&,const Id& objective,const Catalog&);
        static void openChest(World&,Player&,const Catalog&);
        static void leave(World&,Player&);
    };
}
