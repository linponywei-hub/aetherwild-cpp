#include "aetherwild/talents.hpp"
#include "aetherwild/runtime.hpp"
#include <fstream>
#include <random>
#include <cstdlib>
#include <chrono>
#include "../runtime/support.hpp"
namespace aw {
using namespace runtime_detail;
    void GameRuntime::input(const Id&key,const Json&j) {
        require(profiles_.contains(key),"登入已失效");
        auto&id=profiles_.at(key).player;
        auto&m=motion_[id];
        auto seq=j.at("sequence").integer();
        require(seq>=0,"輸入序號錯誤");
        if(static_cast<std::uint64_t>(seq)<=m.sequence)return;
        double x=j.num("x"),z=j.num("z"),yaw=j.num("yaw");
        require(std::isfinite(x)&&std::isfinite(z)&&std::isfinite(yaw)&&std::abs(x)<=1&&std::abs(z)<=1&&std::abs(yaw)<1e8,"移動輸入錯誤");
        m.sequence=static_cast<std::uint64_t>(seq);
        double len=std::max(1.0,std::hypot(x,z));
        m.x=x/len;
        m.z=z/len;
        m.yaw=yaw;
        m.sprint=j.flag("sprint");
        m.jump=m.jump||j.flag("jump");
        m.lastInput=m.lastSeen=world_.now;
    }
    void GameRuntime::physics(Player&p,Motion&m,double dt) {
        if(!p.vitals.alive())return;
        if(!m.lift.empty()) {
            auto&l=lifts_.at(m.lift);
            double k=std::clamp((world_.now-l.start)/6,0.0,1.0);
            p.position.y=l.from+(l.to-l.from)*k;
            m.velocityY=0;
            m.grounded=true;
            if(k>=1)m.lift.clear();
            return;
        }
        bool active=world_.now-m.lastInput<.45;
        double speed=5+InventorySystem::bonuses(p,catalog_).speed;
        bool sprint=active&&m.sprint&&!p.sprintLocked&&p.vitals.stamina>=18*dt;
        double vx=active?(m.x*std::cos(m.yaw)-m.z*std::sin(m.yaw)):0;
        double vz=active?(-m.x*std::sin(m.yaw)-m.z*std::cos(m.yaw)):0;
        double scale=speed*(sprint?1.6:1)*dt;
        Vec3 next=p.position;
        auto obstacle=[&](Vec3 v) {
            return p.scene=="world"?(!contentAllowed(v)||blocked(v,buildings_)):std::abs(v.x)>24||std::abs(v.z)>29;
        };
        next.x=std::clamp(next.x+vx*scale,-catalog_.worldLimit+10,catalog_.worldLimit-10);
        if(obstacle(next))next.x=p.position.x;
        next.z=std::clamp(next.z+vz*scale,-catalog_.worldLimit+10,catalog_.worldLimit-10);
        if(obstacle(next))next.z=p.position.z;
        double ground=p.scene=="world"?groundBelow(next,catalog_):0;
        bool walking=std::hypot(vx,vz)>.01;
        if(m.jump&&m.grounded) {
            m.velocityY=8;
            m.grounded=false;
            m.airTime=0;
            m.peak=p.position.y;
        }
        m.jump=false;
        if(m.grounded&&m.velocityY<=0&&p.position.y-ground<=1.4&&p.position.y-ground>=-1.4) {
            next.y=ground;
        }
        else {
            if(m.grounded) {
                m.airTime=0;
                m.peak=p.position.y;
            }
            m.grounded=false;
            m.airTime+=dt;
            m.peak=std::max(m.peak,p.position.y);
            m.velocityY-=22*dt;
            next.y=p.position.y+m.velocityY*dt;
            if(next.y<=ground) {
                next.y=ground;
                auto damage=MovementSystem::land(p,m.peak-ground,m.airTime);
                if(damage>0)world_.events.push_back( {
                    "FallDamage",p.id,p.id,damage, {}
                });
                m.grounded=true;
                m.velocityY=0;
                m.airTime=0;
            }
        }
        p.position=next;
        MovementSystem::tickStamina(p,dt,sprint&&walking,world_.now);
        if(p.scene=="world")for(auto&[id,r]:catalog_.regions) {
            if(onCityFloor(next,r)&&!r.wilderness)p.lastCity=id;
            if(distance(next,r.center)<r.radius)QuestSystem::record(p,"visit",id,catalog_);
        }
    }
}
