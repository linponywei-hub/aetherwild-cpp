#include "aetherwild/persistence.hpp"
#include <iostream>
#include <sstream>
#include <string_view>
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif
using namespace aw;
namespace {
    void status(const Player&p) {
        std::cout<<p.name<<" ["<<p.id<<"] "<<p.profession<<" Lv."<<p.level<<" XP "<<p.xp<<" HP "<<p.vitals.hp<<'/'<<p.vitals.maxHp<<" MP "<<p.vitals.mp<<'/'<<p.vitals.maxMp<<" 體力 "<<p.vitals.stamina<<" 金幣 "<<p.coins<<" 採集 "<<p.gatherXp<<" 場景 "<<p.scene<<" 座標 "<<p.position.x<<','<<p.position.y<<','<<p.position.z<<'\n';
    }
    void claimAll(Simulation&s,Player&p) {
        std::vector<Id>ids;
        for(auto&[id,r]:s.world().rewards)if(r.player==p.id)ids.push_back(id);
        for(auto&id:ids)RewardSystem::claim(s.world(),p,id,s.catalog());
    }
    void populate(Simulation&s) {
        auto&w=s.world();
        for(int i=0;i<6;i++)CombatSystem::spawn(w,"wolf"+std::to_string(i), {90.0+i*8,0,24},
        1,"windchime");
        for(auto&[id,b]:s.catalog().bosses) {
            CombatSystem::spawn(w,id,b.position,b.level,b.region,true,id);
            auto&m=w.monsters.at(id);
            m.vitals.hp=m.vitals.maxHp=b.hp;
            m.attack=b.attack;
        }
        for(auto&code: {
            "twig","stone","wood","ore","herb"
        })for(int i=0;i<4;i++) {
            ResourceNode n;
            n.id=std::string(code)+std::to_string(i);
            n.definition=code;
            n.position= {88.0+i,0,24};
            w.resources.emplace(n.id,n);
        }
    }
    void help() {
        std::cout<<R"(指令（ID 使用英文；本版是 C++ 核心文字測試介面）：
 status | look | bag | catalog [items/classes/dungeons/recipes/quests]
 walk <dx> <dz> [sprint]   每次1秒，普通最多5m，奔跑8m
 wait <seconds 0..10> | attack <monster> [arrow] | cast <skill> <monster>
 gather <node> | harvest <node> | cancel <node>  開始/完成/取消
 craft <recipe> [npc] | equip <instance> | potion [potion/energy_potion]
 accept <quest> <npc> | quest <quest> <npc> | job <class> <npc>
 buy <code> <count> <npc> | store <code> <count> <npc> | withdraw <code> <count> <npc>
 enter <dungeon> | objective <objective_1..3> | chest | leave
 rewards | respawn | save <file.awc> | load <file.awc> | help | quit
附近 NPC 的 id 可由 look 查看。例：windchime_guild。
)";
    }
    void demo() {
        Simulation s;
        populate(s);
        auto&p=s.addPlayer("demo","旅人");
        auto&w=s.world();
        auto&c=s.catalog();
        QuestSystem::accept(p,"first_gather","windchime_guild",c);
        p.position= {90,0,24};
        for(auto id: {
            "twig0","twig1","stone0","stone1"
        }) {
            GatheringSystem::begin(w,p,id,c);
            s.advance(2);
            GatheringSystem::finish(w,p,id,c);
        }
        p.position=c.regions.at("windchime").center;
        QuestSystem::claim(p,"first_gather","windchime_guild",c);
        p.position= {90,0,24};
        while(w.monsters.at("wolf0").vitals.alive()) {
            auto r=CombatSystem::attack(w,c,p.id,"wolf0");
            std::cout<<"攻擊 wolf0："<<r.damage<<'\n';
            s.advance(.6);
            if(!r.killed)CombatSystem::monsterAttack(w,c,"wolf0",p.id);
        }
        claimAll(s,p);
        status(p);
        std::cout<<"DEMO OK：採集 → 任務交付 → 戰鬥 → 獎勵。\n";
    }
}
int main(int argc,char**argv) {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
#endif
    try {
        if(argc>1&&std::string_view(argv[1])=="--demo") {
            demo();
            return 0;
        }
        Simulation s;
        populate(s);
        auto&c=s.catalog();
        auto&w=s.world();
        s.addPlayer("local-player","旅人");
        std::cout<<"界域紀元 C++ 0.1.0 — 本機核心重建版（無3D、無網路登入）\n";
        help();
        std::string line;
        while(std::cout<<"> "&&std::getline(std::cin,line)) {
            try {
                std::istringstream in(line);
                std::string cmd,a,b,npc;
                Quantity n=0;
                in>>cmd;
                auto&p=w.players.at("local-player");
                if(cmd.empty())continue;
                if(cmd=="quit")break;
                else if(cmd=="help")help();
                else if(cmd=="status")status(p);
                else if(cmd=="look") {
                    for(auto&[id,n]:c.npcs)if(p.scene=="world"&&distance(p.position,n.position)<100)std::cout<<"NPC "<<id<<' '<<n.name<<" 距離 "<<distance(p.position,n.position)<<'\n';
                    for(auto&[id,m]:w.monsters)if(m.scene==p.scene&&m.vitals.alive()&&distance(p.position,m.position)<100)std::cout<<"怪物 "<<id<<" HP "<<m.vitals.hp<<" 距離 "<<distance(p.position,m.position)<<'\n';
                    for(auto&[id,r]:w.resources)if(r.scene==p.scene&&distance(p.position,r.position)<100)std::cout<<"資源 "<<id<<" 距離 "<<distance(p.position,r.position)<<'\n';
                }
                else if(cmd=="bag") {
                    for(auto&[id,v]:p.materials)std::cout<<id<<" x"<<v<<'\n';
                    for(auto&i:p.inventory)std::cout<<i.id<<' '<<c.items.at(i.definition).name<<" 需求Lv."<<i.requiredLevel<<'\n';
                    for(auto&[id,t]:p.tools)std::cout<<"工具 "<<id<<" T"<<t<<'\n';
                }
                else if(cmd=="catalog") {
                    in>>a;
                    if(a=="items")for(auto&[id,v]:c.items)std::cout<<id<<' '<<v.name<<'\n';
                    else if(a=="classes")for(auto&[id,v]:c.classes) {
                        if(v.secret.empty()||p.secrets.contains(v.secret))std::cout<<id<<' '<<v.name<<" Lv."<<v.level<<'\n';
                    }
                    else if(a=="dungeons")for(auto&[id,v]:c.dungeons)std::cout<<id<<' '<<v.name<<" Lv."<<v.level<<" 入口 "<<v.entrancePosition.x<<','<<v.entrancePosition.z<<'\n';
                    else if(a=="recipes")for(auto&[id,v]:c.recipes) {
                        std::cout<<id<<" -> "<<v.output<<" 材料";
                        for(auto&[k,q]:v.cost)std::cout<<' '<<k<<':'<<q;
                        std::cout<<'\n';
                    }
                    else if(a=="quests")for(auto&[id,v]:c.quests)std::cout<<id<<' '<<v.name<<" NPC "<<v.npc<<'\n';
                }
                else if(cmd=="walk") {
                    double x=0,z=0;
                    in>>x>>z>>a;
                    MovementSystem::move(p, {p.position.x+x,p.position.y,p.position.z+z},
                    1,a=="sprint",c,w.now+1);
                    s.advance(1);
                    status(p);
                }
                else if(cmd=="wait") {
                    double t=0;
                    in>>t;
                    s.advance(t);
                }
                else if(cmd=="attack") {
                    in>>a>>b;
                    auto r=CombatSystem::attack(w,c,p.id,a,b.empty()?"woodarrow":b);
                    std::cout<<"傷害 "<<r.damage<<(r.killed?" 擊殺":"")<<'\n';
                    s.advance(.8);
                    auto& monster=w.monsters.at(a);
                    if(!r.killed&&distance(p.position,monster.position)<=5&&w.now>=monster.nextAttack&&w.now>=monster.frozenUntil) CombatSystem::monsterAttack(w,c,a,p.id);
                }
                else if(cmd=="cast") {
                    in>>a>>b;
                    CombatSystem::cast(w,c,p.id,a,b);
                }
                else if(cmd=="gather") {
                    in>>a;
                    GatheringSystem::begin(w,p,a,c);
                    std::cout<<"完成時間 "<<w.resources.at(a).completeAt<<"（使用 wait 等待）\n";
                }
                else if(cmd=="harvest") {
                    in>>a;
                    GatheringSystem::finish(w,p,a,c);
                }
                else if(cmd=="cancel") {
                    in>>a;
                    GatheringSystem::cancel(w,p,a);
                }
                else if(cmd=="craft") {
                    in>>a>>npc;
                    CraftingSystem::craft(w,p,a,npc,c);
                }
                else if(cmd=="equip") {
                    in>>a;
                    InventorySystem::equip(p,a,c);
                }
                else if(cmd=="potion") {
                    in>>a;
                    InventorySystem::usePotion(p,a.empty()?"potion":a,c);
                }
                else if(cmd=="accept") {
                    in>>a>>npc;
                    QuestSystem::accept(p,a,npc,c);
                }
                else if(cmd=="quest") {
                    in>>a>>npc;
                    QuestSystem::claim(p,a,npc,c);
                }
                else if(cmd=="job") {
                    in>>a>>npc;
                    ProgressionSystem::changeClass(p,a,npc,c);
                }
                else if(cmd=="buy") {
                    in>>a>>n>>npc;
                    EconomySystem::buy(w,p,a,n,npc,c);
                }
                else if(cmd=="store"||cmd=="withdraw") {
                    in>>a>>n>>npc;
                    InventorySystem::store(p,a,n,npc,c,cmd=="withdraw");
                }
                else if(cmd=="enter") {
                    in>>a;
                    std::cout<<DungeonSystem::enter(w,p,a,c)<<'\n';
                }
                else if(cmd=="objective") {
                    in>>a;
                    DungeonSystem::interactObjective(w,p,a,c);
                }
                else if(cmd=="chest")DungeonSystem::openChest(w,p,c);
                else if(cmd=="leave")DungeonSystem::leave(w,p);
                else if(cmd=="rewards")claimAll(s,p);
                else if(cmd=="respawn") {
                    require(!p.vitals.alive(),"角色尚未死亡");
                    if(w.runs.contains(p.scene))DungeonSystem::leave(w,p);
                    CombatSystem::respawn(p,c);
                }
                else if(cmd=="save") {
                    in>>a;
                    claimAll(s,p);
                    CharacterFile::save(p,a,c);
                    std::cout<<"已儲存角色。\n";
                }
                else if(cmd=="load") {
                    in>>a;
                    require(p.scene=="world","請先離開副本");
                    auto loaded=CharacterFile::load(a,c);
                    require(loaded.id==p.id,"此存檔角色ID不符");
                    p=std::move(loaded);
                    for(auto&i:p.inventory) {
                        auto pos=i.id.rfind('_');
                        if(pos!=Id::npos)try {
                            w.sequence=std::max(w.sequence,static_cast<std::uint64_t>(std::stoull(i.id.substr(pos+1))));
                        }
                        catch(const std::exception&) {}
                    }
                    p.attackReady=w.now;
                    p.skillReady.clear();
                    p.regenReady=w.now+2;
                    p.staminaUpdatedAt=w.now;
                    status(p);
                }
                else std::cout<<"未知指令。輸入 help。\n";
            }
            catch(const std::exception&e) {
                std::cout<<"操作未完成："<<e.what()<<'\n';
            }
        }
        return 0;
    }
    catch(const std::exception&e) {
        std::cerr<<e.what()<<'\n';
        return 1;
    }
}
