#include "aetherwild/services.hpp"
namespace aw {
    EnhancementQuote EnhancementSystem::quote(int level) {
        require(level>=0&&level<20,"裝備強化上限為 +20");
        return {static_cast<Quantity>(std::ceil(220*std::pow(1.8,level))),{{"ore",2}},level<6?1:std::max(.02,.75*std::pow(.76,level-6)),level+1};
    }
    bool EnhancementSystem::enhance(World& w,Player& p,const Id& id,const Id& npc,int expected,const Catalog& c,double roll) {
        nearbyNpc(p,npc,"smith",c);
        auto it=std::find_if(p.inventory.begin(),p.inventory.end(),[&](const auto& i){return i.id==id;});
        require(it!=p.inventory.end()&&c.items.contains(it->definition),"請選擇背包中的裝備，倉庫裝備須先取出");
        require(it->upgrade==expected,"裝備強化等級已變更，請重新確認報價");
        auto q=quote(it->upgrade);
        require(p.coins>=q.coins&&InventorySystem::canPay(p.materials,q.materials),"強化金幣或鐵礦不足");
        require(std::isfinite(roll)&&roll>=0&&roll<1,"強化抽樣錯誤");
        // Build a replacement first: rejected operations never charge or change equipment.
        Player next=p;auto index=static_cast<std::size_t>(it-p.inventory.begin());
        bool success=roll<q.chance;next.coins-=q.coins;InventorySystem::pay(next.materials,q.materials);
        next.inventory[index].upgrade=success?q.nextLevel:0;ProgressionSystem::refresh(next,c);p=std::move(next);
        w.events.push_back({success?"EnhancementSucceeded":"EnhancementFailed",p.id,id,static_cast<double>(p.inventory[index].upgrade),success?"鐵匠完成強化":"強化失敗，裝備歸零"});
        return success;
    }
}
