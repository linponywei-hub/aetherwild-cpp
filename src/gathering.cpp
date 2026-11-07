#include "aetherwild/services.hpp"
#include <random>
namespace aw {
    namespace {
        void check(World& w,Player& p,ResourceNode& n,const Catalog& c) {
            require(p.vitals.alive()&&p.scene==n.scene&&distance(p.position,n.position)<=4,"無法採集：死亡、場景或距離不符");
            auto&d=c.resources.at(n.definition);
            require(p.gatherXp>=d.proficiency,"採集熟練度不足");
            if(!d.tool.empty())require(p.tools.contains(d.tool)&&p.tools.at(d.tool)>=d.tier,"需要對應階級的工具");
            require(w.now>=n.readyAt,"資源尚未重生");
        }
    }
    void GatheringSystem::begin(World& w,Player& p,const Id& node,const Catalog& c) {
        auto&n=w.resources.at(node);
        check(w,p,n,c);
        if(n.reservedBy&&w.now>n.completeAt+15)n.reservedBy.reset();
        require(!n.reservedBy,"資源已被預約");
        n.reservedBy=p.id;
        n.completeAt=w.now+c.resources.at(n.definition).seconds;
        w.events.push_back( {
            "GatherStarted",p.id,node,n.completeAt, {}
        });
    }
    void GatheringSystem::finish(World& w,Player& p,const Id& node,const Catalog& c) {
        auto&n=w.resources.at(node);
        check(w,p,n,c);
        require(n.reservedBy&&*n.reservedBy==p.id,"沒有採集預約");
        require(w.now>=n.completeAt&&w.now<=n.completeAt+15,"採集尚未完成或已逾時");
        auto&d=c.resources.at(n.definition);
        Player next=p;
        static thread_local std::mt19937 random{std::random_device{}()};
        int amount=1+(p.talents.contains("forage")&&std::uniform_int_distribution<int>(0,3)(random)==0?1:0);
        InventorySystem::add(next.materials,n.definition,amount);
        require(next.gatherXp<=INT64_MAX-d.xp,"熟練度溢位");
        next.gatherXp+=d.xp;
        QuestSystem::record(next,"gather",n.definition,c);
        p=std::move(next);
        n.reservedBy.reset();
        n.readyAt=w.now+30;
        w.events.push_back( {
            "GatherCompleted",p.id,node,static_cast<double>(amount),d.family=="wood"?"砍伐":"採集"
        });
    }
    void GatheringSystem::cancel(World& w,Player& p,const Id& node) {
        auto&n=w.resources.at(node);
        require(n.reservedBy&&*n.reservedBy==p.id,"不能取消他人採集");
        n.reservedBy.reset();
    }
}
