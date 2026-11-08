#include "aetherwild/talents.hpp"
#include "aetherwild/runtime.hpp"
#include <fstream>
#include <random>
#include <cstdlib>
#include <chrono>
#include "support.hpp"
namespace aw {
using namespace runtime_detail;
    void GameRuntime::populate() {
        for(auto&[id,r]:catalog_.regions) {
            for(int i=0;i<5;i++) {
                double a=i*tau/5;
                Vec3 p {r.center.x+std::cos(a)*(r.radius+27+i*4),0,r.center.z+std::sin(a)*(r.radius+27+i*4)};
                auto mid=world_.nextId("wild");
                CombatSystem::spawn(world_,mid,p,id=="windchime"?1:r.level,id);
                homes_[mid]=p;
            }
            int index=0;
            for(auto&[code,d]:catalog_.resources) {
                double a=index++*2.399963;
                double radius=r.radius+12+(index%4)*6;
                Vec3 p {r.center.x+std::cos(a)*radius,0,r.center.z+std::sin(a)*radius};
                ResourceNode n;
                n.id="resource_"+id+"_"+code;
                n.definition=code;
                n.position=p;
                world_.resources.emplace(n.id,n);
            }
        }
        // Starter gathering trail has separate, real nodes; no automatic wood pickup.
        for(int i=0;i<10;i++) {
            ResourceNode n;
            n.id="starter_"+std::to_string(i);
            n.definition=i<5?"twig":"stone";
            n.position= {80.0+(i%5)*3,0,18.0+(i/5)*8};
            world_.resources.emplace(n.id,n);
        }
        for(auto&[id,b]:catalog_.bosses) {
            auto mid=world_.nextId("raid");
            CombatSystem::spawn(world_,mid,b.position,b.level,b.region,true,id);
            auto&m=world_.monsters.at(mid);
            m.vitals.hp=m.vitals.maxHp=b.hp;
            m.attack=b.attack;
            homes_[mid]=b.position;
        }
    }
}
