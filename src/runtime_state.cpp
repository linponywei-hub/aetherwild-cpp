#include "aetherwild/runtime.hpp"
#include <fstream>
namespace aw {
    namespace {
        Json mats(const Materials&m) {
            Json j=Json::object();
            for(auto&[k,v]:m)j[k]=v;
            return j;
        }
        Materials unmats(const Json&j) {
            Materials m;
            for(auto&[k,v]:j.objectItems()) {
                require(v.integer()>=0,"負數材料");
                m[k]=v.integer();
            }
            return m;
        }
        Json strings(const std::set<Id>&s) {
            Json a=Json::array();
            for(auto&v:s)a.push(v);
            return a;
        }
        std::set<Id> unstrings(const Json&j) {
            std::set<Id> s;
            for(auto&v:j.arrayItems())s.insert(v.string());
            return s;
        }
    }
    Json vec(Vec3 v) {
        return Json {
            {
                "x",v.x
            },
            {
                "y",v.y
            },
            {
                "z",v.z
            }
        };
    }
    Vec3 unvec(const Json&j) {
        Vec3 v {
            j.num("x"),j.num("y"),j.num("z")
        };
        require(finite(v),"座標錯誤");
        return v;
    }
    Json itemJson(const ItemInstance&i) {
        return Json {
            {
                "id",i.id
            },
            {
                "code",i.definition
            },
            {
                "rarity",static_cast<int>(i.rarity)
            },
            {
                "required",i.requiredLevel
            },
            {
                "upgrade",i.upgrade
            }
        };
    }
    ItemInstance itemFromJson(const Json&j) {
        int r=static_cast<int>(j.at("rarity").integer());
        require(r>=0&&r<=6,"稀有度錯誤");
        return {
            j.str("id"),j.str("code"),static_cast<Rarity>(r),static_cast<int>(j.at("required").integer()),static_cast<int>(j.at("upgrade").integer())
        };
    }
    Json playerJson(const Player&p,bool priv) {
        Json j {
            {
                "id",p.id
            },
            {
                "name",p.name
            },
            {
                "profession",p.profession
            },
            {
                "level",p.level
            },
            {
                "position",vec(p.position)
            },
            {
                "scene",p.scene
            },
            {
                "hp",p.vitals.hp
            },
            {
                "maxHp",p.vitals.maxHp
            },
            {
                "mp",p.vitals.mp
            },
            {
                "maxMp",p.vitals.maxMp
            },
            {
                "stamina",p.vitals.stamina
            },
            {
                "maxStamina",p.vitals.maxStamina
            }
        };
        if(!priv)return j;
        j["account"]=p.accountId;
        j["xp"]=p.xp;
        j["coins"]=p.coins;
        j["bankCoins"]=p.bankCoins;
        j["lastCity"]=p.lastCity;
        j["materials"]=mats(p.materials);
        j["warehouse"]=mats(p.warehouse);
        j["tools"]=Json::object();
        for(auto&[k,v]:p.tools)j["tools"][k]=v;
        j["inventory"]=Json::array();
        for(auto&i:p.inventory)j["inventory"].push(itemJson(i));
        j["equipped"]=Json::object();
        for(auto&[k,v]:p.equipped)j["equipped"][std::to_string(static_cast<int>(k))]=v;
        j["jobs"]=strings(p.jobs);
        j["secrets"]=strings(p.secrets);
        j["skills"]=strings(p.learnedSkills);
        j["maps"]=strings(p.unlockedMaps);
        j["processed"]=strings(p.processedRewards);
        j["gatherXp"]=p.gatherXp;
        j["quests"]=Json::object();
        for(auto&[k,v]:p.quests)j["quests"][k]=Json {
            {
                "count",v.count
            },
            {
                "claimed",v.claimed
            }
        };
        j["cooldowns"]=Json::object();
        for(auto&[k,v]:p.skillReady)j["cooldowns"][k]=v;
        j["attackReady"]=p.attackReady;
        j["shield"]=p.shield;
        j["shieldUntil"]=p.shieldUntil;
        j["regenReady"]=p.regenReady;
        j["sprintLocked"]=p.sprintLocked;
        j["arrowHasteUntil"]=p.arrowHasteUntil;
        return j;
    }
    Player playerFromJson(const Json&j,const Catalog&c) {
        Player p;
        p.id=j.str("id");
        p.accountId=j.str("account");
        p.name=j.str("name");
        p.profession=j.str("profession");
        p.level=static_cast<int>(j.at("level").integer());
        require(c.classes.contains(p.profession)&&p.level>=1&&p.level<=100&&!p.id.empty(),"角色資料錯誤");
        p.position=unvec(j.at("position"));
        p.scene=j.str("scene");
        p.lastCity=j.str("lastCity");
        require(c.regions.contains(p.lastCity),"回城點錯誤");
        p.xp=j.at("xp").integer();
        p.coins=j.at("coins").integer();
        p.bankCoins=j.at("bankCoins").integer();
        require(p.xp>=0&&p.coins>=0&&p.bankCoins>=0,"角色財產錯誤");
        p.materials=unmats(j.at("materials"));
        p.warehouse=unmats(j.at("warehouse"));
        for(auto&[k,v]:j.at("tools").objectItems())p.tools[k]=static_cast<int>(v.integer());
        std::set<Id> ids;
        for(auto&v:j.at("inventory").arrayItems()) {
            auto i=itemFromJson(v);
            require(c.items.contains(i.definition)&&ids.insert(i.id).second&&i.requiredLevel>=1&&i.requiredLevel<=100&&i.upgrade>=0&&i.upgrade<=20,"裝備資料錯誤");
            p.inventory.push_back(i);
        }
        for(auto&[k,v]:j.at("equipped").objectItems()) {
            int slot=std::stoi(k);
            require(slot>=0&&slot<=4&&ids.contains(v.string()),"裝備參照錯誤");
            p.equipped[static_cast<Slot>(slot)]=v.string();
        }
        p.jobs=unstrings(j.at("jobs"));
        p.secrets=unstrings(j.at("secrets"));
        p.learnedSkills=unstrings(j.at("skills"));
        p.unlockedMaps=unstrings(j.at("maps"));
        p.processedRewards=unstrings(j.at("processed"));
        p.gatherXp=j.at("gatherXp").integer();
        for(auto&[k,v]:j.at("quests").objectItems()) {
            require(c.quests.contains(k),"未知任務");
            p.quests[k]= {
                static_cast<int>(v.at("count").integer()),v.flag("claimed")
            };
        }
        for(auto&[k,v]:j.at("cooldowns").objectItems())p.skillReady[k]=v.number();
        p.attackReady=j.num("attackReady");
        p.shield=j.num("shield");
        p.shieldUntil=j.num("shieldUntil");
        p.arrowHasteUntil=j.num("arrowHasteUntil");
        p.regenReady=j.num("regenReady");
        p.sprintLocked=j.flag("sprintLocked");
        ProgressionSystem::refresh(p,c);
        p.vitals.hp=j.num("hp");
        p.vitals.mp=j.num("mp");
        p.vitals.stamina=j.num("stamina");
        require(p.vitals.hp>=0&&p.vitals.hp<=p.vitals.maxHp&&p.vitals.mp>=0&&p.vitals.mp<=p.vitals.maxMp&&p.vitals.stamina>=0&&p.vitals.stamina<=p.vitals.maxStamina,"角色能力錯誤");
        return p;
    }
    Json GameRuntime::state()const {
        Json j {
            {
                "version",2
            },
            {
                "now",world_.now
            },
            {
                "sequence",world_.sequence
            },
            {
                "pvp",social_.pvpEnabled
            }
        };
        for(auto k: {
            "players","monsters","resources","runs","rewards","parties","profiles"
        })j[k]=Json::array();
        for(auto&[id,p]:world_.players)j["players"].push(playerJson(p));
        for(auto&[id,m]:world_.monsters) {
            Json v {
                {
                    "id",id
                },
                {
                    "definition",m.definition
                },
                {
                    "region",m.region
                },
                {
                    "scene",m.scene
                },
                {
                    "position",vec(m.position)
                },
                {
                    "home",vec(homes_.contains(id)?homes_.at(id):m.position)
                },
                {
                    "level",m.level
                },
                {
                    "hp",m.vitals.hp
                },
                {
                    "maxHp",m.vitals.maxHp
                },
                {
                    "attack",m.attack
                },
                {
                    "defense",m.defense
                },
                {
                    "boss",m.boss
                },
                {
                    "issued",m.rewardIssued
                },
                {
                    "nextAttack",m.nextAttack
                },
                {
                    "frozen",m.frozenUntil
                }
            };
            v["contribution"]=Json::object();
            for(auto&[p,n]:m.contribution)v["contribution"][p]=n;
            j["monsters"].push(v);
        }
        for(auto&[id,n]:world_.resources)j["resources"].push(Json {
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
                "scene",n.scene
            },
            {
                "readyAt",n.readyAt
            }
        });
        for(auto&[id,r]:world_.runs) {
            Json v {
                {
                    "id",id
                },
                {
                    "definition",r.definition
                },
                {
                    "player",r.player
                },
                {
                    "guards",r.guardsRemaining
                },
                {
                    "objectives",r.objectives
                },
                {
                    "interacted",strings(r.interacted)
                },
                {
                    "members",strings(r.members)
                },
                {
                    "chestClaimed",strings(r.chestClaimed)
                },
                {
                    "bossDead",r.bossDead
                },
                {
                    "claimed",r.claimed
                },
                {
                    "returnScene",r.returnScene
                },
                {
                    "returnPosition",vec(r.returnPosition)
                }
            };
            v["returnPositions"]=Json::object();
            for(auto&[p,pos]:r.returnPositions)v["returnPositions"][p]=vec(pos);
            j["runs"].push(v);
        }
        for(auto&[id,r]:world_.rewards) {
            Json v {
                {
                    "id",id
                },
                {
                    "player",r.player
                },
                {
                    "coins",r.coins
                },
                {
                    "xp",r.xp
                },
                {
                    "materials",mats(r.materials)
                }
            };
            v["items"]=Json::array();
            for(auto&i:r.items)v["items"].push(itemJson(i));
            j["rewards"].push(v);
        }
        for(auto&[id,p]:social_.parties)j["parties"].push(Json {
            {
                "id",id
            },
            {
                "leader",p.leader
            },
            {
                "members",strings(p.members)
            },
            {
                "invited",strings(p.invited)
            }
        });
        for(auto&[key,p]:profiles_)j["profiles"].push(Json {
            {
                "token",key
            },
            {
                "player",p.player
            },
            {
                "role",static_cast<int>(social_.accounts.at(world_.players.at(p.player).accountId).role)
            },
            {
                "receipts",strings(p.receipts)
            }
        });
        j["lifts"]=Json::object();
        for(auto&[id,l]:lifts_)j["lifts"][id]=Json {
            {
                "from",l.from
            },
            {
                "to",l.to
            },
            {
                "start",l.start
            },
            {
                "moving",l.moving
            }
        };
        j["rides"]=Json::object();
        for(auto&[id,m]:motion_)if(!m.lift.empty())j["rides"][id]=m.lift;
        j["features"]=Json::object();
        for(auto&[k,v]:features_)j["features"][k]=v;
        j["chat"]=chat_;
        return j;
    }
    void GameRuntime::restore(const Json&j) {
        require(j.num("version")==2,"工作站存檔版本不符");
        World w;
        SessionState s;
        std::map<Id,LocalProfile> profiles;
        std::map<Id,Vec3> homes;
        w.now=j.num("now");
        w.sequence=static_cast<std::uint64_t>(j.at("sequence").integer());
        for(auto&v:j.at("players").arrayItems()) {
            auto p=playerFromJson(v,catalog_);
            require(w.players.emplace(p.id,p).second,"重複角色");
        }
        for(auto&v:j.at("monsters").arrayItems()) {
            Monster m;
            m.id=v.str("id");
            m.definition=v.str("definition");
            m.region=v.str("region");
            m.scene=v.str("scene");
            m.position=unvec(v.at("position"));
            m.level=static_cast<int>(v.num("level"));
            m.vitals.hp=v.num("hp");
            m.vitals.maxHp=v.num("maxHp");
            m.attack=v.num("attack");
            m.defense=v.num("defense");
            m.boss=v.flag("boss");
            m.rewardIssued=v.flag("issued");
            m.nextAttack=v.num("nextAttack");
            m.frozenUntil=v.num("frozen");
            for(auto&[k,n]:v.at("contribution").objectItems())m.contribution[k]=n.number();
            w.monsters.emplace(m.id,m);
            homes[m.id]=unvec(v.at("home"));
        }
        for(auto&v:j.at("resources").arrayItems()) {
            ResourceNode n;
            n.id=v.str("id");
            n.definition=v.str("definition");
            require(catalog_.resources.contains(n.definition),"未知資源");
            n.position=unvec(v.at("position"));
            n.scene=v.str("scene");
            n.readyAt=v.num("readyAt");
            w.resources.emplace(n.id,n);
        }
        for(auto&v:j.at("runs").arrayItems()) {
            DungeonRun r;
            r.id=v.str("id");
            r.definition=v.str("definition");
            require(catalog_.dungeons.contains(r.definition),"未知副本");
            r.player=v.str("player");
            r.guardsRemaining=static_cast<int>(v.num("guards"));
            r.objectives=static_cast<int>(v.num("objectives"));
            r.interacted=unstrings(v.at("interacted"));
            r.members=unstrings(v.at("members"));
            r.chestClaimed=unstrings(v.at("chestClaimed"));
            r.bossDead=v.flag("bossDead");
            r.claimed=v.flag("claimed");
            r.returnScene=v.str("returnScene");
            r.returnPosition=unvec(v.at("returnPosition"));
            for(auto&[k,pos]:v.at("returnPositions").objectItems())r.returnPositions[k]=unvec(pos);
            w.runs.emplace(r.id,r);
        }
        for(auto&v:j.at("rewards").arrayItems()) {
            Reward r;
            r.id=v.str("id");
            r.player=v.str("player");
            require(w.players.contains(r.player),"獎勵角色不存在");
            r.coins=v.at("coins").integer();
            r.xp=v.at("xp").integer();
            r.materials=unmats(v.at("materials"));
            for(auto&i:v.at("items").arrayItems())r.items.push_back(itemFromJson(i));
            w.rewards.emplace(r.id,r);
        }
        for(auto&v:j.at("parties").arrayItems()) {
            Party p;
            p.id=v.str("id");
            p.leader=v.str("leader");
            p.members=unstrings(v.at("members"));
            p.invited=unstrings(v.at("invited"));
            for(auto&id:p.members) {
                require(w.players.contains(id)&&!s.memberParty.contains(id),"隊伍成員錯誤");
                s.memberParty[id]=p.id;
            }
            s.parties.emplace(p.id,p);
        }
        for(auto&v:j.at("profiles").arrayItems()) {
            LocalProfile p;
            p.player=v.str("player");
            require(w.players.contains(p.player),"登入角色不存在");
            p.receipts=unstrings(v.at("receipts"));
            auto role=static_cast<int>(v.num("role"));
            require(role>=0&&role<=2,"帳號權限錯誤");
            auto acc=w.players.at(p.player).accountId;
            s.accounts[acc]= {acc,static_cast<Role>(role)};
            profiles.emplace(v.str("token"),p);
        }
        for(auto&[id,p]:w.players)require(p.scene=="world"||w.runs.contains(p.scene),"角色副本不存在");
        s.pvpEnabled=j.flag("pvp");
        world_=std::move(w);
        social_=std::move(s);
        profiles_=std::move(profiles);
        homes_=std::move(homes);
        for(auto&[k,v]:j.at("features").objectItems())features_[k]=v.boolean();
        chat_=j.at("chat");
        if(j.contains("lifts"))for(auto&[id,v]:j.at("lifts").objectItems())lifts_[id]= {
            v.num("from"),v.num("to"),v.num("start"),v.flag("moving")
        };
        for(auto&[id,p]:world_.players) {
            auto&m=motion_[id];
            m.lastInput=-100;
            m.lastSeen=-100;
            m.peak=p.position.y;
            if(j.contains("rides")&&j.at("rides").contains(id))m.lift=j.at("rides").str(id);
            p.staminaUpdatedAt=world_.now;
        }
    }
    void GameRuntime::save() {
        if(savePath_.empty())return;
        std::filesystem::create_directories(savePath_.parent_path());
        auto temp=savePath_;
        temp+=".tmp";
        std::ofstream o(temp,std::ios::binary|std::ios::trunc);
        require(bool(o),"無法建立工作站存檔");
        o<<state().dump();
        o.flush();
        require(bool(o),"工作站存檔寫入失敗");
        o.close();
        auto bak=savePath_;
        bak+=".bak";
        std::error_code ec;
        if(std::filesystem::exists(savePath_)) {
            std::filesystem::copy_file(savePath_,bak,std::filesystem::copy_options::overwrite_existing,ec);
            require(!ec,"無法備份工作站存檔");
        }
        std::filesystem::rename(temp,savePath_,ec);
        if(ec) {
            std::filesystem::remove(savePath_,ec);
            std::filesystem::rename(temp,savePath_,ec);
        }
        require(!ec,"無法替換工作站存檔，舊資料仍在bak");
        savedAt_=world_.now;
    }
}
