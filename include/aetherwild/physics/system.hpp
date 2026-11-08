#pragma once
#include "aetherwild/domain.hpp"
namespace aw {
    class MovementSystem {
        public: static bool safe(Vec3,const Id& scene,const Catalog&);
        static double landingDamage(double drop,double maxHp,double airTime);
        static double land(Player&,double drop,double airTime);
        static void move(Player&,Vec3 destination,double dt,bool sprint,const Catalog&,double now);
        static void tickStamina(Player&,double dt,bool sprint,double now);
    };
}
