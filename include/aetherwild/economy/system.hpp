#pragma once
#include "aetherwild/domain.hpp"
namespace aw {
    class EconomySystem {
        public: static void buy(World&,Player&,const Id& code,Quantity count,const Id& npc,const Catalog&);
        static void transfer(Player& from,Player& to,Quantity coins);
    };
}
