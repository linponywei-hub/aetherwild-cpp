#include "aetherwild/services.hpp"
#include "aetherwild/geometry.hpp"
namespace aw {
    bool MovementSystem::safe(Vec3 pos,const Id& scene,const Catalog& c) {
        if(scene!="world")return false;
        for(auto&[id,r]:c.regions)if(!r.wilderness&&onCityFloor(pos,r))return true;
        return false;
    }
    double MovementSystem::landingDamage(double drop,double hp,double seconds) {
        if(!std::isfinite(drop)||!std::isfinite(hp)||!std::isfinite(seconds)||hp<=0||seconds<.6||drop<=6)return 0;
        return std::max(0.0,std::min( {180.0,std::floor(hp*.35),std::round((drop-6)*3)}));
    }
    double MovementSystem::land(Player& p,double drop,double airTime) {
        return p.receiveDamage(landingDamage(drop,p.vitals.maxHp,airTime));
    }
    void MovementSystem::tickStamina(Player& p,double dt,bool sprint,double now) {
        require(std::isfinite(dt)&&dt>=0&&dt<=10&&std::isfinite(now),"體力時間參數錯誤");
        dt=std::min(dt,std::max(0.0,now-p.staminaUpdatedAt));
        p.staminaUpdatedAt=std::max(p.staminaUpdatedAt,now);
        if(sprint&&!p.sprintLocked&&p.vitals.alive()) {
            p.vitals.stamina=std::max(0.0,p.vitals.stamina-dt*18);
            p.regenReady=now+2;
            if(p.vitals.stamina<=0)p.sprintLocked=true;
        }
        else if(now>=p.regenReady)p.vitals.stamina=std::min(p.vitals.maxStamina,p.vitals.stamina+dt*12);
        if(p.vitals.stamina>=p.vitals.maxStamina*.2)p.sprintLocked=false;
    }
    void MovementSystem::move(Player& p,Vec3 dest,double dt,bool sprint,const Catalog& c,double now) {
        require(p.vitals.alive()&&finite(dest)&&std::isfinite(dt)&&dt>0&&dt<=1,"移動參數錯誤");
        require(std::abs(dest.x)<=1800&&std::abs(dest.z)<=1800&&std::abs(dest.y-p.position.y)<=1.4,"超出邊界或需要樓層碰撞解算");
        bool running=sprint&&!p.sprintLocked&&p.vitals.stamina>=18*dt;
        double speed=5+InventorySystem::bonuses(p,c).speed;
        require(distance(dest,p.position)<=(running?speed*1.6:speed)*dt+1e-6,"移動速度超過上限");
        tickStamina(p,dt,running,now);
        p.position=dest;
        if(p.scene=="world")for(auto&[id,r]:c.regions)if(distance(dest,r.center)<=r.radius) {
            QuestSystem::record(p,"visit",id,c);
            if(!r.wilderness)p.lastCity=id;
        }
    }
}
