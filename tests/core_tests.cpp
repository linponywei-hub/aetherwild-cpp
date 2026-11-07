#include "aetherwild/persistence.hpp"
#include "aetherwild/social.hpp"
#include "aetherwild/ports.hpp"
#include <fstream>
#include <iostream>
#include <limits>
using namespace aw;
namespace {
    int checks=0,groups=0;
    void check(bool v,const char*expr) {
        ++checks;
        if(!v)throw std::runtime_error(expr);
    }
#define CHECK(x) check(bool(x),#x)
    template<class F>void rejected(F f) {
        ++checks;
        try {
            f();
        }
        catch(const RuleError&) {
            return;
        }
        throw std::runtime_error("expected RuleError");
    }
    template<class F>void group(const char*name,F f) {
        f();
        ++groups;
        std::cout<<"PASS "<<name<<'\n';
    }
    void wild(Player&p) {
        p.position= {100,0,24};
    }
    void kill(Simulation&s,Player&p,const Id&id) {
        auto&w=s.world();
        while(w.monsters.at(id).vitals.alive()) {
            CombatSystem::attack(w,s.catalog(),p.id,id);
            s.advance(1);
        }
    }
}
int main() {
    try {
        group("catalog integrity and preserved IDs",[] {
            auto c=makeCatalog();
            CHECK(c.items.size()==84);
            CHECK(c.classes.size()==15);
            CHECK(c.dungeons.size()==55);
            CHECK(c.bosses.size()==8);
            CHECK(c.quests.size()==63);
            CHECK(c.regions.size()==8);
            CHECK(c.recipes.size()==40);
            CHECK(c.items.at("cindergreatsword").weapon==WeaponKind::Sword);
            CHECK(c.dungeons.at("ruins").entrance=="hut");
        });
        group("entity damage invariants",[] {
            Vitals v;
            CHECK(v.receiveDamage(500)==100);
            CHECK(!v.alive());
            CHECK(v.receiveDamage(1)==0);
            rejected([&] {
                v.receiveDamage(-1);
            });
            rejected([&] {
                v.receiveDamage(std::numeric_limits<double>::quiet_NaN());
            });
            v.restore(200,100);
            CHECK(v.hp==v.maxHp);
            CHECK(v.mp==v.maxMp);
        });
        group("player and weapon ownership",[] {
            Simulation s;
            auto&p=s.addPlayer("p","角色");
            auto&c=s.catalog();
            CHECK(p.vitals.hp==140);
            CHECK(p.inventory.size()==1);
            rejected([&] {
                s.addPlayer("p","重複");
            });
            rejected([&] {
                s.addPlayer("q","角色","archmage");
            });
            auto item=InventorySystem::create(s.world(),c,"oakstaff");
            p.inventory.push_back(item);
            rejected([&] {
                InventorySystem::equip(p,item.id,c);
            });
            CHECK(p.equipped.at(Slot::Weapon)==p.inventory.front().id);
            auto high=InventorySystem::create(s.world(),c,"cindergreatsword");
            p.inventory.push_back(high);
            rejected([&] {
                InventorySystem::equip(p,high.id,c);
            });
        });
        group("attack range cooldown scene safe zone and duplicate death",[] {
            Simulation s;
            auto&p=s.addPlayer("p","角色");
            auto&w=s.world();
            auto&c=s.catalog();
            CombatSystem::spawn(w,"m", {100,0,24},
            1,"windchime");
            rejected([&] {
                CombatSystem::attack(w,c,"p","m");
            });
            wild(p);
            w.monsters.at("m").scene="other";
            rejected([&] {
                CombatSystem::attack(w,c,"p","m");
            });
            w.monsters.at("m").scene="world";
            auto hit=CombatSystem::attack(w,c,"p","m");
            CHECK(hit.damage==33);
            rejected([&] {
                CombatSystem::attack(w,c,"p","m");
            });
            s.advance(1);
            kill(s,p,"m");
            CHECK(!w.monsters.at("m").vitals.alive());
            CHECK(w.rewards.size()==1);
            RewardSystem::onDeath(w,w.monsters.at("m"),c);
            CHECK(w.rewards.size()==1);
            rejected([&] {
                CombatSystem::attack(w,c,"p","m");
            });
        });
        group("reward ownership exactly once",[] {
            Simulation s;
            auto&p=s.addPlayer("p","角色");
            auto&q=s.addPlayer("q","角色");
            auto&w=s.world();
            wild(p);
            CombatSystem::spawn(w,"m",p.position,1,"windchime");
            kill(s,p,"m");
            auto id=w.rewards.begin()->first;
            rejected([&] {
                RewardSystem::claim(w,q,id,s.catalog());
            });
            CHECK(RewardSystem::claim(w,p,id,s.catalog()));
            auto coins=p.coins;
            CHECK(!RewardSystem::claim(w,p,id,s.catalog()));
            CHECK(coins==p.coins);
        });
        group("bows consume arrows and magic consumes mana",[] {
            Simulation s;
            auto&p=s.addPlayer("p","弓手","ranger");
            wild(p);
            CombatSystem::spawn(s.world(),"m",p.position,3,"windchime");
            auto n=p.materials.at("woodarrow");
            CombatSystem::attack(s.world(),s.catalog(),p.id,"m");
            CHECK(p.materials.at("woodarrow")==n-1);
            s.advance(1);
            p.materials["woodarrow"]=0;
            auto hp=s.world().monsters.at("m").vitals.hp;
            rejected([&] {
                CombatSystem::attack(s.world(),s.catalog(),p.id,"m");
            });
            CHECK(hp==s.world().monsters.at("m").vitals.hp);
            auto&q=s.addPlayer("q","法師","mage");
            wild(q);
            auto mp=q.vitals.mp;
            CombatSystem::attack(s.world(),s.catalog(),q.id,"m");
            CHECK(q.vitals.mp==mp-2);
        });
        group("world boss shared pool hitters first",[] {
            Simulation s;
            auto&w=s.world();
            auto&c=s.catalog();
            for(int i=0;i<9;i++) {
                auto&p=s.addPlayer("p"+std::to_string(i),"參與者");
                wild(p);
            }
            auto&spectator=s.addPlayer("spectator","旁觀者");
            (void)spectator;
            auto boss=CombatSystem::spawn(w,"encounter", {100,0,24},
            1,"windchime",true,"boss_windchime");
            auto&m=w.monsters.at(boss.id);
            for(int i=0;i<9;i++)m.contribution["p"+std::to_string(i)]=i+1;
            m.receiveDamage(m.vitals.maxHp);
            RewardSystem::onDeath(w,m,c);
            CHECK(w.rewards.size()==9);
            int items=0;
            for(auto&[id,r]:w.rewards) {
                items+=static_cast<int>(r.items.size());
                CHECK(r.player!="spectator");
                for(auto&i:r.items)CHECK(i.rarity==Rarity::Rare);
            }
            CHECK(items==3);
            CHECK(!w.rewards.at("reward:encounter:p8").items.empty());
        });
        group("gather reservations distance proficiency tools respawn",[] {
            Simulation s;
            auto&p=s.addPlayer("p","角色");
            auto&q=s.addPlayer("q","角色");
            wild(p);
            wild(q);
            auto&w=s.world();
            auto&c=s.catalog();
            w.resources.emplace("tree",ResourceNode {
                "tree","wood",p.position,"world",0,std::nullopt,0
            });
            rejected([&] {
                GatheringSystem::begin(w,p,"tree",c);
            });
            p.gatherXp=12;
            p.tools["axe"]=1;
            q.gatherXp=12;
            q.tools["axe"]=1;
            GatheringSystem::begin(w,p,"tree",c);
            rejected([&] {
                GatheringSystem::begin(w,q,"tree",c);
            });
            rejected([&] {
                GatheringSystem::finish(w,p,"tree",c);
            });
            s.advance(4);
            p.position.x+=10;
            rejected([&] {
                GatheringSystem::finish(w,p,"tree",c);
            });
            p.position.x-=10;
            GatheringSystem::finish(w,p,"tree",c);
            CHECK(p.materials.at("wood")==1);
            CHECK(p.gatherXp>12);
            rejected([&] {
                GatheringSystem::begin(w,p,"tree",c);
            });
            for(int i=0;i<3;i++)s.advance(10);
            GatheringSystem::begin(w,q,"tree",c);
            GatheringSystem::cancel(w,q,"tree");
            CHECK(!w.resources.at("tree").reservedBy);
        });
        group("craft atomicity city restrictions tools and arrow discounts",[] {
            Simulation s;
            auto&p=s.addPlayer("p","角色");
            auto&w=s.world();
            auto&c=s.catalog();
            p.materials= {
                {
                    "twig",3
                },
                {
                    "stone",2
                }
            };
            CraftingSystem::craft(w,p,"stoneaxe","",c);
            CHECK(p.tools.at("axe")==1);
            CHECK(p.materials.at("twig")==0);
            auto before=p.materials;
            rejected([&] {
                CraftingSystem::craft(w,p,"stonepick","",c);
            });
            CHECK(p.materials==before);
            auto&r=c.recipes.at("gear_cindergreatsword");
            p.materials=r.cost;
            rejected([&] {
                CraftingSystem::craft(w,p,r.id,"windchime_smith",c);
            });
            CHECK(p.materials==r.cost);
            p.position=c.regions.at("cinder").center;
            CraftingSystem::craft(w,p,r.id,"cinder_smith",c);
            CHECK(p.inventory.back().definition==r.output);
            CHECK(CraftingSystem::requiredRegion(c.recipes.at("gear_leafbow"),p,c)=="verdant");
            CHECK(CraftingSystem::requiredRegion(c.recipes.at("gear_astralstaff"),p,c)=="astral");
            auto&q=s.addPlayer("q","弓手","ranger");
            q.materials= {
                {
                    "wood",2
                },
                {
                    "stone",1
                }
            };
            CraftingSystem::craft(w,q,"woodarrow","",c);
            CHECK(q.materials.at("woodarrow")==20);
        });
        group("quest progression and no repeat claim",[] {
            Simulation s;
            auto&p=s.addPlayer("p","角色");
            auto&c=s.catalog();
            QuestSystem::accept(p,"first_gather","windchime_guild",c);
            rejected([&] {
                QuestSystem::claim(p,"first_gather","windchime_guild",c);
            });
            for(int i=0;i<5;i++)QuestSystem::record(p,"gather","stone",c);
            CHECK(p.quests.at("first_gather").count==4);
            QuestSystem::claim(p,"first_gather","windchime_guild",c);
            CHECK(p.coins==90);
            CHECK(p.xp==80);
            rejected([&] {
                QuestSystem::claim(p,"first_gather","windchime_guild",c);
            });
        });
        group("class advancement at NPC with materials secret and parent",[] {
            Simulation s;
            auto&p=s.addPlayer("p","角色");
            auto&c=s.catalog();
            p.level=5;
            p.materials= {
                {
                    "ore",5
                },
                {
                    "crystal",2
                }
            };
            wild(p);
            rejected([&] {
                ProgressionSystem::changeClass(p,"knight","windchime_guild",c);
            });
            p.position=c.regions.at("windchime").center;
            ProgressionSystem::changeClass(p,"knight","windchime_guild",c);
            CHECK(p.profession=="knight");
            CHECK(p.materials.at("ore")==0);
            ProgressionSystem::changeClass(p,"knight","windchime_guild",c);
            p.level=100;
            rejected([&] {
                ProgressionSystem::changeClass(p,"shadow","windchime_guild",c);
            });
            CHECK(p.profession=="knight");
        });
        group("economy overflow negatives proximity and warehouse",[] {
            Simulation s;
            auto&p=s.addPlayer("p","角色");
            auto&q=s.addPlayer("q","角色");
            auto&c=s.catalog();
            p.coins=200;
            EconomySystem::buy(s.world(),p,"potion",2,"windchime_merchant",c);
            CHECK(p.coins==160);
            CHECK(p.materials.at("potion")==2);
            rejected([&] {
                EconomySystem::buy(s.world(),p,"potion",-1,"windchime_merchant",c);
            });
            InventorySystem::store(p,"potion",1,"windchime_merchant",c);
            CHECK(p.warehouse.at("potion")==1);
            InventorySystem::store(p,"potion",1,"windchime_merchant",c,true);
            CHECK(p.materials.at("potion")==2);
            EconomySystem::transfer(p,q,10);
            CHECK(p.coins==150&&q.coins==10);
            rejected([&] {
                EconomySystem::transfer(p,q,-10);
            });
            Materials m {
                {
                    "stone",INT64_MAX
                }
            };
            rejected([&] {
                InventorySystem::add(m,"stone",1);
            });
            CHECK(m.at("stone")==INT64_MAX);
        });
        group("equipment warehouse ownership atomicity and character compatibility",[] {
            Simulation s;
            auto& p=s.addPlayer("p","倉庫測試");
            auto& c=s.catalog();
            auto& w=s.world();
            const Id npc="windchime_merchant";
            const auto equipped=p.inventory.front().id;
            rejected([&]{InventorySystem::storeItem(p,equipped,npc,c);});
            CHECK(p.warehouseItems.empty());
            auto item=InventorySystem::create(w,c,p.inventory.front().definition,Rarity::Rare,1);
            item.upgrade=3;
            p.inventory.push_back(item);
            p.position={1000,0,1000};
            rejected([&]{InventorySystem::storeItem(p,item.id,npc,c);});
            CHECK(p.inventory.back().id==item.id);
            p.position=c.npcs.at(npc).position;
            InventorySystem::storeItem(p,item.id,npc,c);
            CHECK(p.warehouseItems.size()==1&&p.warehouseItems.front().upgrade==3);
            rejected([&]{InventorySystem::equip(p,item.id,c);});
            rejected([&]{InventorySystem::storeItem(p,item.id,npc,c);});
            rejected([&]{InventorySystem::storeItem(p,"other_player_item",npc,c,true);});
            CHECK(p.warehouseItems.size()==1);
            CharacterFile::save(p,"warehouse.awc",c);
            auto loaded=CharacterFile::load("warehouse.awc",c);
            CHECK(loaded.warehouseItems.front().id==item.id);
            CHECK(loaded.warehouseItems.front().rarity==Rarity::Rare);
            loaded.warehouseItems.push_back(loaded.inventory.front());
            rejected([&]{CharacterFile::save(loaded,"invalid-warehouse.awc",c);});
            InventorySystem::storeItem(p,item.id,npc,c,true);
            CHECK(p.warehouseItems.empty()&&p.inventory.back().id==item.id);
            CHECK(p.inventory.back().upgrade==3);
            const double oldMax=p.vitals.maxHp;
            InventorySystem::unequip(p,equipped,c);
            CHECK(p.equipped.empty()&&p.vitals.maxHp<=oldMax);
            rejected([&]{InventorySystem::unequip(p,equipped,c);});
            InventorySystem::storeItem(p,equipped,npc,c);
            CHECK(p.warehouseItems.front().id==equipped);
            InventorySystem::storeItem(p,equipped,npc,c,true);
            // A v1 character has no warehouse item section; verify backward loading.
            CharacterFile::save(p,"warehouse.awc",c);
            std::ifstream input("warehouse.awc");
            std::string data((std::istreambuf_iterator<char>(input)),{});
            data.replace(0,8,"AWCHAR 1");
            const auto tail=data.rfind("0\nEND\n");
            CHECK(tail!=std::string::npos);
            data.erase(tail,2);
            std::ofstream("warehouse-v1.awc")<<data;
            CHECK(CharacterFile::load("warehouse-v1.awc",c).warehouseItems.empty());
            std::filesystem::remove("warehouse.awc");
            std::filesystem::remove("warehouse.awc.bak");
            std::filesystem::remove("warehouse-v1.awc");
        });
        group("fall damage boundaries and low health",[] {
            CHECK(MovementSystem::landingDamage(6,100,.6)==0);
            CHECK(MovementSystem::landingDamage(100,100,.59)==0);
            CHECK(MovementSystem::landingDamage(7,100,.6)==3);
            CHECK(MovementSystem::landingDamage(1000,100,.6)==35);
            CHECK(MovementSystem::landingDamage(1000,5000,.6)==180);
            CHECK(MovementSystem::landingDamage(std::numeric_limits<double>::quiet_NaN(),100,1)==0);
            Player p;
            p.vitals.hp=2;
            MovementSystem::land(p,7,1);
            CHECK(!p.vitals.alive());
        });
        group("stamina exhaustion lock recovery and speed",[] {
            Simulation s;
            auto&p=s.addPlayer("p","角色");
            auto&c=s.catalog();
            p.vitals.stamina=10;
            MovementSystem::tickStamina(p,1,true,1);
            CHECK(p.sprintLocked);
            CHECK(p.vitals.stamina==0);
            MovementSystem::tickStamina(p,1,false,4);
            CHECK(p.sprintLocked);
            MovementSystem::tickStamina(p,1,false,5);
            CHECK(!p.sprintLocked);
            auto pos=p.position;
            rejected([&] {
                MovementSystem::move(p, {1000,0,24},
                1,false,c,6);
            });
            CHECK(p.position.x==pos.x);
            MovementSystem::move(p, {5,0,24},
            1,false,c,6);
            CHECK(p.position.x==5);
            auto stamina=p.vitals.stamina;
            MovementSystem::tickStamina(p,1,false,6);
            CHECK(p.vitals.stamina==stamina);
        });
        group("skill mana cooldown area freeze and ownership",[] {
            Simulation s;
            auto&p=s.addPlayer("p","法師","mage");
            wild(p);
            auto&w=s.world();
            auto&c=s.catalog();
            CombatSystem::spawn(w,"m",p.position,10,"windchime");
            rejected([&] {
                CombatSystem::cast(w,c,p.id,"frost","m");
            });
            p.learnedSkills.insert("frost");
            auto mp=p.vitals.mp;
            CombatSystem::cast(w,c,p.id,"frost","m");
            CHECK(p.vitals.mp==mp-24);
            CHECK(w.monsters.at("m").frozenUntil==3);
            rejected([&] {
                CombatSystem::monsterAttack(w,c,"m",p.id);
            });
            rejected([&] {
                CombatSystem::cast(w,c,p.id,"frost","m");
            });
        });
        group("dungeon guards objectives boss chest and return",[] {
            Simulation s;
            auto&p=s.addPlayer("p","角色");
            p.level=100;
            ProgressionSystem::refresh(p,s.catalog());
            p.vitals.hp=p.vitals.maxHp;
            auto&c=s.catalog();
            auto&w=s.world();
            rejected([&] {
                DungeonSystem::enter(w,p,"ruins",c);
            });
            p.position=c.dungeons.at("ruins").entrancePosition;
            auto pos=p.position;
            auto run=DungeonSystem::enter(w,p,"ruins",c);
            Id boss;
            std::vector<Id>guards;
            for(auto&[id,m]:w.monsters)if(m.scene==run) {
                if(m.definition=="dungeon_boss")boss=id;
                else guards.push_back(id);
            }
            rejected([&] {
                CombatSystem::attack(w,c,p.id,boss);
            });
            rejected([&] {
                DungeonSystem::openChest(w,p,c);
            });
            for(auto&id:guards)kill(s,p,id);
            CHECK(w.runs.at(run).guardsRemaining==0);
            for(auto&id: {
                "objective_1","objective_2","objective_3"
            })DungeonSystem::interactObjective(w,p,id,c);
            rejected([&] {
                DungeonSystem::interactObjective(w,p,"objective_1",c);
            });
            kill(s,p,boss);
            DungeonSystem::openChest(w,p,c);
            CHECK(w.runs.at(run).claimed);
            CHECK(p.learnedSkills.contains("wind"));
            rejected([&] {
                DungeonSystem::openChest(w,p,c);
            });
            DungeonSystem::leave(w,p);
            CHECK(p.scene=="world");
            CHECK(distance(pos,p.position)==0);
            CHECK(w.runs.empty());
            CHECK(w.monsters.empty());
        });
        group("character snapshot roundtrip version validation and backup",[] {
            Simulation s;
            auto&p=s.addPlayer("p","有 空格的角色");
            auto&c=s.catalog();
            p.materials= {
                {
                    "stone",3
                }
            };
            p.tools["axe"]=2;
            p.processedRewards.insert("long:reward:id:1234567890");
            p.learnedSkills.insert("wind");
            QuestSystem::accept(p,"first_gather","windchime_guild",c);
            CharacterFile::save(p,"test.awc",c);
            auto loaded=CharacterFile::load("test.awc",c);
            CHECK(loaded.name==p.name);
            CHECK(loaded.inventory.front().id==p.inventory.front().id);
            CHECK(loaded.materials==p.materials);
            CHECK(loaded.processedRewards==p.processedRewards);
            CHECK(loaded.learnedSkills==p.learnedSkills);
            CHECK(loaded.quests.size()==1);
            CharacterFile::save(loaded,"test.awc",c);
            CHECK(std::filesystem::exists("test.awc.bak"));
            loaded.coins=-1;
            rejected([&] {
                CharacterFile::save(loaded,"test.awc",c);
            });
            CHECK(CharacterFile::load("test.awc",c).coins==0);
            std::ofstream("broken.awc")<<"AWCHAR 99\n";
            rejected([&] {
                CharacterFile::load("broken.awc",c);
            });
            p.scene="run_invalid";
            rejected([&] {
                CharacterFile::save(p,"test.awc",c);
            });
            for(auto path: {
                "test.awc","test.awc.bak","broken.awc"
            })std::filesystem::remove(path);
        });
        group("death respawn and simulation invalid time",[] {
            Simulation s;
            auto&p=s.addPlayer("p","角色");
            auto&c=s.catalog();
            wild(p);
            p.xp=80;
            p.receiveDamage(1000);
            rejected([&] {
                p.attackIntent();
            });
            CombatSystem::respawn(p,c);
            CHECK(p.vitals.alive());
            CHECK(p.xp==70);
            CHECK(p.position.x==0);
            rejected([&] {
                s.advance(-1);
            });
            rejected([&] {
                s.advance(std::numeric_limits<double>::infinity());
            });
        });
        group("party invitation membership and leadership",[] {
            Simulation s;
            auto&p=s.addPlayer("p","隊長");
            auto&q=s.addPlayer("q","隊友");
            SessionState sessions;
            auto id=PartySystem::create(s.world(),sessions,p.id);
            rejected([&] {
                PartySystem::join(s.world(),sessions,q.id,id);
            });
            PartySystem::invite(s.world(),sessions,p.id,q.id);
            PartySystem::join(s.world(),sessions,q.id,id);
            CHECK(sessions.parties.at(id).members.size()==2);
            PartySystem::leave(sessions,p.id);
            CHECK(sessions.parties.at(id).leader==q.id);
            PartySystem::leave(sessions,q.id);
            CHECK(sessions.parties.empty());
        });
        group("admin permissions audit and PvP actual HP",[] {
            Simulation s;
            auto&p=s.addPlayer("p","玩家");
            auto&q=s.addPlayer("q","玩家");
            auto&w=s.world();
            auto&c=s.catalog();
            SessionState state;
            state.accounts.emplace("owner",Account {
                "owner",Role::Owner
            });
            state.accounts.emplace("normal",Account {
                "normal",Role::Player
            });
            rejected([&] {
                AdministrationSystem::setPvp(w,state,"normal",true);
            });
            rejected([&] {
                AdministrationSystem::grantAdmin(w,state,"normal","owner");
            });
            AdministrationSystem::grantAdmin(w,state,"owner","normal");
            CHECK(state.accounts.at("normal").role==Role::Admin);
            AdministrationSystem::setPvp(w,state,"normal",true);
            rejected([&] {
                PvpSystem::attack(w,c,state,p.id,q.id);
            });
            wild(p);
            wild(q);
            auto hp=q.vitals.hp;
            auto damage=PvpSystem::attack(w,c,state,p.id,q.id);
            CHECK(q.vitals.hp==hp-damage);
            CHECK(damage==33);
            CHECK(!w.events.empty());
            AdministrationSystem::teleportToCity(w,c,state,"normal",q.id,"astral");
            CHECK(q.position.x==c.regions.at("astral").center.x);
        });
        std::cout<<"OK "<<groups<<" groups, "<<checks<<" checks\n";
        return 0;
    }
    catch(const std::exception&e) {
        std::cerr<<"FAIL after "<<groups<<" groups: "<<e.what()<<'\n';
        return 1;
    }
}

