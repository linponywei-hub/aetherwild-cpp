#include "aetherwild/services.hpp"
namespace aw {
    Quantity ProgressionSystem::levelXp(int n) {
        require(n>=1&&n<=100,"等級範圍 1–100");
        return static_cast<Quantity>(std::llround(100*std::pow(1.3,n-1)));
    }
    void ProgressionSystem::grantXp(Player& p,Quantity n) {
        require(n>=0&&p.xp<=INT64_MAX-n,"經驗值錯誤或溢位");
        p.xp+=n;
        while(p.level<100&&p.xp>=levelXp(p.level)) {
            p.xp-=levelXp(p.level);
            ++p.level;
        }
    }
    void ProgressionSystem::refresh(Player& p,const Catalog& c) {
        auto b=c.classes.at(p.profession).base;
        b+=InventorySystem::bonuses(p,c);
        p.vitals.maxHp=b.hp+(p.level-1)*12;
        p.vitals.maxMp=b.mp+(p.level-1)*5;
        p.vitals.maxStamina=std::min(200,100+(p.level-1)*2);
        p.vitals.hp=std::clamp(p.vitals.hp,0.0,p.vitals.maxHp);
        p.vitals.mp=std::clamp(p.vitals.mp,0.0,p.vitals.maxMp);
        p.vitals.stamina=std::clamp(p.vitals.stamina,0.0,p.vitals.maxStamina);
    }
    void ProgressionSystem::changeClass(Player& p,const Id& job,const Id& npc,const Catalog& c) {
        nearbyNpc(p,npc,"guild",c);
        require(c.classes.contains(job),"未知職業");
        auto&j=c.classes.at(job);
        require(p.level>=j.level,"轉職等級不足");
        require(j.parent.empty()||p.jobs.contains(j.parent),"缺少前置職業");
        require(j.secret.empty()||p.secrets.contains(j.secret),"尚未達成隱藏轉職條件");
        if(!p.jobs.contains(job))InventorySystem::pay(p.materials,j.cost);
        p.jobs.insert(job);
        p.profession=job;
        auto w=p.equipped.find(Slot::Weapon);
        if(w!=p.equipped.end()) {
            auto it=std::find_if(p.inventory.begin(),p.inventory.end(),[&](auto&i) {
                return i.id==w->second;
            });
            if(it!=p.inventory.end()&&c.items.at(it->definition).weapon!=j.weapon)p.equipped.erase(w);
        }
        refresh(p,c);
    }
}
