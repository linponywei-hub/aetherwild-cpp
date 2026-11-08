#include "aetherwild/services.hpp"
namespace aw {
    void QuestSystem::accept(Player& p,const Id& id,const Id& npc,const Catalog& c) {
        nearbyNpc(p,npc,"guild",c);
        require(c.quests.contains(id),"未知任務");
        auto&q=c.quests.at(id);
        require(npc==q.npc,"任務委託人不符");
        require(p.level>=q.level,"任務等級不足");
        require(!p.quests.contains(id),"已接取此任務");
        p.quests.emplace(id,QuestProgress {});
    }
    void QuestSystem::record(Player& p,const std::string& type,const Id& target,const Catalog& c) {
        for(auto&[id,v]:p.quests) {
            auto&q=c.quests.at(id);
            if(!v.claimed&&q.type==type&&(q.target.empty()||q.target==target))v.count=std::min(q.count,v.count+1);
        }
    }
    void QuestSystem::claim(Player& p,const Id& id,const Id& npc,const Catalog& c) {
        nearbyNpc(p,npc,"guild",c);
        require(c.quests.contains(id)&&p.quests.contains(id),"尚未接取此任務");
        auto&q=c.quests.at(id);
        auto&v=p.quests.at(id);
        require(npc==q.npc&&!v.claimed&&v.count>=q.count,"任務未完成、已領取或委託人錯誤");
        Player next=p;
        require(next.coins<=INT64_MAX-q.coins,"金幣溢位");
        next.coins+=q.coins;
        ProgressionSystem::grantXp(next,q.xp);
        next.quests.at(id).claimed=true;
        ProgressionSystem::refresh(next,c);
        p=std::move(next);
    }
}
