#include "aetherwild/talents.hpp"
#include "aetherwild/runtime.hpp"
#include <fstream>
#include <random>
#include <cstdlib>
#include <chrono>
#include "../runtime/support.hpp"
namespace aw {
using namespace runtime_detail;
    void GameRuntime::ai(double dt) {
        std::vector<Id> remove;
        for(auto&[id,m]:world_.monsters) {
            if(!m.vitals.alive()) {
                if(!deadAt_.contains(id))deadAt_[id]=world_.now;
                if(m.scene=="world"&&world_.now-deadAt_[id]>(m.boss?120:45)) {
                    remove.push_back(id);
                }
                continue;
            }
            if(contentState_.flag("expanded")&&m.scene=="world"&&!contentAllowed(m.position))continue;
            if(!features_["combat"]||world_.now<m.frozenUntil){m.castResolve=0;m.castSkill.clear();continue;}
            Player* target=nullptr;
            double best=m.boss?28:20;
            for(auto&[pid,p]:world_.players)if(p.scene==m.scene&&p.vitals.alive()&&world_.now-motion_[pid].lastSeen<8&&!MovementSystem::safe(p.position,p.scene,catalog_)) {
                double d=distance(p.position,m.position);
                if(d<best&&(p.scene!="world"||clearSight(m.position,p.position,buildings_))) {
                    target=&p;
                    best=d;
                }
            }
            if(world_.now<m.tauntUntil&&world_.players.contains(m.tauntedBy)){
                auto& t=world_.players.at(m.tauntedBy);if(t.vitals.alive()&&t.scene==m.scene&&!MovementSystem::safe(t.position,t.scene,catalog_)&&distance(t.position,m.position)<40){target=&t;best=distance(t.position,m.position);}
            }
            if(m.definition=="dungeon_boss"&&world_.runs.contains(m.scene)) {
                auto&r=world_.runs.at(m.scene);
                auto&d=catalog_.dungeons.at(r.definition);
                int need=d.objective=="hunt"?0:d.objective=="seals"||d.objective=="braziers"?3:1;
                if(r.guardsRemaining>0||r.objectives<need)target=nullptr;
            }
            if(monsterAbility(m,target))continue;
            Vec3 home=homes_.contains(id)?homes_.at(id):m.position;
            bool leash=distance(m.position,home)>65;
            Vec3 goal=target&&!leash?target->position:home;
            double d=distance(goal,m.position);
            if(d>2.8) {
                double speed=m.boss?2.5:3.1;
                Vec3 n=m.position;
                n.x+=(goal.x-n.x)/d*speed*dt;
                n.z+=(goal.z-n.z)/d*speed*dt;
                if(m.scene!="world"||(!blocked(n,buildings_,.6)&&contentAllowed(n))){if(m.scene=="world")n.y=worldLandHeight(n,catalog_);m.position=n;}
            }
            if(target&&!leash&&best<=3.4&&world_.now>=m.nextAttack)try {
                CombatSystem::monsterAttack(world_,catalog_,id,target->id);
            }
            catch(const RuleError&) {}
        }
        for(auto&id:remove) {
            auto old=world_.monsters.at(id);
            auto home=homes_.at(id);
            auto newId=world_.nextId(old.boss?"raid":"wild");
            CombatSystem::spawn(world_,newId,home,old.level,old.region,old.boss,old.definition);
            if(old.boss&&catalog_.bosses.contains(old.definition)) {
                auto&b=catalog_.bosses.at(old.definition);
                world_.monsters.at(newId).vitals.hp=world_.monsters.at(newId).vitals.maxHp=b.hp;
                world_.monsters.at(newId).attack=b.attack;
            }
            homes_[newId]=home;
            world_.monsters.erase(id);
            homes_.erase(id);
            deadAt_.erase(id);
        }
    }
}
