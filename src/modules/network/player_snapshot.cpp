#include "aetherwild/talents.hpp"
#include "aetherwild/runtime.hpp"
#include <fstream>
#include <random>
#include <cstdlib>
#include <chrono>
#include "../runtime/support.hpp"
namespace aw {
using namespace runtime_detail;
    Json GameRuntime::snapshot(const Id&key)const {
        require(profiles_.contains(key),"工作站登入已失效，請重新選擇角色");
        auto&p=world_.players.at(profiles_.at(key).player);
        auto m=motion_.find(p.id);
        Json j {
            {
                "ok",true
            },
            {
                "now",world_.now
            },
            {
                "player",playerJson(p)
            },
            {
                "role",roleName(social_.accounts.at(p.accountId).role)
            },
            {
                "stats",statsJson(catalog_.classes.at(p.profession).base)
            },
            {
                "bonus",statsJson(InventorySystem::bonuses(p,catalog_))
            },
            {
                "pvp",social_.pvpEnabled
            },
            {
                "chat",chat_
            }
        };
        j["equipmentStats"]=Json::object();
        j["enhancement"]=Json{{"maxLevel",20},{"npc",""},{"enabled",features_.at("crafting")},{"quotes",Json::object()}};
        if(p.scene=="world"&&p.vitals.alive())for(const auto&[id,n]:catalog_.npcs) {
            if(n.role=="smith"&&contentRegionOpen(catalog_,n.region)&&distance(p.position,n.position)<=8&&clearSight(p.position,n.position,buildings_)) {
                j["enhancement"]["npc"]=id;break;
            }
        }
        auto describe=[&](const ItemInstance& item) {
            auto st=InventorySystem::itemStats(item,catalog_);
            Json v{{"stats",statsJson(st)},{"delta",statsJson(st)}};
            auto slot=catalog_.items.at(item.definition).slot;
            if(p.equipped.contains(slot)) {
                for(const auto& worn:p.inventory)if(worn.id==p.equipped.at(slot)) {
                    auto current=InventorySystem::itemStats(worn,catalog_);
                    Stats delta{st.hp-current.hp,st.mp-current.mp,st.attack-current.attack,st.defense-current.defense,st.crit-current.crit,st.speed-current.speed};
                    v["delta"]=statsJson(delta);break;
                }
            }
            return v;
        };
        for(const auto& item:p.inventory) {
            j["equipmentStats"][item.id]=describe(item);
            if(item.upgrade<20) {
                auto q=EnhancementSystem::quote(item.upgrade);auto next=item;next.upgrade=q.nextLevel;
                j["enhancement"]["quotes"][item.id]=Json{{"expected",item.upgrade},{"nextLevel",q.nextLevel},{"coins",q.coins},{"ore",2},{"chance",q.chance},{"affordable",p.coins>=q.coins&&InventorySystem::canPay(p.materials,q.materials)},{"nextStats",statsJson(InventorySystem::itemStats(next,catalog_))}};
            }
        }
        for(const auto& item:p.warehouseItems)j["equipmentStats"][item.id]=describe(item);
        j["player"]["talentPoints"]=talentPoints(p);
        j["player"]["skillSlots"]=equippedSkills(p);j["player"]["skillOptions"]=Json::array();for(const auto& skill:p.learnedSkills)if(catalog_.skills.contains(skill)&&catalog_.skills.at(skill).effect!="dash"&&(!catalog_.skillOwners.contains(skill)||catalog_.skillOwners.at(skill)==p.profession))j["player"]["skillOptions"].push(skill);
        j["player"]["weaponType"]=weaponName(catalog_.classes.at(p.profession).weapon);
        const double epoch=std::chrono::duration<double>(std::chrono::system_clock::now().time_since_epoch()).count();
        j["environment"]=Json{{"unixSeconds",epoch},{"cycleSeconds",1800}};
        const bool drawing=m!=motion_.end()&&m->second.bowDrawAt>=0&&p.vitals.alive()&&m->second.bowScene==p.scene&&m->second.bowClass==p.profession&&world_.now-m->second.bowDrawAt<=12;
        j["bow"]=Json{{"drawing",drawing},{"charge",drawing?std::clamp((world_.now-m->second.bowDrawAt)/1.2,0.0,1.0):0},{"fullDrawSeconds",1.2}};
        j["player"]["yaw"]=m==motion_.end()?0:m->second.yaw;
        j["player"]["grounded"]=m==motion_.end()||m->second.grounded;
        j["_motion"]=Json{{"sequence",m==motion_.end()?0:m->second.sequence},
            {"velocityY",m==motion_.end()?0:m->second.velocityY},
            {"jumpPending",m!=motion_.end()&&m->second.jump},
            {"lift",m==motion_.end()?Id{}:m->second.lift},
            {"speed",5+InventorySystem::bonuses(p,catalog_).speed},
            {"sprintAllowed",!p.sprintLocked&&p.vitals.stamina>=.9},
            {"gravity",22},{"jumpSpeed",8}};
        j["xpNext"]=ProgressionSystem::levelXp(p.level);
        for(auto k: {
            "players","monsters","resources","events","objectives","online"
        })j[k]=Json::array();
        for(auto&[id,other]:world_.players) {
            bool online=motion_.contains(id)&&world_.now-motion_.at(id).lastSeen<8;
            if(online) {
                auto pub=playerJson(other,false);
                pub["yaw"]=motion_.contains(id)?motion_.at(id).yaw:0;
                pub["weaponType"]="unarmed";pub["weapon"]=Json{};
                auto equipped=other.equipped.find(Slot::Weapon);
                if(equipped!=other.equipped.end())for(const auto& item:other.inventory)if(item.id==equipped->second){const auto& def=catalog_.items.at(item.definition);pub["weaponType"]=weaponName(def.weapon);pub["weapon"]=Json{{"code",def.id},{"name",def.name},{"type",weaponName(def.weapon)},{"rarity",static_cast<int>(item.rarity)},{"upgrade",item.upgrade}};break;}

                if(!catalog_.classes.at(other.profession).secret.empty())pub["profession"]="unknown";
                j["online"].push(pub);
                if(id!=p.id&&other.scene==p.scene&&distance(other.position,p.position)<=180)j["players"].push(pub);
            }
        }
        for(auto&[id,mob]:world_.monsters)if(mob.scene==p.scene&&distance(mob.position,p.position)<320) {
            j["monsters"].push(Json {
                {
                    "id",id
                },
                {
                    "definition",mob.definition
                },
                {
                    "name",catalog_.bosses.contains(mob.definition)?catalog_.bosses.at(mob.definition).name:mob.definition=="dungeon_boss"?"遺跡守衛者":"原野灰狼"
                },
                {
                    "position",vec(mob.position)
                },
                {
                    "hp",mob.vitals.hp
                },
                {
                    "maxHp",mob.vitals.maxHp
                },
                {
                    "level",mob.level
                },
                {
                    "boss",mob.boss
                },
                {
                    "frozen",world_.now<mob.frozenUntil
                },
                {
                    "yaw",std::atan2(p.position.x-mob.position.x,p.position.z-mob.position.z)
                }
            });
        }
        for(auto&[id,n]:world_.resources)if(n.scene==p.scene&&distance(n.position,p.position)<220)j["resources"].push(Json {
            {
                "id",id
            },
            {
                "definition",n.definition
            },
            {
                "position",vec(n.position)
            },
            {
                "readyAt",n.readyAt
            },
            {
                "reservedBy",n.reservedBy?*n.reservedBy:""
            },
            {
                "completeAt",n.completeAt
            }
        });
        std::size_t start=world_.events.size()>45?world_.events.size()-45:0;
        for(std::size_t i=start;i<world_.events.size();i++) {
            auto&e=world_.events[i];
            if(e.actor.empty()||e.actor==p.id||e.target==p.id)j["events"].push(Json {
                {
                    "index",i
                },
                {
                    "type",e.type
                },
                {
                    "actor",e.actor
                },
                {
                    "target",e.target
                },
                {
                    "value",e.value
                },
                {
                    "text",e.text
                }
            });
        }
        j["features"]=Json::object();
        for(auto&[id,b]:features_)j["features"][id]=b;
        if(social_.memberParty.contains(p.id)) {
            auto&party=social_.parties.at(social_.memberParty.at(p.id));
            Json a=Json::array();
            for(auto&id:party.members)a.push(id);
            j["party"]=Json {
                {
                    "id",party.id
                },
                {
                    "leader",party.leader
                },
                {
                    "members",a
                }
            };
        }
        j["invitations"]=Json::array();
        for(auto&[id,party]:social_.parties)if(party.invited.contains(p.id))j["invitations"].push(Json {
            {
                "id",id
            },
            {
                "leader",party.leader
            }
        });
        if(world_.runs.contains(p.scene)) {
            auto&r=world_.runs.at(p.scene);
            auto&d=catalog_.dungeons.at(r.definition);
            j["run"]=Json {
                {
                    "id",r.id
                },
                {
                    "name",d.name
                },
                {
                    "guards",r.guardsRemaining
                },
                {
                    "objectives",r.objectives
                },
                {
                    "required",d.objective=="hunt"?0:d.objective=="seals"||d.objective=="braziers"?3:1
                },
                {
                    "bossDead",r.bossDead
                },
                {
                    "claimed",r.chestClaimed.contains(p.id)
                }
            };
            int need=static_cast<int>(j["run"].num("required"));
            Vec3 points[]= {
                {-12,0,-12},
                {12,0,-12},
                {0,0,-4}
            };
            for(int i=0;i<need;i++) {
                auto id="objective_"+std::to_string(i+1);
                j["objectives"].push(Json {
                    {
                        "id",id
                    },
                    {
                        "position",vec(points[i])
                    },
                    {
                        "done",r.interacted.contains(id)
                    }
                });
            }
        }
        j["lifts"]=Json::array();
        for(auto&[id,l]:lifts_) {
            auto pos=catalog_.regions.at(id).center;
            double k=l.moving?std::clamp((world_.now-l.start)/6,0.0,1.0):1;
            pos.y=l.from+(l.to-l.from)*k;
            pos.z+=110;
            j["lifts"].push(Json {
                {
                    "id",id
                },
                {
                    "position",vec(pos)
                },
                {
                    "moving",l.moving
                }
            });
        }
        j["content"]=contentSnapshot(p);
        auto& list=std::get<Json::Array>(j["monsters"].value);
        for(auto& v:list){
            auto& mob=world_.monsters.at(v.str("id"));
            if(mob.castResolve>world_.now){auto skill=contentEntry("skills",mob.castSkill);if(skill)v["cast"]=Json{{"id",mob.castSkill},{"name",skill->str("name")},{"shape",skill->str("shape")},{"position",vec(mob.castPoint)},{"origin",vec(mob.position)},{"resolveAt",mob.castResolve},{"radius",skill->str("shape")=="annulus"?20:skill->str("shape")=="cone"||skill->str("shape")=="melee_arc"?12:mob.boss?7:4},{"innerRadius",8},{"width",5}};}
            if(contentState_.flag("expanded")&&!contentAllowed(mob.position)) {v["hp"]=0;continue;}
            if(mob.definition=="dungeon_boss"&&world_.runs.contains(mob.scene)){auto d=contentEntry("monsters","aw_bestiary_dungeon_"+world_.runs.at(mob.scene).definition);if(d)v["name"]=d->str("name");}
            else if(mob.definition.empty()){unsigned hash=0;for(unsigned char ch:mob.id)hash=hash*31+ch;auto m=contentEntry("monsters","aw_species_"+mob.region+"_"+std::to_string(1+hash%5));if(m)v["name"]=m->str("name");}
        }
        return j;
    }
}
