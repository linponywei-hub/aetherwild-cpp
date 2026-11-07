#include "aetherwild/social.hpp"
namespace aw {
    namespace {
        Role role(const SessionState&s,const Id&id) {
            require(s.accounts.contains(id),"沒有受信任帳號");
            return s.accounts.at(id).role;
        }
        void admin(const SessionState&s,const Id&id) {
            require(role(s,id)==Role::Admin||role(s,id)==Role::Owner,"需要管理員權限");
        }
    }
    Id PartySystem::create(World&w,SessionState&s,const Id&leader) {
        require(w.players.contains(leader)&&!s.memberParty.contains(leader),"角色不存在或已有隊伍");
        Party p;
        p.id=w.nextId("party");
        p.leader=leader;
        p.members.insert(leader);
        s.memberParty[leader]=p.id;
        s.parties.emplace(p.id,p);
        return p.id;
    }
    void PartySystem::invite(const World&w,SessionState&s,const Id&leader,const Id&target) {
        require(s.memberParty.contains(leader)&&w.players.contains(target)&&target!=leader&&!s.memberParty.contains(target),"邀請對象無效");
        auto&p=s.parties.at(s.memberParty.at(leader));
        require(p.leader==leader&&p.members.size()<5,"需要隊長權限且隊伍不可滿員");
        p.invited.insert(target);
    }
    void PartySystem::join(const World&w,SessionState&s,const Id&player,const Id&id) {
        require(w.players.contains(player)&&!s.memberParty.contains(player)&&s.parties.contains(id),"加入隊伍條件不符");
        auto&p=s.parties.at(id);
        require(p.invited.contains(player)&&p.members.size()<5,"未受邀請或已滿員");
        p.members.insert(player);
        p.invited.erase(player);
        s.memberParty[player]=id;
    }
    void PartySystem::leave(SessionState&s,const Id&player) {
        require(s.memberParty.contains(player),"沒有隊伍");
        auto id=s.memberParty.at(player);
        auto&p=s.parties.at(id);
        p.members.erase(player);
        s.memberParty.erase(player);
        if(p.members.empty())s.parties.erase(id);
        else if(p.leader==player)p.leader=*p.members.begin();
    }
    double PvpSystem::attack(World&w,const Catalog&c,const SessionState&s,const Id&from,const Id&to,double bowPower) {
        require(s.pvpEnabled&&from!=to,"PvP 尚未開啟或目標錯誤");
        auto&a=w.players.at(from);
        auto&b=w.players.at(to);
        a.attackIntent();
        require(b.vitals.alive()&&a.scene==b.scene,"目標已死亡或場景不同");
        require(!MovementSystem::safe(a.position,a.scene,c)&&!MovementSystem::safe(b.position,b.scene,c),"安全區禁止 PvP");
        require(!s.memberParty.contains(from)||!s.memberParty.contains(to)||s.memberParty.at(from)!=s.memberParty.at(to),"不能攻擊隊友");
        auto&job=c.classes.at(a.profession);
        require(distance(a.position,b.position)<=job.range&&w.now>=a.attackReady,"距離或冷卻不符");
        if(job.weapon==WeaponKind::Bow)InventorySystem::pay(a.materials, {
            {
                "woodarrow",1
            }
        });
        if(job.weapon==WeaponKind::Staff) {
            require(a.vitals.mp>=2,"魔力不足");
            a.vitals.mp-=2;
        }
        double damage=job.base.attack+(a.level-1)*3+InventorySystem::bonuses(a,c).attack-InventorySystem::bonuses(b,c).defense;
        damage=std::max(1.0,std::round(damage*(job.weapon==WeaponKind::Staff?.32:job.weapon==WeaponKind::Bow?std::clamp(bowPower,.55,1.6):1)));
        if(a.talents.contains("execution")&&b.vitals.hp<b.vitals.maxHp*.3)damage*=1.2;
        if(w.now<b.shieldUntil) {
            double shield=std::min(damage,b.shield);
            b.shield-=shield;
            damage-=shield;
        }
        auto actual=b.receiveDamage(damage);
        a.attackReady=w.now+.8;
        w.events.push_back( {
            "PvpDamage",from,to,actual, {}
        });
        return actual;
    }
    void AdministrationSystem::setPvp(World&w,SessionState&s,const Id&actor,bool enabled) {
        admin(s,actor);
        s.pvpEnabled=enabled;
        w.events.push_back( {
            "AdminSetPvp",actor, {},
            enabled?1.0:0.0, {}
        });
    }
    void AdministrationSystem::grantAdmin(World&w,SessionState&s,const Id&actor,const Id&target) {
        require(role(s,actor)==Role::Owner,"只有擁有者可以發放管理員");
        require(role(s,target)!=Role::Owner,"不可修改擁有者");
        s.accounts.at(target).role=Role::Admin;
        w.events.push_back( {
            "AdminGranted",actor,target,0, {}
        });
    }
    void AdministrationSystem::teleportToCity(World&w,const Catalog&c,const SessionState&s,const Id&actor,const Id&player,const Id&city) {
        admin(s,actor);
        require(c.regions.contains(city)&&!c.regions.at(city).wilderness,"目的地不是城鎮");
        auto&p=w.players.at(player);
        require(p.scene=="world"&&p.vitals.alive(),"先離開副本且角色須存活");
        p.position=c.regions.at(city).center;
        p.lastCity=city;
        w.events.push_back( {
            "AdminTeleport",actor,player,0,city
        });
    }
}
