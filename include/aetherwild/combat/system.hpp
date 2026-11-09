#pragma once
#include "aetherwild/domain.hpp"
namespace aw {
    class RewardSystem {
        public: static bool claim(World&,Player&,const Id&,const Catalog&);
        static void onDeath(World&,Monster&,const Catalog&);
    };
    class CombatSystem {
        public: static Monster spawn(World&,const Id&,Vec3,int level,const Id& region,bool boss=false,const Id& definition="");
        static AttackResult attack(World&,const Catalog&,const Id& player,const Id& monster,const Id& arrow="woodarrow",double bowPower=1);
        static double monsterAttack(World&,const Catalog&,const Id& monster,const Id& player);
        static void cast(World&,const Catalog&,const Id& player,const Id& skill,const Id& target="");
        static void weaponSkill(World&,const Catalog&,const Id& player,const Id& target="");
        static void respawn(Player&,const Catalog&);
    };
}
