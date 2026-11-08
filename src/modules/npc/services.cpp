#include "aetherwild/npc/system.hpp"
#include "aetherwild/content.hpp"
namespace aw {
    const NpcDefinition& nearbyNpc(const Player& p,const Id& id,std::string_view role,const Catalog& c) {
        require(p.vitals.alive()&&p.scene=="world"&&c.npcs.contains(id),"NPC 不存在或無法互動");
        auto&n=c.npcs.at(id);
        require(contentRegionOpen(c,n.region),"此區服務尚未開放");
        require(n.role==role&&distance(p.position,n.position)<=8,"NPC 職責或互動距離不符");
        return n;
    }
}
