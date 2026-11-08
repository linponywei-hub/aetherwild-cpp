#include "aetherwild/services.hpp"
namespace aw {
    void EconomySystem::buy(World& w,Player& p,const Id& code,Quantity n,const Id& npc,const Catalog& c) {
        nearbyNpc(p,npc,"merchant",c);
        require(n>0&&n<=1000&&c.prices.contains(code),"商品或購買數量錯誤");
        auto price=c.prices.at(code);
        require(price>0&&n<=INT64_MAX/price,"金額溢位");
        Quantity total=price*n;
        require(p.coins>=total,"金幣不足");
        Player next=p;
        next.coins-=total;
        if(c.items.contains(code)) {
            require(n<=50,"單次最多購買50件裝備");
            for(Quantity i=0;i<n;i++)next.inventory.push_back(InventorySystem::create(w,c,code));
        }
        else InventorySystem::add(next.materials,code,n);
        p=std::move(next);
    }
    void EconomySystem::transfer(Player& a,Player& b,Quantity n) {
        require(a.id!=b.id&&n>0&&a.coins>=n&&b.coins<=INT64_MAX-n,"轉帳數值錯誤");
        require(a.vitals.alive()&&b.vitals.alive()&&a.scene==b.scene&&distance(a.position,b.position)<=5,"轉帳對象不在附近");
        a.coins-=n;
        b.coins+=n;
    }
}
