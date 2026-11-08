#pragma once
#include "aetherwild/runtime.hpp"
namespace aw::runtime_detail {
inline constexpr double tau=6.283185307179586;
        inline Json statsJson(Stats s) {
            return Json {
                {
                    "attack",s.attack
                },
                {
                    "hp",s.hp
                },
                {
                    "mp",s.mp
                },
                {
                    "defense",s.defense
                },
                {
                    "crit",s.crit
                },
                {
                    "speed",s.speed
                }
            };
        }
        inline Json costJson(const Materials&m) {
            Json j=Json::object();
            for(auto&[id,n]:m)j[id]=n;
            return j;
        }
        inline std::string roleName(Role r) {
            return r==Role::Owner?"owner":r==Role::Admin?"admin":"player";
        }
        inline std::string kindName(WeaponKind k) {
            return weaponName(k);
        }
}
