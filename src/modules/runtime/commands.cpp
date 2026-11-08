#include "aetherwild/talents.hpp"
#include "aetherwild/runtime.hpp"
#include <fstream>
#include <random>
#include <cstdlib>
#include <chrono>
#include "support.hpp"
namespace aw {
using namespace runtime_detail;
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
        if(type=="learn_talent"){learnTalent(p,j.str("target"),catalog_);return;}
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
        else if(type=="enhance") {
            require(features_["crafting"],"製作與強化已暫停");
            auto smith=npc.empty()?nearService("smith"):npc;auto n=catalog_.npcs.find(smith);require(n!=catalog_.npcs.end()&&clearSight(p.position,n->second.position,buildings_),"鐵匠被建築物遮擋");
            const auto level=j.at("expected").integer();require(level>=0&&level<=20,"強化報價錯誤");
            static std::mt19937 engine(std::random_device{}());
            EnhancementSystem::enhance(world_,p,target,smith,static_cast<int>(level),catalog_,std::generate_canonical<double,32>(engine));
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
    }}
