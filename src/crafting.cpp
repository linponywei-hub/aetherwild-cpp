#include "aetherwild/services.hpp"
namespace aw {
    Id CraftingSystem::requiredRegion(const Recipe& r,const Player& p,const Catalog& c) {
        if(!r.equipment||r.tier<2)return {};
        auto&i=c.items.at(r.output);
        auto type=i.slot==Slot::Weapon?i.weapon:c.classes.at(p.profession).weapon;
        return type==WeaponKind::Staff?"astral":type==WeaponKind::Bow?"verdant":"cinder";
    }
    void CraftingSystem::craft(World& w,Player& p,const Id& id,const Id& npc,const Catalog& c) {
        require(p.vitals.alive()&&c.recipes.contains(id),"角色死亡或未知配方");
        auto&r=c.recipes.at(id);
        if(r.station=="smith") {
            auto&n=nearbyNpc(p,npc,"smith",c);
            auto region=requiredRegion(r,p,c);
            require(region.empty()||region==n.region,"此高階裝備必須到指定城市鍛造");
        }
        Player next=p;
        auto materials=r.cost;
        if(c.arrowBonus.contains(r.output)&&c.classes.at(p.profession).weapon==WeaponKind::Bow)for(auto&[k,n]:materials)n=(n+1)/2;
        InventorySystem::pay(next.materials,materials);
        if(r.equipment)next.inventory.push_back(InventorySystem::create(w,c,r.output));
        else if(c.tools.contains(r.output)) {
            auto&t=c.tools.at(r.output);
            next.tools[t.kind]=std::max(next.tools[t.kind],t.tier);
        }
        else InventorySystem::add(next.materials,r.output,r.count);
        p=std::move(next);
        w.events.push_back( {
            "Crafted",p.id,id,static_cast<double>(r.count), {}
        });
    }
}
