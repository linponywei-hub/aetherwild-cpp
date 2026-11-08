#pragma once
#include "aetherwild/domain.hpp"
namespace aw {
    class ProgressionSystem {
        public: static Quantity levelXp(int level);
        static void grantXp(Player&,Quantity);
        static void refresh(Player&,const Catalog&);
        static void changeClass(Player&,const Id&,const Id& npc,const Catalog&);
    };
}
