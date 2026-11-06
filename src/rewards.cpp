#include "aetherwild/services.hpp"
namespace aw {
    bool RewardSystem::claim(World& w,Player& p,const Id& id,const Catalog& c) {
        if(p.processedRewards.contains(id))return false;
        require(w.rewards.contains(id),"找不到獎勵事件");
        auto&r=w.rewards.at(id);
        require(r.player==p.id,"獎勵不屬於此角色");
        Player next=p;
        require(r.coins>=0&&next.coins<=INT64_MAX-r.coins,"獎勵金幣溢位");
        next.coins+=r.coins;
        ProgressionSystem::grantXp(next,r.xp);
        for(auto&[k,n]:r.materials)InventorySystem::add(next.materials,k,n);
        for(auto&i:r.items) {
            require(std::none_of(next.inventory.begin(),next.inventory.end(),[&](auto&v) {
                return v.id==i.id;
            }),"重複物品ID");
            next.inventory.push_back(i);
        }
        next.processedRewards.insert(id);
        ProgressionSystem::refresh(next,c);
        p=std::move(next);
        w.events.push_back( {
            "RewardClaimed",p.id,id,0, {}
        });
        return true;
    }
    void RewardSystem::onDeath(World& w,Monster& m,const Catalog& c) {
        if(m.rewardIssued)return;
        m.rewardIssued=true;
        std::vector<std::pair<Id,double>> participants;
        for(auto&[id,n]:m.contribution)if(n>0&&w.players.contains(id))participants.emplace_back(id,n);
        std::sort(participants.begin(),participants.end(),[](auto&a,auto&b) {
            return a.second!=b.second?a.second>b.second:a.first<b.first;
        });
        std::size_t items=m.boss?std::min<std::size_t>(3,(participants.size()+3)/4):0;
        items=std::min(items,participants.size());
        for(std::size_t i=0;i<participants.size();++i) {
            auto&p=w.players.at(participants[i].first);
            Reward r;
            r.id="reward:"+m.id+":"+p.id;
            r.player=p.id;
            r.xp=m.level*(m.boss?50:12);
            r.coins=m.level*(m.boss?20:3);
            if(i<items) {
                // Stable slot selection: retrying a death event cannot reroll its pool.
                std::uint32_t hash=2166136261u;
                for(unsigned char ch:m.id)hash=(hash^ch)*16777619u;
                const char* slots[]= {
                    "weapon","armor","helm","boots","charm"
                };
                Id slot=slots[(hash+i)%5];
                if(slot=="weapon")slot=c.classes.at(p.profession).weapon==WeaponKind::Staff?"staff":c.classes.at(p.profession).weapon==WeaponKind::Bow?"bow":"sword";
                auto code=m.definition+"_"+slot;
                if(c.items.contains(code))r.items.push_back(InventorySystem::create(w,c,code,Rarity::Rare));
            }
            w.rewards.try_emplace(r.id,r);
            QuestSystem::record(p,m.boss?"boss":"hunt",m.boss?m.definition:m.region,c);
        }
        if(m.scene!="world"&&w.runs.contains(m.scene)) {
            auto&run=w.runs.at(m.scene);
            if(m.definition=="dungeon_boss")run.bossDead=true;
            else run.guardsRemaining=std::max(0,run.guardsRemaining-1);
        }
        w.events.push_back( {
            "MonsterDied", {},
            m.id,0, {}
        });
    }
}
