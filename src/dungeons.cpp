#include "aetherwild/services.hpp"
namespace aw {
    Id DungeonSystem::enter(World& w,Player& p,const Id& id,const Catalog& c) {
        require(c.dungeons.contains(id)&&p.scene=="world"&&p.vitals.alive(),"無法進入副本");
        auto&d=c.dungeons.at(id);
        require(p.level>=d.level,"副本等級不足");
        require(distance(p.position,d.entrancePosition)<=8,"請先抵達副本入口");
        DungeonRun r;
        r.id=w.nextId("run");
        r.definition=id;
        r.player=p.id;
        r.members.insert(p.id);
        r.returnPositions[p.id]=p.position;
        r.guardsRemaining=d.guards;
        r.returnScene=p.scene;
        r.returnPosition=p.position;
        w.runs.emplace(r.id,r);
        p.scene=r.id;
        p.position= {0,0,0};
        for(int i=0;i<d.guards;i++) {
            auto mid=w.nextId("guard");
            CombatSystem::spawn(w,mid, {2.0+i*.1,0,2},
            d.level,d.region);
            w.monsters.at(mid).scene=r.id;
        }
        auto mid=w.nextId("boss");
        CombatSystem::spawn(w,mid, {3,0,3},
        d.level,d.region,true,"dungeon_boss");
        w.monsters.at(mid).scene=r.id;
        return r.id;
    }
    void DungeonSystem::interactObjective(World& w,Player& p,const Id& object,const Catalog& c) {
        require(w.runs.contains(p.scene)&&p.vitals.alive(),"不在副本或已死亡");
        auto&r=w.runs.at(p.scene);
        require((r.player==p.id||r.members.contains(p.id))&&r.guardsRemaining==0,"先清除守衛");
        auto&d=c.dungeons.at(r.definition);
        int need=d.objective=="hunt"?0:d.objective=="seals"||d.objective=="braziers"?3:1;
        require(object=="objective_1"||(need==3&&(object=="objective_2"||object=="objective_3")),"目標物件不存在");
        require(need>0&&!r.interacted.contains(object),"目標已互動");
        r.interacted.insert(object);
        ++r.objectives;
    }
    void DungeonSystem::openChest(World& w,Player& p,const Catalog& c) {
        require(w.runs.contains(p.scene)&&p.vitals.alive(),"不在副本或已死亡");
        auto&r=w.runs.at(p.scene);
        require((r.player==p.id||r.members.contains(p.id))&&!r.chestClaimed.contains(p.id)&&r.bossDead,"副本未完成或已領獎");
        auto&d=c.dungeons.at(r.definition);
        int need=d.objective=="hunt"?0:d.objective=="seals"||d.objective=="braziers"?3:1;
        require(r.guardsRemaining==0&&r.objectives>=need,"副本目標未完成");
        auto rewardId="chest:"+r.id+":"+p.id;
        Reward reward;
        reward.id=rewardId;
        reward.player=p.id;
        reward.xp=d.level*30;
        reward.coins=d.level*20;
        w.rewards.try_emplace(rewardId,reward);
        RewardSystem::claim(w,p,rewardId,c);
        if(!d.skill.empty())p.learnedSkills.insert(d.skill);
        if(!d.secret.empty())p.secrets.insert(d.secret);
        QuestSystem::record(p,"dungeon",d.id,c);
        r.chestClaimed.insert(p.id);
        r.claimed=r.chestClaimed.size()>=r.members.size();
    }
    void DungeonSystem::leave(World& w,Player& p) {
        require(w.runs.contains(p.scene),"不在副本");
        auto& active=w.runs.at(p.scene);
        require(active.player==p.id||active.members.contains(p.id),"副本成員不符");
        auto run=active;
        active.members.erase(p.id);
        p.scene=run.returnScene;
        p.position=run.returnPositions.contains(p.id)?run.returnPositions.at(p.id):run.returnPosition;
        if(!active.members.empty()) {
            if(active.player==p.id)active.player=*active.members.begin();
            return;
        }
        for(auto it=w.monsters.begin();it!=w.monsters.end();)if(it->second.scene==run.id)it=w.monsters.erase(it);
        else ++it;
        w.runs.erase(run.id);
    }
}
