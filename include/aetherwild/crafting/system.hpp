#pragma once
#include "aetherwild/domain.hpp"
namespace aw {
    class CraftingSystem {
        public: static Id requiredRegion(const Recipe&,const Player&,const Catalog&);
        static void craft(World&,Player&,const Id& recipe,const Id& npc,const Catalog&);
    };
    struct EnhancementQuote { Quantity coins; Materials materials; double chance; int nextLevel; };
    class EnhancementSystem {
        public: static EnhancementQuote quote(int level);
        static bool enhance(World&,Player&,const Id& item,const Id& npc,int expectedLevel,const Catalog&,double roll);
    };
}
