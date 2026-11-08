#pragma once
#include "aetherwild/domain.hpp"
namespace aw {
    class InventorySystem {
        public: static bool canPay(const Materials&,const Materials&);
        static void pay(Materials&,const Materials&);
        static void add(Materials&,const Id&,Quantity);
        static ItemInstance create(World&,const Catalog&,const Id&,Rarity=Rarity::Common,int required=0);
        static Stats bonuses(const Player&,const Catalog&);
        static Stats itemStats(const ItemInstance&,const Catalog&);
        static void equip(Player&,const Id&,const Catalog&);
        static void unequip(Player&,const Id& instance,const Catalog&);
        static void usePotion(Player&,const Id&,const Catalog&);
        static void storeItem(Player&,const Id& instance,const Id& npc,const Catalog&,bool withdraw=false);
        static void store(Player&,const Id&,Quantity,const Id& npc,const Catalog&,bool withdraw=false);
    };
}
