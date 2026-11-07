#include "aetherwild/services.hpp"
#include "aetherwild/content.hpp"
namespace aw {
    const NpcDefinition& nearbyNpc(const Player& p,const Id& id,std::string_view role,const Catalog& c) {
        require(p.vitals.alive()&&p.scene=="world"&&c.npcs.contains(id),"NPC 不存在或無法互動");
        auto&n=c.npcs.at(id);
        require(contentRegionOpen(c,n.region),"此區服務尚未開放");
        require(n.role==role&&distance(p.position,n.position)<=8,"NPC 職責或互動距離不符");
        return n;
    }
    Simulation::Simulation():catalog_(makeCatalog()) {}
    Player& Simulation::addPlayer(const Id& id,const std::string& name,const Id& profession) {
        require(!id.empty()&&id.size()<=128&&!name.empty()&&name.size()<=128&&!world_.players.contains(id),"角色 ID 或名稱無效");
        require(catalog_.classes.contains(profession)&&catalog_.classes.at(profession).level==1,"初始職業無效");
        Player p;
        p.id=id;
        p.accountId="local:"+id;
        p.name=name;
        p.profession=profession;
        p.jobs= {profession};
        ProgressionSystem::refresh(p,catalog_);
        p.vitals.hp=p.vitals.maxHp;
        p.vitals.mp=p.vitals.maxMp;
        auto code=startingWeapon(catalog_,profession);
        p.inventory.push_back(InventorySystem::create(world_,catalog_,code));
        InventorySystem::equip(p,p.inventory.front().id,catalog_);
        if(catalog_.classes.at(profession).weapon==WeaponKind::Bow)p.materials["woodarrow"]=20;
        return world_.players.emplace(id,std::move(p)).first->second;
    }
    void Simulation::advance(double dt) {
        require(std::isfinite(dt)&&dt>=0&&dt<=10,"單次模擬時間範圍0–10秒");
        world_.now+=dt;
        for(auto&[id,p]:world_.players)MovementSystem::tickStamina(p,dt,false,world_.now);
    }
}

