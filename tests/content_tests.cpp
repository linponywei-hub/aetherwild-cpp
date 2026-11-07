#include "aetherwild/runtime.hpp"
#include <fstream>
#include <iostream>
#include <source_location>
using namespace aw;
namespace {int checks=0;void yes(bool c,std::source_location at=std::source_location::current()){++checks;if(!c)throw std::runtime_error("content assertion "+std::to_string(checks)+" at line "+std::to_string(at.line()));}template<class F>void no(F f){++checks;try{f();}catch(const RuleError&){return;}throw std::runtime_error("expected refusal "+std::to_string(checks));}
Json cmd(GameRuntime& g,const Json& s,const std::string& type,Json extra=Json::object()) {static int count=0;extra["type"]=type;extra["request"]="content_test_"+std::to_string(++count);return g.command(s.str("token"),extra);}
Json create(GameRuntime&g,const std::string& job="warrior"){return g.createSession(Json{{"name","內容測試"},{"profession",job}});}
}
int main(){try{
 auto c=makeCatalog();installContent(c);yes(c.classes.size()==46);yes(c.skills.size()>=285);yes(c.npcs.size()>=165);
 yes(c.classes.at("aw_class_tank").weapon==WeaponKind::ShieldBlade);yes(c.classes.at("aw_class_assassin").weapon==WeaponKind::Daggers);
 yes(c.items.at(startingWeapon(c,"aw_class_assassin")).weapon==WeaponKind::Daggers);
 yes(contentBundle().at("monsters").objectItems().size()==441);yes(contentBundle().at("skills").objectItems().size()==1681);
 const auto& profile=*contentEntry("profiles","windchime_merchant");auto rel=NpcSystem::initial();double now=2000000;
 NpcSystem::event(rel,profile,"chat","c1",now);yes(NpcSystem::score(rel)==1);NpcSystem::event(rel,profile,"chat","c1",now);yes(NpcSystem::score(rel)==1);
 rel["score"]=80;NpcSystem::event(rel,profile,"attack","a1",now+10);yes(NpcSystem::score(rel)==55);yes(NpcSystem::view(rel,profile,now+10).flag("restricted"));
 NpcSystem::event(rel,profile,"purchase","p1",now+20,200);yes(NpcSystem::score(rel)==55);no([&]{NpcSystem::event(rel,profile,"repair","r1",now+30);});
 NpcSystem::event(rel,profile,"repair","r2",now+86411);yes(!rel.flag("repair"));yes(NpcSystem::score(rel)==60);
 auto capped=NpcSystem::initial();for(int i=0;i<9;i++)NpcSystem::event(capped,profile,"quest","q"+std::to_string(i),now+i);yes(NpcSystem::score(capped)==20);
 GameRuntime g({},true);auto a=create(g,"aw_class_tank"),b=create(g,"aw_class_assassin");auto aid=a.at("player").str("id"),bid=b.at("player").str("id");
 yes(g.world().players.at(aid).learnedSkills.size()>=6);yes(g.world().players.at(aid).inventory.size()==1);
 auto chat=cmd(g,a,"npc_chat",Json{{"target","windchime_merchant"},{"topic",profile.at("conversation_topics").arrayItems()[0]}});
 yes(chat.at("content").at("relations").at("windchime_merchant").num("score")==1);
 yes(g.snapshot(b.str("token")).at("content").at("relations").at("windchime_merchant").num("score")==0);
 no([&]{cmd(g,b,"content_release",Json{{"release",2}});});
 auto item=g.world().players.at(aid).inventory[0].id;
 auto expanded=cmd(g,a,"expansion_start");yes(expanded.at("content").flag("expanded"));yes(g.definitions().regions.size()==25);yes(g.definitions().dungeons.size()==180);yes(g.definitions().worldLimit==48000);yes(g.definitions().npcs.at("aw_crownreach_guild").role=="guild");yes(contentEntry("profiles","aw_crownreach_guild")!=nullptr);yes(contentBundle().at("profiles").objectItems().size()==263);
 yes(g.definitions().regions.at("aw_crownreach").center.x==0&&g.definitions().regions.at("aw_crownreach").center.z==0);
 yes(g.world().players.at(aid).inventory[0].id==item);yes(std::abs(g.world().players.at(aid).position.x-g.definitions().regions.at("windchime").center.x)<100);
 no([&]{cmd(g,a,"admin_travel",Json{{"target","aw_crownreach"}});});
 no([&]{cmd(g,a,"content_release",Json{{"release",4}});});
 const auto& ex=g.definitions();yes(!contentPositionAllowed(ex,{0,0,0}));yes(contentPositionAllowed(ex,ex.regions.at("windchime").center));
 // Open region anchors remain accessible while historical release fixtures gate capital.
 yes(contentPositionAllowed(ex,ex.regions.at("windchime").center));
 auto opened=cmd(g,a,"content_release",Json{{"release",2}});yes(opened.at("content").num("release")==2);cmd(g,a,"admin_travel",Json{{"target","aw_crownreach"}});yes(distance(g.world().players.at(aid).position,cityArrival(g.definitions().regions.at("aw_crownreach")))<1);

 // Class ownership and taunt are server rules, not UI switches.
 Simulation sim;Catalog all=makeCatalog();installContent(all);auto& w=sim.world();Player tank;tank.id="t";tank.profession="aw_class_tank";tank.jobs={tank.profession};tank.position={1000,0,0};ProgressionSystem::refresh(tank,all);tank.vitals.hp=tank.vitals.maxHp;tank.vitals.mp=tank.vitals.maxMp;w.players[tank.id]=tank;
 CombatSystem::spawn(w,"enemy",{1004,0,0},5,"windchime");auto taunt=all.classSkills.at(tank.profession)[2];CombatSystem::cast(w,all,"t",taunt);yes(w.monsters.at("enemy").tauntedBy=="t");
 auto foreign=all.classSkills.at("aw_class_assassin")[0];w.players.at("t").learnedSkills.insert(foreign);no([&]{CombatSystem::cast(w,all,"t",foreign,"enemy");});
 // Authored dialogue graph cannot jump to completion or accept invalid choices.
 GameRuntime narrative({},true);auto ns=create(narrative);auto npid=ns.at("player").str("id");
 auto qid=Id("aw_update_windchime_1");
 cmd(narrative,ns,"story_talk",Json{{"target",qid}});
 no([&]{cmd(narrative,ns,"story_choice",Json{{"target",qid},{"choice","community"}});});
 cmd(narrative,ns,"story_choice",Json{{"target",qid},{"choice","accept"}});
 no([&]{cmd(narrative,ns,"story_inspect",Json{{"target",qid}});});
 auto region=narrative.definitions().regions.at("windchime");auto evidence=cityArrival(region);evidence.x+=20;evidence.z+=12;narrative.world().players.at(npid).position=evidence;
 cmd(narrative,ns,"story_inspect",Json{{"target",qid}});
 narrative.world().players.at(npid).position=narrative.definitions().npcs.at("windchime_guild").position;
 cmd(narrative,ns,"story_talk",Json{{"target",qid}});
 auto ending=cmd(narrative,ns,"story_choice",Json{{"target",qid},{"choice","community"}});bool found=false;
 for(auto& st:ending.at("content").at("stories").arrayItems())if(st.str("id")==qid)found=st.at("progress").flag("done");yes(found);
 no([&]{cmd(narrative,ns,"story_choice",Json{{"target",qid},{"choice","community"}});});
 // Player-supplied scores and gifts have no authority.
 auto beforeScore=ending.at("content").at("relations").at("windchime_guild").num("score");
 no([&]{cmd(narrative,ns,"npc_gift",Json{{"target","windchime_guild"},{"score",100}});});
 yes(narrative.snapshot(ns.str("token")).at("content").at("relations").at("windchime_guild").num("score")==beforeScore);
 // World bounds and locked destinations are checked inside domain services too.
 auto actor=g.world().players.at(aid);auto locked=g.definitions().dungeons.at("aw_exp_aw_starfall_01");actor.position=locked.entrancePosition;
 no([&]{DungeonSystem::enter(g.world(),actor,locked.id,g.definitions());});
 yes(worldLandHeight(g.definitions().regions.at("frosthold").center,g.definitions())>=250);
 // Runtime restart retains expanded layout, relationship state and actor binding.
 auto pathFile=std::filesystem::temp_directory_path()/"aw_content_restart_test"/"world.json";std::filesystem::remove_all(pathFile.parent_path());Id key,pid;
 {GameRuntime save(pathFile,true);auto s=create(save);key=s.str("token");pid=s.at("player").str("id");cmd(save,s,"npc_chat",Json{{"target","windchime_merchant"},{"topic",profile.at("conversation_topics").arrayItems()[0]}});cmd(save,s,"expansion_start");save.save();}
 {GameRuntime restored(pathFile,true);auto state=restored.snapshot(key);yes(state.at("content").flag("expanded"));yes(state.at("player").str("id")==pid);yes(state.at("content").at("relations").at("windchime_merchant").num("score")==1);}
 // Production startup upgrades legacy saves without changing identities, wallets or items.
 {GameRuntime full(pathFile);auto state=full.snapshot(key);yes(state.at("content").flag("expanded"));yes(state.at("content").num("release")==5);yes(full.definitions().regions.size()==25);yes(full.definitions().worldLimit==48000);yes(full.definitions().regions.at("aw_crownreach").radius==10240);yes(state.at("player").str("id")==pid);yes(full.adminMap(key).at("planned").arrayItems().size()==25);yes(contentPositionAllowed(full.definitions(),{40000,0,40000}));yes(!contentPositionAllowed(full.definitions(),{49000,0,0}));full.save();}
 {GameRuntime full(pathFile);auto state=full.snapshot(key);yes(state.at("content").num("release")==5);yes(std::abs(state.at("player").at("position").num("x")-full.definitions().regions.at("windchime").center.x)<100);}
 // A genuine pre-reference expanded save migrates once, preserving private progression.
 {GameRuntime legacy(pathFile,true);auto state=legacy.snapshot(key);auto raw=Json::parse([&]{std::ifstream f(pathFile);return std::string(std::istreambuf_iterator<char>(f),{});}());raw["contentState"]["layoutVersion"]=1;raw["contentState"]["release"]=2;auto& list=std::get<Json::Array>(raw["players"].value);for(auto& player:list)if(player.str("id")==pid){player["position"]=Json{{"x",-4500},{"y",0},{"z",4800}};player["coins"]=777;}std::ofstream(pathFile)<<raw.dump();}
 double migratedX=0;
 {GameRuntime full(pathFile);auto state=full.snapshot(key);yes(state.at("player").num("coins")==777);yes(state.at("content").at("relations").at("windchime_merchant").num("score")==1);migratedX=state.at("player").at("position").num("x");yes(std::abs(migratedX-full.definitions().regions.at("windchime").center.x)<1);full.save();}
 {GameRuntime full(pathFile);yes(full.snapshot(key).at("player").at("position").num("x")==migratedX);}
 std::filesystem::remove_all(pathFile.parent_path());
 std::cout<<"OK content integration: "<<checks<<" checks\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
