#include "aetherwild/runtime.hpp"
#include <fstream>
#include <random>
#include <cstdlib>
#include <chrono>
namespace aw {
    namespace {
        constexpr double tau=6.283185307179586;
        bool sameCode(const std::string& supplied,const char* expected) {
            if(!expected||!*expected||supplied.size()>512)return false;
            std::string configured(expected);
            std::size_t difference=supplied.size()^configured.size();
            for(std::size_t i=0;i<std::max(supplied.size(),configured.size());++i)
                difference|=(i<supplied.size()?static_cast<unsigned char>(supplied[i]):0)^(i<configured.size()?static_cast<unsigned char>(configured[i]):0);
            return difference==0;
        }
        std::string tokenValue() {
            std::random_device r;
            std::ostringstream o;
            for(int i=0;i<8;i++)o<<std::hex<<std::setw(8)<<std::setfill('0')<<r();
            return o.str();
        }
        Json statsJson(Stats s) {
            return Json {
                {
                    "attack",s.attack
                },
                {
                    "hp",s.hp
                },
                {
                    "mp",s.mp
                },
                {
                    "defense",s.defense
                },
                {
                    "crit",s.crit
                },
                {
                    "speed",s.speed
                }
            };
        }
        Json costJson(const Materials&m) {
            Json j=Json::object();
            for(auto&[id,n]:m)j[id]=n;
            return j;
        }
        std::string roleName(Role r) {
            return r==Role::Owner?"owner":r==Role::Admin?"admin":"player";
        }
        std::string kindName(WeaponKind k) {
            return weaponName(k);
        }
    }
    GameRuntime::GameRuntime(std::filesystem::path path,bool legacyTestWorld):catalog_(makeCatalog()),buildings_(buildings(catalog_)),savePath_(std::move(path)) {
        configureContent(false);
        const char* publicMode=std::getenv("AETHERWILD_PUBLIC");
        if(publicMode&&std::string(publicMode)=="1") {
            const char* ownerCode=std::getenv("GAME_OWNER_CODE");
            const char* inviteCode=std::getenv("GAME_INVITE_CODE");
            require(ownerCode&&std::string(ownerCode).size()>=24&&inviteCode&&std::string(inviteCode).size()>=24,"雲端測試站需先設定足夠長度的擁有者驗證碼與邀請碼");
        }
        features_= {
            {
                "combat",true
            },
            {
                "gathering",true
            },
            {
                "crafting",true
            },
            {
                "dungeons",true
            },
            {
                "chat",true
            }
        };
        for(auto&[id,n]:catalog_.npcs) {
            auto&r=catalog_.regions.at(n.region);
            if(n.role=="resident")continue;
            n.position=cityArrival(r);
            if(n.role=="guild")n.position.z-=5;
            else if(n.role=="merchant")n.position.x+=5;
            else if(n.role=="smith")n.position.x-=5;
            else if(n.role=="healer")n.position.z+=5;
            else {
                n.position.x+=5;
                n.position.z+=5;
            }
        }
        lifts_["astral"]= {0,0,-100,false};
        if(!savePath_.empty()&&std::filesystem::exists(savePath_)) {
            std::ifstream in(savePath_,std::ios::binary);
            std::string text((std::istreambuf_iterator<char>(in)), {});
            restore(Json::parse(text));
        }
        else populate();
        if(!legacyTestWorld)applyWorldPatch();
    }
    Id GameRuntime::token() {
        Id v;
        do {
            v=tokenValue();
        }
        while(profiles_.contains(v));
        return v;
    }
    void GameRuntime::populate() {
        for(auto&[id,r]:catalog_.regions) {
            for(int i=0;i<5;i++) {
                double a=i*tau/5;
                Vec3 p {r.center.x+std::cos(a)*(r.radius+27+i*4),0,r.center.z+std::sin(a)*(r.radius+27+i*4)};
                auto mid=world_.nextId("wild");
                CombatSystem::spawn(world_,mid,p,id=="windchime"?1:r.level,id);
                homes_[mid]=p;
            }
            int index=0;
            for(auto&[code,d]:catalog_.resources) {
                double a=index++*2.399963;
                double radius=r.radius+12+(index%4)*6;
                Vec3 p {r.center.x+std::cos(a)*radius,0,r.center.z+std::sin(a)*radius};
                ResourceNode n;
                n.id="resource_"+id+"_"+code;
                n.definition=code;
                n.position=p;
                world_.resources.emplace(n.id,n);
            }
        }
        // Starter gathering trail has separate, real nodes; no automatic wood pickup.
        for(int i=0;i<10;i++) {
            ResourceNode n;
            n.id="starter_"+std::to_string(i);
            n.definition=i<5?"twig":"stone";
            n.position= {80.0+(i%5)*3,0,18.0+(i/5)*8};
            world_.resources.emplace(n.id,n);
        }
        for(auto&[id,b]:catalog_.bosses) {
            auto mid=world_.nextId("raid");
            CombatSystem::spawn(world_,mid,b.position,b.level,b.region,true,id);
            auto&m=world_.monsters.at(mid);
            m.vitals.hp=m.vitals.maxHp=b.hp;
            m.attack=b.attack;
            homes_[mid]=b.position;
        }
    }
    Json GameRuntime::createSession(const Json&request) {
        const char* ownerCode=std::getenv("GAME_OWNER_CODE");
        const char* inviteCode=std::getenv("GAME_INVITE_CODE");
        bool protectedMode=(ownerCode&&*ownerCode)||(inviteCode&&*inviteCode);
        bool verifiedOwner=sameCode(request.str("ownerCode"),ownerCode);
        if(protectedMode)require(verifiedOwner||sameCode(request.str("inviteCode"),inviteCode),"邀請碼或擁有者驗證碼不正確");
        auto name=request.str("name","旅人");
        auto job=request.str("profession","warrior");
        require(!name.empty()&&name.size()<=60,"角色名稱需1至20個中文字");
        for(unsigned char ch:name)require(ch>=32,"名稱含控制字元");
        require(catalog_.classes.contains(job)&&catalog_.classes.at(job).level==1&&catalog_.classes.at(job).parent.empty(),"請選擇基礎職業");
        require(profiles_.size()<100,"此本機工作站角色數已達上限");
        auto id=world_.nextId("player");
        Player p;
        p.id=id;
        p.accountId="workstation:"+id;
        p.name=name;
        p.profession=job;
        p.jobs= {job};
        ProgressionSystem::refresh(p,catalog_);
        p.vitals.hp=p.vitals.maxHp;
        p.vitals.mp=p.vitals.maxMp;
        auto type=catalog_.classes.at(job).weapon;
        auto code=startingWeapon(catalog_,job);
        p.inventory.push_back(InventorySystem::create(world_,catalog_,code));
        InventorySystem::equip(p,p.inventory.front().id,catalog_);
        p.materials["potion"]=3;
        if(type==WeaponKind::Bow)p.materials["woodarrow"]=40;
        if(contentState_.flag("expanded"))p.position=cityArrival(catalog_.regions.at("windchime"));
        p.coins=60;
        world_.players.emplace(id,p);
        auto key=token();
        Role role=protectedMode?(verifiedOwner?Role::Owner:Role::Player):(profiles_.empty()?Role::Owner:Role::Player);
        social_.accounts[p.accountId]= {p.accountId,role};
        profiles_[key]= {
            id, {}
        };
        motion_[id].lastSeen=world_.now;
        motion_[id].lastInput=-100;
        save();
        return Json {
            {
                "ok",true
            },
            {
                "token",key
            },
            {
                "player",playerJson(p)
            },
            {
                "role",roleName(role)
            }
        };
    }
    Json GameRuntime::catalog()const {
        Json j;
        for(auto k: {
            "regions","items","resources","npcs","dungeons","quests","recipes","classes","skills","buildings"
        })j[k]=Json::array();
        for(auto&[id,r]:catalog_.regions)j["regions"].push(Json {
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
                if(!catalog_.classes.at(other.profession).secret.empty())pub["profession"]="unknown";
                j["online"].push(pub);
                if(id!=p.id&&other.scene==p.scene)j["players"].push(pub);
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
    void GameRuntime::input(const Id&key,const Json&j) {
        require(profiles_.contains(key),"登入已失效");
        auto&id=profiles_.at(key).player;
        auto&m=motion_[id];
        auto seq=j.at("sequence").integer();
        require(seq>=0,"輸入序號錯誤");
        if(static_cast<std::uint64_t>(seq)<=m.sequence)return;
        double x=j.num("x"),z=j.num("z"),yaw=j.num("yaw");
        require(std::isfinite(x)&&std::isfinite(z)&&std::isfinite(yaw)&&std::abs(x)<=1&&std::abs(z)<=1&&std::abs(yaw)<1e8,"移動輸入錯誤");
        m.sequence=static_cast<std::uint64_t>(seq);
        double len=std::max(1.0,std::hypot(x,z));
        m.x=x/len;
        m.z=z/len;
        m.yaw=yaw;
        m.sprint=j.flag("sprint");
        m.jump=m.jump||j.flag("jump");
        m.lastInput=m.lastSeen=world_.now;
    }
    void GameRuntime::settleRewards() {
        for(auto&[id,r]:world_.rewards)if(world_.players.contains(r.player)) {
            auto&p=world_.players.at(r.player);
            if(!p.processedRewards.contains(id))RewardSystem::claim(world_,p,id,catalog_);
        }
    }
    void GameRuntime::physics(Player&p,Motion&m,double dt) {
        if(!p.vitals.alive())return;
        if(!m.lift.empty()) {
            auto&l=lifts_.at(m.lift);
            double k=std::clamp((world_.now-l.start)/6,0.0,1.0);
            p.position.y=l.from+(l.to-l.from)*k;
            m.velocityY=0;
            m.grounded=true;
            if(k>=1)m.lift.clear();
            return;
        }
        bool active=world_.now-m.lastInput<.45;
        double speed=5+InventorySystem::bonuses(p,catalog_).speed;
        bool sprint=active&&m.sprint&&!p.sprintLocked&&p.vitals.stamina>=18*dt;
        double vx=active?(m.x*std::cos(m.yaw)-m.z*std::sin(m.yaw)):0;
        double vz=active?(-m.x*std::sin(m.yaw)-m.z*std::cos(m.yaw)):0;
        double scale=speed*(sprint?1.6:1)*dt;
        Vec3 next=p.position;
        auto obstacle=[&](Vec3 v) {
            return p.scene=="world"?(!contentAllowed(v)||blocked(v,buildings_)):std::abs(v.x)>24||std::abs(v.z)>29;
        };
        next.x=std::clamp(next.x+vx*scale,-catalog_.worldLimit+10,catalog_.worldLimit-10);
        if(obstacle(next))next.x=p.position.x;
        next.z=std::clamp(next.z+vz*scale,-catalog_.worldLimit+10,catalog_.worldLimit-10);
        if(obstacle(next))next.z=p.position.z;
        double ground=p.scene=="world"?groundBelow(next,catalog_):0;
        bool walking=std::hypot(vx,vz)>.01;
        if(m.jump&&m.grounded) {
            m.velocityY=8;
            m.grounded=false;
            m.airTime=0;
            m.peak=p.position.y;
        }
        m.jump=false;
        if(m.grounded&&m.velocityY<=0&&p.position.y-ground<=1.4&&p.position.y-ground>=-1.4) {
            next.y=ground;
        }
        else {
            if(m.grounded) {
                m.airTime=0;
                m.peak=p.position.y;
            }
            m.grounded=false;
            m.airTime+=dt;
            m.peak=std::max(m.peak,p.position.y);
            m.velocityY-=22*dt;
            next.y=p.position.y+m.velocityY*dt;
            if(next.y<=ground) {
                next.y=ground;
                auto damage=MovementSystem::land(p,m.peak-ground,m.airTime);
                if(damage>0)world_.events.push_back( {
                    "FallDamage",p.id,p.id,damage, {}
                });
                m.grounded=true;
                m.velocityY=0;
                m.airTime=0;
            }
        }
        p.position=next;
        MovementSystem::tickStamina(p,dt,sprint&&walking,world_.now);
        if(p.scene=="world")for(auto&[id,r]:catalog_.regions) {
            if(onCityFloor(next,r)&&!r.wilderness)p.lastCity=id;
            if(distance(next,r.center)<r.radius)QuestSystem::record(p,"visit",id,catalog_);
        }
    }
    void GameRuntime::ai(double dt) {
        std::vector<Id> remove;
        for(auto&[id,m]:world_.monsters) {
            if(!m.vitals.alive()) {
                if(!deadAt_.contains(id))deadAt_[id]=world_.now;
                if(m.scene=="world"&&world_.now-deadAt_[id]>(m.boss?120:45)) {
                    remove.push_back(id);
                }
                continue;
            }
            if(contentState_.flag("expanded")&&m.scene=="world"&&!contentAllowed(m.position))continue;
            if(!features_["combat"]||world_.now<m.frozenUntil){m.castResolve=0;m.castSkill.clear();continue;}
            Player* target=nullptr;
            double best=m.boss?28:20;
            for(auto&[pid,p]:world_.players)if(p.scene==m.scene&&p.vitals.alive()&&world_.now-motion_[pid].lastSeen<8&&!MovementSystem::safe(p.position,p.scene,catalog_)) {
                double d=distance(p.position,m.position);
                if(d<best&&(p.scene!="world"||clearSight(m.position,p.position,buildings_))) {
                    target=&p;
                    best=d;
                }
            }
            if(world_.now<m.tauntUntil&&world_.players.contains(m.tauntedBy)){
                auto& t=world_.players.at(m.tauntedBy);if(t.vitals.alive()&&t.scene==m.scene&&!MovementSystem::safe(t.position,t.scene,catalog_)&&distance(t.position,m.position)<40){target=&t;best=distance(t.position,m.position);}
            }
            if(m.definition=="dungeon_boss"&&world_.runs.contains(m.scene)) {
                auto&r=world_.runs.at(m.scene);
                auto&d=catalog_.dungeons.at(r.definition);
                int need=d.objective=="hunt"?0:d.objective=="seals"||d.objective=="braziers"?3:1;
                if(r.guardsRemaining>0||r.objectives<need)target=nullptr;
            }
            if(monsterAbility(m,target))continue;
            Vec3 home=homes_.contains(id)?homes_.at(id):m.position;
            bool leash=distance(m.position,home)>65;
            Vec3 goal=target&&!leash?target->position:home;
            double d=distance(goal,m.position);
            if(d>2.8) {
                double speed=m.boss?2.5:3.1;
                Vec3 n=m.position;
                n.x+=(goal.x-n.x)/d*speed*dt;
                n.z+=(goal.z-n.z)/d*speed*dt;
                if(m.scene!="world"||(!blocked(n,buildings_,.6)&&contentAllowed(n))){if(m.scene=="world")n.y=worldLandHeight(n,catalog_);m.position=n;}
            }
            if(target&&!leash&&best<=3.4&&world_.now>=m.nextAttack)try {
                CombatSystem::monsterAttack(world_,catalog_,id,target->id);
            }
            catch(const RuleError&) {}
        }
        for(auto&id:remove) {
            auto old=world_.monsters.at(id);
            auto home=homes_.at(id);
            auto newId=world_.nextId(old.boss?"raid":"wild");
            CombatSystem::spawn(world_,newId,home,old.level,old.region,old.boss,old.definition);
            if(old.boss&&catalog_.bosses.contains(old.definition)) {
                auto&b=catalog_.bosses.at(old.definition);
                world_.monsters.at(newId).vitals.hp=world_.monsters.at(newId).vitals.maxHp=b.hp;
                world_.monsters.at(newId).attack=b.attack;
            }
            homes_[newId]=home;
            world_.monsters.erase(id);
            homes_.erase(id);
            deadAt_.erase(id);
        }
    }
    void GameRuntime::tick(double dt) {
        require(std::isfinite(dt)&&dt>=0&&dt<=.25,"工作站tick時間錯誤");
        world_.now+=dt;
        for(auto&[id,p]:world_.players) {
            auto&m=motion_[id];
            if(world_.now-m.lastSeen<8)physics(p,m,dt);
            if(p.vitals.alive()&&MovementSystem::safe(p.position,p.scene,catalog_))p.vitals.restore(dt*2,dt*3);
        }
        for(auto&[id,l]:lifts_)if(l.moving&&world_.now-l.start>=6) {
            l.moving=false;
            l.from=l.to;
        }
        for(auto&[id,n]:world_.resources)if(n.reservedBy&&world_.now>=n.completeAt) {
            if(world_.players.contains(*n.reservedBy))try {
                GatheringSystem::finish(world_,world_.players.at(*n.reservedBy),id,catalog_);
            }
            catch(const RuleError&) {
                n.reservedBy.reset();
            }
            else n.reservedBy.reset();
        }
        contentTick(dt);
        ai(dt);
        settleRewards();
        if(world_.events.size()>2000)world_.events.erase(world_.events.begin(),world_.events.begin()+1000);
        if(world_.now-savedAt_>=10)save();
    }
    Json GameRuntime::command(const Id&key,const Json&j) {
        require(profiles_.contains(key),"登入已失效");
        auto&profile=profiles_.at(key);
        auto request=j.str("request");
        require(!request.empty()&&request.size()<=100,"操作缺少有效請求ID");
        motion_[profile.player].lastSeen=world_.now;
        if(profile.receipts.contains(request))return snapshot(key);
        auto before=state();
        auto motions=motion_;
        auto lifts=lifts_;
        auto events=world_.events;
        try {
            execute(world_.players.at(profile.player),j);
            settleRewards();
            auto&receipts=profiles_.at(key).receipts;
            receipts.insert(request);
            if(receipts.size()>512)receipts.erase(receipts.begin());
            save();
            return snapshot(key);
        }
        catch(...) {
            restore(before);
            motion_=std::move(motions);
            lifts_=std::move(lifts);
            world_.events=std::move(events);
            throw;
        }
    }
    void GameRuntime::execute(Player&p,const Json&j) {
        auto type=j.str("type"),target=j.str("target"),npc=j.str("npc");
        if(contentCommand(p,j))return;
        if(type=="travel"||type=="admin_travel")checkContentRegion(target);
        if((type=="enter"||type=="admin_entrance")&&catalog_.dungeons.contains(target))checkContentRegion(catalog_.dungeons.at(target).region);
        auto&motion=motion_[p.id];
        auto admin=[&] {
            auto role=social_.accounts.at(p.accountId).role;
            require(role==Role::Admin||role==Role::Owner,"需要管理員權限");
        };
        auto nearService=[&](const std::string&role) {
            for(auto&[id,n]:catalog_.npcs)if(n.role==role&&p.scene=="world"&&distance(p.position,n.position)<=8&&contentRegionOpen(catalog_,n.region))return id;
            throw RuleError("請先走到對應NPC附近");
        };
        auto freshMotion=[&] {
            motion.velocityY=0;
            motion.grounded=true;
            motion.airTime=0;
            motion.peak=p.position.y;
            motion.x=motion.z=0;
            motion.lift.clear();motion.bowDrawAt=-1;
        };
        bool releasingBow=type=="bow_release";double bowPower=1;
        auto validDraw=[&]{return motion.bowDrawAt>=0&&motion.bowScene==p.scene&&motion.bowClass==p.profession&&world_.now-motion.bowDrawAt<=12;};
        if(type=="bow_cancel"){motion.bowDrawAt=-1;return;}
        if(type=="bow_draw"){
            require(features_["combat"]&&p.vitals.alive()&&catalog_.classes.at(p.profession).weapon==WeaponKind::Bow,"目前無法拉弓");
            require(world_.now>=p.attackReady,"射擊冷卻中");
            if(!validDraw()){motion.bowDrawAt=world_.now;motion.bowScene=p.scene;motion.bowClass=p.profession;}
            return;
        }
        if(releasingBow){
            require(p.vitals.alive()&&catalog_.classes.at(p.profession).weapon==WeaponKind::Bow&&validDraw(),"請先按住拉弓，切換場景後需重新拉弓");
            bowPower=.55+1.05*std::clamp((world_.now-motion.bowDrawAt)/1.2,0.0,1.0);type="attack";
        }
        if(type=="attack"||type=="weapon_skill"||type=="skill") {
            if(type=="attack"&&catalog_.classes.at(p.profession).weapon==WeaponKind::Bow)require(releasingBow,"弓箭需按住拉弓後放開射擊");
            require(features_["combat"],"管理員暫停了戰鬥");
            require(world_.now-motion.lastAction>=.08,"操作太快");
            if(target.empty()) {
                double best=catalog_.classes.at(p.profession).range;
                for(auto&[id,m]:world_.monsters)if(m.scene==p.scene&&m.vitals.alive()) {
                    double dx=m.position.x-p.position.x,dz=m.position.z-p.position.z,d=distance(p.position,m.position);
                    double dot=(-std::sin(motion.yaw)*dx-std::cos(motion.yaw)*dz)/std::max(.01,std::hypot(dx,dz));
                    if(d<=best&&dot>.35) {
                        best=d;
                        target=id;
                    }
                }
            }
            if(world_.monsters.contains(target)) {
                auto&m=world_.monsters.at(target);
                double dx=m.position.x-p.position.x,dz=m.position.z-p.position.z;
                double dot=(-std::sin(motion.yaw)*dx-std::cos(motion.yaw)*dz)/std::max(.01,std::hypot(dx,dz));
                require(dot>.2||distance(p.position,m.position)<2,"目標不在前方");
                require(p.scene!="world"||clearSight(p.position,m.position,buildings_),"目標被建築物遮擋");
            }
            if(type=="attack") {
                if(releasingBow&&target.empty()){
                    require(!MovementSystem::safe(p.position,p.scene,catalog_)&&world_.now>=p.attackReady,"安全區或射擊冷卻中");
                    const auto arrow=j.str("arrow","woodarrow");require(catalog_.arrowBonus.contains(arrow),"未知箭矢");
                    InventorySystem::pay(p.materials,{{arrow,1}});p.attackReady=world_.now+.8;
                }
                else if(world_.players.contains(target))PvpSystem::attack(world_,catalog_,social_,p.id,target,bowPower);
                else {
                    require(world_.monsters.contains(target),"瞄準前方怪物再攻擊");
                    CombatSystem::attack(world_,catalog_,p.id,target,j.str("arrow","woodarrow"),bowPower);
                }
            }
            else if(type=="weapon_skill")CombatSystem::weaponSkill(world_,catalog_,p.id,target);
            else CombatSystem::cast(world_,catalog_,p.id,j.str("skill"),target);
            if(releasingBow){motion.bowDrawAt=-1;world_.events.push_back({"ArrowReleased",p.id,target,bowPower,"蓄力射箭"});}
            else if(type!="attack")motion.bowDrawAt=-1;
            motion.lastAction=world_.now;
        }
        else if(type=="gather") {
            require(features_["gathering"],"採集已暫停");
            GatheringSystem::begin(world_,p,target,catalog_);
        }
        else if(type=="cancel_gather")GatheringSystem::cancel(world_,p,target);
        else if(type=="craft") {
            require(features_["crafting"],"製作已暫停");
            CraftingSystem::craft(world_,p,target,npc.empty()&&catalog_.recipes.at(target).station=="smith"?nearService("smith"):npc,catalog_);
        }
        else if(type=="equip")InventorySystem::equip(p,target,catalog_);
        else if(type=="unequip")InventorySystem::unequip(p,target,catalog_);
        else if(type=="potion")InventorySystem::usePotion(p,target.empty()?"potion":target,catalog_);
        else if(type=="accept")QuestSystem::accept(p,target,npc.empty()?catalog_.quests.at(target).npc:npc,catalog_);
        else if(type=="quest"){
            auto giver=npc.empty()?catalog_.quests.at(target).npc:npc;bool done=p.quests.contains(target)&&p.quests.at(target).claimed;
            QuestSystem::claim(p,target,giver,catalog_);
            auto prof=contentEntry("profiles",giver);if(!done&&prof){auto& rel=contentState_["relations"][p.id];if(!rel.contains(giver))rel[giver]=NpcSystem::initial();NpcSystem::event(rel[giver],*prof,"quest","quest:"+target,NpcSystem::clock());}
        }
        else if(type=="job"){ProgressionSystem::changeClass(p,target,npc.empty()?nearService("guild"):npc,catalog_);motion.bowDrawAt=-1;}
        else if(type=="buy"){
            auto seller=npc.empty()?nearService("merchant"):npc;auto before=p.coins;
            EconomySystem::buy(world_,p,target,j.contains("count")?j.at("count").integer():1,seller,catalog_);
            auto prof=contentEntry("profiles",seller);if(prof){auto& rel=contentState_["relations"][p.id];if(!rel.contains(seller))rel[seller]=NpcSystem::initial();
                auto view=NpcSystem::view(rel[seller],*prof,NpcSystem::clock());auto spent=before-p.coins;auto discount=static_cast<Quantity>(std::floor(spent*view.num("discount")/100));p.coins+=discount;
                NpcSystem::event(rel[seller],*prof,"purchase",j.str("request"),NpcSystem::clock(),static_cast<int>(std::min<Quantity>(200,spent-discount)));}
        }
        else if(type=="store_item"||type=="withdraw_item")InventorySystem::storeItem(p,target,npc.empty()?nearService("merchant"):npc,catalog_,type=="withdraw_item");
        else if(type=="store"||type=="withdraw")InventorySystem::store(p,target,j.contains("count")?j.at("count").integer():1,npc.empty()?nearService("merchant"):npc,catalog_,type=="withdraw");
        else if(type=="heal") {
            nearService("healer");
            require(p.coins>=15,"治療需要15金幣");
            p.coins-=15;
            p.vitals.restore(p.vitals.maxHp,p.vitals.maxMp);
        }
        else if(type=="map") {
            auto id=nearService("cartographer");
            auto&n=catalog_.npcs.at(id);
            require(!p.unlockedMaps.contains(n.region),"已購買此地圖");
            auto price=35+catalog_.regions.at(n.region).level*3;
            require(p.coins>=price,"地圖金幣不足");
            p.coins-=price;
            p.unlockedMaps.insert(n.region);
        }
        else if(type=="travel") {
            nearService("guild");
            require(catalog_.regions.contains(target)&&!catalog_.regions.at(target).wilderness,"未知目的地");
            require(p.coins>=25,"驛車需要25金幣");
            p.coins-=25;
            p.position=cityArrival(catalog_.regions.at(target));
            p.lastCity=target;
            freshMotion();
        }
        else if(type=="enter") {
            require(features_["dungeons"],"副本已暫停");
            auto before=p.position;
            auto runId=DungeonSystem::enter(world_,p,target,catalog_);
            auto&run=world_.runs.at(runId);
            p.position= {0,0,22};
            if(social_.memberParty.contains(p.id)) {
                auto&party=social_.parties.at(social_.memberParty.at(p.id));
                for(auto&id:party.members)if(id!=p.id) {
                    auto&other=world_.players.at(id);
                    if(other.scene=="world"&&other.vitals.alive()&&other.level>=catalog_.dungeons.at(target).level&&distance(other.position,before)<=8) {
                        run.members.insert(id);
                        run.returnPositions[id]=other.position;
                        other.scene=runId;
                        other.position= {2,0,22};
                        motion_[id].velocityY=0;
                    }
                }
            }
            int i=0;
            for(auto&[id,m]:world_.monsters)if(m.scene==runId) {
                if(m.definition=="dungeon_boss")m.position= {0,0,-21};
                else {
                    double a=(i+++.5)*tau/std::max(1,run.guardsRemaining);
                    m.position= {std::cos(a)*12,0,std::sin(a)*10};
                }
                homes_[id]=m.position;
            }
            freshMotion();
        }
        else if(type=="objective") {
            require(world_.runs.contains(p.scene),"不在副本");
            int n=target=="objective_1"?0:target=="objective_2"?1:target=="objective_3"?2:-1;
            require(n>=0,"機關不存在");
            Vec3 positions[]= {
                {-12,0,-12},
                {12,0,-12},
                {0,0,-4}
            };
            require(distance(p.position,positions[n])<=4,"請走到機關旁");
            DungeonSystem::interactObjective(world_,p,target,catalog_);
        }
        else if(type=="chest") {
            require(distance(p.position, {0,0,-25})<=4,"請走到寶箱旁");
            DungeonSystem::openChest(world_,p,catalog_);
        }
        else if(type=="leave") {
            require(distance(p.position, {0,0,22})<=5||!p.vitals.alive(),"請走到返回傳送門");
            DungeonSystem::leave(world_,p);
            freshMotion();
        }
        else if(type=="respawn") {
            require(!p.vitals.alive(),"角色尚未死亡");
            if(world_.runs.contains(p.scene))DungeonSystem::leave(world_,p);
            CombatSystem::respawn(p,catalog_);
            p.position=cityArrival(catalog_.regions.at(p.lastCity));
            freshMotion();
        }
        else if(type=="lift") {
            require(p.scene=="world"&&p.vitals.alive(),"無法使用升降台");
            auto base=catalog_.regions.at("astral").center;
            base.z+=110;
            auto&l=lifts_.at("astral");
            require(!l.moving,"升降台正在移動");
            require(std::hypot(p.position.x-base.x,p.position.z-base.z)<=6&&(std::abs(p.position.y)<3||std::abs(p.position.y-62)<3),"請走到升降台上下站");
            double station=p.position.y>30?62:0;
            if(std::abs(station-l.to)>3) {
                l.from=l.to;
                l.to=station;
                l.start=world_.now;
                l.moving=true;
            }
            else {
                base.y=l.to;
                l.from=l.to;
                l.to=l.to<30?62:0;
                l.start=world_.now;
                l.moving=true;
                for(auto&[id,other]:world_.players)if(other.scene=="world"&&distance(other.position,base)<=6) {
                    motion_[id].lift="astral";
                    other.position=base;
                }
            }
        }
        else if(type=="party_create")PartySystem::create(world_,social_,p.id);
        else if(type=="party_invite")PartySystem::invite(world_,social_,p.id,target);
        else if(type=="party_join")PartySystem::join(world_,social_,p.id,target);
        else if(type=="party_leave")PartySystem::leave(social_,p.id);
        else if(type=="chat") {
            require(features_["chat"],"聊天已暫停");
            auto text=j.str("text");
            require(!text.empty()&&text.size()<=600,"訊息長度錯誤");
            chat_.push(Json {
                {
                    "player",p.id
                },
                {
                    "name",p.name
                },
                {
                    "text",text
                },
                {
                    "time",world_.now
                }
            });
            auto&a=std::get<Json::Array>(chat_.value);
            if(a.size()>60)a.erase(a.begin());
        }
        else if(type=="admin_pvp")AdministrationSystem::setPvp(world_,social_,p.accountId,j.flag("enabled"));
        else if(type=="admin_grant") {
            require(world_.players.contains(target),"玩家不存在");
            AdministrationSystem::grantAdmin(world_,social_,p.accountId,world_.players.at(target).accountId);
        }
        else if(type=="admin_travel") {
            admin();
            AdministrationSystem::teleportToCity(world_,catalog_,social_,p.accountId,p.id,target);
            p.position=cityArrival(catalog_.regions.at(target));
            freshMotion();
        }
        else if(type=="admin_entrance") {
            admin();
            require(catalog_.dungeons.contains(target)&&p.scene=="world","副本不存在或需先離開目前副本");
            p.position=catalog_.dungeons.at(target).entrancePosition;
            freshMotion();
        }
        else if(type=="admin_feature") {
            admin();
            require(features_.contains(target),"未知功能");
            features_[target]=j.flag("enabled");
            world_.events.push_back( {
                "AdminFeature",p.accountId,target,j.flag("enabled")?1.0:0.0, {}
            });
        }
        else if(type=="admin_supply") {
            admin();
            require(p.scene=="world","請先離開副本");
            p.level=60;
            p.coins=10000;
            p.gatherXp=1200;
            for(auto&[id,r]:catalog_.resources)p.materials[id]=100;
            for(auto&id: {
                "axe","pick","sickle"
            })p.tools[id]=5;
            for(auto&[id,s]:catalog_.skills)if(s.effect!="dash")p.learnedSkills.insert(id);
            p.materials["woodarrow"]=300;
            p.materials["potion"]=30;
            ProgressionSystem::refresh(p,catalog_);
            p.vitals.hp=p.vitals.maxHp;
            p.vitals.mp=p.vitals.maxMp;
            world_.events.push_back( {
                "AdminSupply",p.accountId,p.id,0,"僅供工作站測試"
            });
        }
        else if(type=="save") {}
        else throw RuleError("不支援的操作："+type);
    }
}

