#include "aetherwild/talents.hpp"
#include "aetherwild/runtime.hpp"
#include <fstream>
#include <random>
#include <cstdlib>
#include <chrono>
#include "../runtime/support.hpp"
namespace aw {
using namespace runtime_detail;
    Json GameRuntime::catalog()const {
        Json j;
        for(auto k: {
            "regions","items","resources","npcs","dungeons","quests","recipes","classes","skills","buildings"
        })j[k]=Json::array();
        for(auto&[id,r]:catalog_.regions)j["regions"].push(Json { {"urbanRadius",urbanRadius(r)},
            {
                "id",id
            },
            {
                "name",r.name
            },
            {
                "position",vec(r.center)
            },
            {
                "elevation",r.center.y+cityElevation(id)
            },
            {
                "radius",r.radius
            },
            {
                "level",r.level
            },
            {
                "city",r.city
            },
            {
                "wilderness",r.wilderness
            }
        });
        for(auto&[id,i]:catalog_.items)j["items"].push(Json {
            {
                "id",id
            },
            {
                "name",i.name
            },
            {
                "slot",static_cast<int>(i.slot)
            },
            {
                "type",kindName(i.weapon)
            },
            {
                "required",i.requiredLevel
            },
            {
                "stats",statsJson(i.stats)
            }
        });
        for(auto&[id,d]:catalog_.resources)j["resources"].push(Json {
            {
                "id",id
            },
            {
                "name",d.name
            },
            {
                "family",d.family
            },
            {
                "tool",d.tool
            },
            {
                "tier",d.tier
            },
            {
                "need",d.proficiency
            },
            {
                "seconds",d.seconds
            }
        });
        for(auto&[id,n]:catalog_.npcs)j["npcs"].push(Json {
            {
                "id",id
            },
            {
                "name",n.name
            },
            {
                "region",n.region
            },
            {
                "role",n.role
            },
            {
                "position",vec(n.position)
            }
        });
        for(auto&[id,d]:catalog_.dungeons)j["dungeons"].push(Json {
            {
                "id",id
            },
            {
                "name",d.name
            },
            {
                "region",d.region
            },
            {
                "level",d.level
            },
            {
                "entrance",d.entrance
            },
            {
                "position",vec(d.entrancePosition)
            },
            {
                "objective",d.objective
            },
            {
                "guards",d.guards
            }
        });
        for(auto&[id,q]:catalog_.quests)j["quests"].push(Json {
            {
                "id",id
            },
            {
                "name",q.name
            },
            {
                "type",q.type
            },
            {
                "target",q.target
            },
            {
                "npc",q.npc
            },
            {
                "level",q.level
            },
            {
                "count",q.count
            },
            {
                "coins",q.coins
            },
            {
                "xp",q.xp
            }
        });
        for(auto&[id,r]:catalog_.recipes)j["recipes"].push(Json {
            {
                "id",id
            },
            {
                "output",r.output
            },
            {
                "name",catalog_.items.contains(r.output)?catalog_.items.at(r.output).name:catalog_.tools.contains(r.output)?catalog_.tools.at(r.output).name:r.output=="potion"?"治療藥水":r.output=="energy_potion"?"能量藥水":"箭矢"
            },
            {
                "cost",costJson(r.cost)
            },
            {
                "station",r.station
            },
            {
                "tier",r.tier
            },
            {
                "count",r.count
            }
        });
        for(auto&[id,c]:catalog_.classes)if(c.secret.empty())j["classes"].push(Json {
            {
                "id",id
            },
            {
                "name",c.name
            },
            {
                "level",c.level
            },
            {
                "parent",c.parent
            },
            {
                "cost",costJson(c.cost)
            },
            {
                "type",kindName(c.weapon)
            }
        });
        j["talents"]=Json::array();for(const auto& d:talentDefinitions())j["talents"].push(Json{{"id",d.id},{"name",d.name},{"branch",d.branch},{"parent",d.parent},{"description",d.description},{"level",d.level}});
        for(auto&[id,s]:catalog_.skills)if(s.effect!="dash")j["skills"].push(Json {
            {
                "id",id
            },
            {
                "name",s.name
            },
            {
                "mana",s.mana
            },
            {
                "cooldown",s.cooldown
            },
            {
                "damage",s.damage
            },
            {
                "heal",s.heal
            }
        });
        for(auto&b:buildings_)j["buildings"].push(Json {
            {
                "id",b.id
            },
            {
                "region",b.region
            },
            {
                "position",vec(b.position)
            },
            {
                "width",b.width
            },
            {
                "depth",b.depth
            },
            {
                "height",b.height
            },
            {
                "style",b.style
            }
        });
        j["contentIndex"]=Json::object();
        for(auto section:{"classes","monsters","equipment","skills","regions","releases","cityLore","storyRealms"})j["contentIndex"][section]=contentBundle().at(section);
        if(catalog_.worldLimit>1800){
            Json heights=Json::array();for(int z=0;z<=128;z++)for(int x=0;x<=128;x++)heights.push(worldLandHeight({-catalog_.worldLimit+x*catalog_.worldLimit/64,0,-catalog_.worldLimit+z*catalog_.worldLimit/64},catalog_));
            j["terrain"]=Json{{"side",129},{"step",catalog_.worldLimit/64},{"x",-catalog_.worldLimit},{"z",-catalog_.worldLimit},{"heights",heights}};
        }
        j["landscape"]=atlasSurface();
        j["atlas"]=contentBundle().at("atlas_layout");
        j["worldLimit"]=catalog_.worldLimit;
        j["worldVersion"]=contentState_.flag("expanded")?2:1;
        j["release"]=contentState_.num("release",1);
        j["prices"]=costJson(catalog_.prices);
        return j;
    }
}
