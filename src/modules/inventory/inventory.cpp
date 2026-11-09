#include "aetherwild/talents.hpp"
#include "aetherwild/services.hpp"
#include <limits>
namespace aw {
    bool InventorySystem::canPay(const Materials& m,const Materials& cost) {
        for(auto&[id,n]:cost) {
            auto i=m.find(id);
            if(n<=0||i==m.end()||i->second<n)return false;
        }
        return true;
    }
    void InventorySystem::pay(Materials& m,const Materials& c) {
        require(canPay(m,c),"材料不足");
        for(auto&[id,n]:c)m.at(id)-=n;
    }
    void InventorySystem::add(Materials& m,const Id& id,Quantity n) {
        require(n>0,"數量必須大於零");
        auto old=m.contains(id)?m.at(id):0;
        require(old<=INT64_MAX-n,"數量溢位");
        m[id]=old+n;
    }
    ItemInstance InventorySystem::create(World& w,const Catalog& c,const Id& code,Rarity r,int required) {
        require(c.items.contains(code),"未知裝備定義");
        require(static_cast<int>(r)>=0&&static_cast<int>(r)<=6&&required>=0&&required<=100,"裝備稀有度或需求等級錯誤");
        auto&d=c.items.at(code);
        return {
            w.nextId("item"),code,r,required?required:d.requiredLevel,0
        };
    }
    Stats InventorySystem::itemStats(const ItemInstance& i,const Catalog& c) {
        const auto& d=c.items.at(i.definition);
        auto b=d.stats;
        constexpr double rarity[]= {1,1.4,1.9,2.25,2.8,3.8,5};
        double m=rarity[static_cast<int>(i.rarity)]*(1+.16*i.upgrade);
        b.hp*=m;
        b.mp*=m;
        b.defense*=m;
        b.crit*=m;
        b.speed*=m;
        b.attack*=m*(d.slot==Slot::Weapon?1+.08*(i.requiredLevel-1):1);
        return b;
    }
    Stats InventorySystem::bonuses(const Player& p,const Catalog& c) {
        Stats s;
        std::map<Id,int> sets;
        std::set<Id> seen;
        for(auto&[slot,id]:p.equipped) {
            auto i=std::find_if(p.inventory.begin(),p.inventory.end(),[&](auto&v) {
                return v.id==id;
            });
            if(i==p.inventory.end()||!seen.insert(id).second)continue;
            auto&d=c.items.at(i->definition);
            if(d.slot!=slot)continue;
            auto b=itemStats(*i,c);
            s+=b;
            if(!d.set.empty())sets[d.set]++;
        }
        for(auto&[id,n]:sets) {
            auto&v=c.sets.at(id);
            if(n>=2)s+=v.two;
            if(n>=4)s+=v.four;
        }
        for(const auto& d:talentDefinitions())if(p.talents.contains(d.id))s+=d.bonus;
        return s;
    }
    void InventorySystem::equip(Player& p,const Id& id,const Catalog& c) {
        require(p.vitals.alive(),"死亡角色無法裝備");
        auto i=std::find_if(p.inventory.begin(),p.inventory.end(),[&](auto&v) {
            return v.id==id;
        });
        require(i!=p.inventory.end(),"背包內沒有此裝備");
        auto&d=c.items.at(i->definition);
        require(p.level>=i->requiredLevel,"裝備等級不足");
        require(d.slot!=Slot::Weapon||d.weapon==c.classes.at(p.profession).weapon,"職業武器種類不符");
        p.equipped[d.slot]=id;
        ProgressionSystem::refresh(p,c);
    }
    void InventorySystem::unequip(Player& p,const Id& id,const Catalog& c) {
        require(p.vitals.alive(),"死亡角色無法卸下裝備");
        auto slot=std::find_if(p.equipped.begin(),p.equipped.end(),[&](const auto& e){return e.second==id;});
        require(slot!=p.equipped.end(),"此裝備未穿戴");
        p.equipped.erase(slot);
        ProgressionSystem::refresh(p,c);
    }
    void InventorySystem::usePotion(Player& p,const Id& id,const Catalog&) {
        require(p.vitals.alive(),"死亡角色無法喝藥");
        require(id=="potion"||id=="energy_potion","未知藥水");
        pay(p.materials, {
            {id,1}
        });
        if(id=="potion")p.vitals.restore(p.talents.contains("renewal")?87.5:70,0);
        else p.vitals.restore(0,p.talents.contains("renewal")?75:60);
    }
    void InventorySystem::storeItem(Player& p,const Id& id,const Id& npc,const Catalog& c,bool withdraw) {
        nearbyNpc(p,npc,"merchant",c);
        require(p.vitals.alive(),"死亡角色無法存取裝備");
        auto& source=withdraw?p.warehouseItems:p.inventory;
        auto& target=withdraw?p.inventory:p.warehouseItems;
        auto item=std::find_if(source.begin(),source.end(),[&](const auto& v){return v.id==id;});
        require(item!=source.end(),withdraw?"倉庫內沒有此裝備":"背包內沒有此裝備");
        require(std::none_of(p.equipped.begin(),p.equipped.end(),[&](const auto& e){return e.second==id;}),"穿戴中的裝備無法存入，請先換裝");
        require(std::none_of(target.begin(),target.end(),[&](const auto& v){return v.id==id;}),"裝備實例重複");
        // Copy first: a failed allocation leaves the original instance untouched.
        target.push_back(*item);
        source.erase(item);
    }
    void InventorySystem::store(Player& p,const Id& id,Quantity n,const Id& npc,const Catalog& c,bool withdraw) {
        nearbyNpc(p,npc,"merchant",c);
        require(n>0,"數量必須為正");
        auto source=withdraw?p.warehouse:p.materials;
        auto target=withdraw?p.materials:p.warehouse;
        pay(source, {
            {id,n}
        });
        add(target,id,n);
        if(withdraw) {
            p.warehouse=std::move(source);
            p.materials=std::move(target);
        }
        else {
            p.materials=std::move(source);
            p.warehouse=std::move(target);
        }
    }
}

