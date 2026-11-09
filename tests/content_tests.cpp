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
 // Village roaming is server-owned, continuous, grounded, and pauses under a conversation lease.
 {GameRuntime village;auto s=create(village),second=create(village);auto key=s.str("token"),pid=s.at("player").str("id");
  auto home=village.definitions().npcs.at("windchime_guild").position;
  village.world().players.at(pid).position=home;village.world().players.at(pid).position.z+=2;
  auto talk=cmd(village,s,"npc_talk",Json{{"target","windchime_guild"}});bool talking=false;
  for(const auto& n:talk.at("content").at("npcs").arrayItems()){if(n.str("id")=="windchime_guild")talking=n.str("mode")=="talk";}yes(talking);
  for(int i=0;i<60;i++){village.tick(.05);}yes(distance(home,village.definitions().npcs.at("windchime_guild").position)<.001);
  auto pid2=second.at("player").str("id");village.world().players.at(pid2).position=village.world().players.at(pid).position;
  no([&]{cmd(village,second,"npc_talk",Json{{"target","windchime_guild"}});});
  auto topic=contentEntry("profiles","windchime_guild")->at("conversation_topics").arrayItems()[0];
  no([&]{cmd(village,second,"npc_chat",Json{{"target","windchime_guild"},{"topic",topic}});});
  cmd(village,second,"npc_leave",Json{{"target","windchime_guild"}});village.tick(.05);
  yes(distance(home,village.definitions().npcs.at("windchime_guild").position)<.001);
  auto old=village.exportBackup(key).dump();no([&]{cmd(village,s,"npc_chat",Json{{"target","windchime_guild"},{"topic","invented"}});});yes(old==village.exportBackup(key).dump());
  auto profile=contentEntry("profiles","windchime_guild");auto chat=cmd(village,s,"npc_chat",Json{{"target","windchime_guild"},{"topic",profile->at("conversation_topics").arrayItems()[0]}});yes(chat.at("content").at("relations").at("windchime_guild").str("dialogue").find("名冊")!=std::string::npos);
  cmd(village,s,"npc_leave",Json{{"target","windchime_guild"}});bool moved=false;auto prev=home;
  for(int i=0;i<400;i++){village.tick(.05);auto next=village.definitions().npcs.at("windchime_guild").position;yes(distance(prev,next)<=.043);yes(std::abs(next.y-groundBelow(next,village.definitions()))<.001);yes(!blocked(next,cityBuildings(village.definitions())));moved=moved||distance(home,next)>.2;prev=next;}yes(moved);
  village.world().players.at(pid).position=prev;cmd(village,s,"npc_talk",Json{{"target","windchime_guild"}});
  village.world().players.at(pid).position.x+=25;village.tick(.05);
  const auto ended=village.snapshot(key);for(const auto& n:ended.at("content").at("npcs").arrayItems())if(n.str("id")=="windchime_guild")yes(n.str("mode")!="talk");
  old=village.exportBackup(key).dump();no([&]{cmd(village,s,"npc_talk",Json{{"target","windchime_guild"}});});yes(old==village.exportBackup(key).dump());
 }
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
 for(auto& st:ending.at("content").at("stories").arrayItems()){if(st.str("id")==qid)found=st.at("progress").flag("done");}
 yes(found);
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
 {GameRuntime full;const auto& c=full.definitions();const auto& port=c.regions.at("tidehaven");yes(port.center.y==4);yes(groundBelow(cityArrival(port),c)==4);yes(atlasWater({(1180-633)*64.0,0,(550-578)*64.0}));yes(atlasHeight({(400-633)*64.0,0,(100-578)*64.0})>600);yes(atlasSurface().at("harbors").arrayItems().size()==3);}
 // Terrain v2 preserves reference anchors and shares native authoring tiles with gameplay.
 {GameRuntime full;auto session=create(full);const auto p=full.world().players.at(session.at("player").str("id")).position;
  const auto before=full.exportBackup(session.str("token")).dump();const auto tile=full.terrain(Json{{"x",p.x},{"z",p.z}}).at("terrain");
  yes(tile.num("side")==65&&tile.num("step")==8&&tile.at("heights").arrayItems().size()==4225);yes(tile.num("version")==5);
  yes(tile.dump()==full.snapshot(session.str("token")).at("content").at("terrainPatch").dump());yes(before==full.exportBackup(session.str("token")).dump());
  const auto detail=full.terrain(Json{{"x",p.x},{"z",p.z},{"detail",true}}).at("terrain");yes(detail.num("side")==129&&detail.num("step")==8&&detail.at("heights").arrayItems().size()==16641);yes(before==full.exportBackup(session.str("token")).dump());
  for(int row:{0,32,64})for(int col:{0,32,64}){double expected=worldLandHeight({tile.num("x")+col*8,0,tile.num("z")+row*8},full.definitions());yes(std::abs(tile.at("heights").arrayItems()[row*65+col].number()-expected)<=.00051);}
  no([&]{full.terrain(Json{{"x",48001},{"z",0}});});no([&]{full.terrain(Json{{"x","wrong"},{"z",0}});});
  double low=1e9,high=-1e9;const auto center=full.definitions().regions.at("darkforest").center;
  for(int z=-4000;z<=4000;z+=500)for(int x=-4000;x<=4000;x+=500)if(std::hypot(x,z)<=4000){double h=atlasHeight({center.x+x,0,center.z+z});yes(std::isfinite(h)&&h>=0);low=std::min(low,h);high=std::max(high,h);}
  // Terrain samples on both sides of a tile boundary must use the same
  // continuous height function; rivers and coasts must stay at water level.
  for(int x=-2048;x<=2048;x+=256)for(int z=-2048;z<=2048;z+=256){
   const Vec3 a{static_cast<double>(x),0,static_cast<double>(z)};
   const double h=atlasHeight(a),repeat=atlasHeight(a);
   yes(std::isfinite(h)&&h>=0&&h==repeat);
   if(atlasWater(a))yes(h==0);
  }
  const Vec3 coastalWater{(1180-633)*64.0,0,(550-578)*64.0};
  yes(atlasWater(coastalWater)&&atlasHeight(coastalWater)==0);
  yes(high-low<=1.4001);const Vec3 ridge{(425-633)*64.0,0,(130-578)*64.0};yes(atlasHeight(ridge)>900);yes(atlasHeight(ridge)==atlasHeight(ridge));
 }
 // The village keeps safe service/building pads, with genuine relief outside them.
 {GameRuntime full;const auto& c=full.definitions();const auto village=c.regions.at("windchime").center;
  for(int z=-12;z<=12;z+=8)for(int x=-12;x<=12;x+=8)yes(std::abs(worldLandHeight({village.x+x,0,village.z+z},c)-village.y)<.00001);
  double variation=0;for(int d=112;d<=400;d+=16){Vec3 p{village.x+d,0,village.z+80};double h=worldLandHeight(p,c);variation=std::max(variation,std::abs(h-village.y));Vec3 q{p.x+1,0,p.z};yes(std::abs(worldLandHeight(q,c)-h)<1);}
  yes(variation>.5);int homes=0;for(const auto& b:cityBuildings(c))if(b.region=="windchime"&&b.style.ends_with("|house")&&homes++<5)yes(std::abs(b.position.y-worldLandHeight(b.position,c))<.00001);
 }
 // Village relief, protected foundations, shallow water and imported wall collision agree.
 {GameRuntime full;const auto& c=full.definitions();const auto center=c.regions.at("windchime").center;const auto& v=atlasSurface().at("starterVillage");
  for(const auto& pad:v.at("foundationPads").arrayItems())for(double dx:{-7.,0.,7.})for(double dz:{-7.,0.,7.})yes(std::abs(worldLandHeight({center.x+pad.num("x")+dx,0,center.z+pad.num("z")+dz},c)-pad.num("height"))<.00001);
  yes(worldLandHeight({center.x-67,0,center.z-58},c)>2);yes(worldLandHeight({center.x+25,0,center.z-90},c)>1.5);
  const auto& river=v.at("stream");double z=60,x=river.num("x")+river.num("meander")*std::sin(z/river.num("period"));yes(worldLandHeight({center.x+x,0,center.z+z},c)<river.num("waterY"));yes(worldLandHeight({center.x+river.num("x"),0,center.z},c)==0);
  auto bs=cityBuildings(c);int count=0;for(const auto& b:bs)if(b.style.ends_with("|starter_wall")){count++;yes(blocked({b.position.x,0,b.position.z},bs));yes(!blocked({b.position.x,b.position.y+b.height+.1,b.position.z},bs));yes(std::abs(groundBelow({b.position.x,b.position.y+b.height+.1,b.position.z},c)-(b.position.y+b.height))<.00001);}yes(count==6);int fences=0;for(const auto& b:bs)if(b.style.ends_with("|starter_fence")){fences++;yes(blocked({b.position.x,b.position.y,b.position.z},bs));yes(std::abs(groundBelow({b.position.x,b.position.y+b.height+.1,b.position.z},c)-(b.position.y+b.height))<.00001);Vec3 off{b.position.x+std::sin(b.yaw)*2,b.position.y,b.position.z+std::cos(b.yaw)*2};yes(!blocked(off,std::vector<Building>{b}));}yes(fences==105);
 }
 // A terrain-v2 save relocates only entity heights, keeping progression and homes.
 Id terrainResource,terrainMonster;
 {GameRuntime full(pathFile);full.save();auto raw=Json::parse([&]{std::ifstream f(pathFile);return std::string(std::istreambuf_iterator<char>(f),{});}());const auto village=full.definitions().regions.at("windchime").center;
  raw["contentState"]["terrainVersion"]=2;
  for(auto& p:std::get<Json::Array>(raw["players"].value))if(p.str("id")==pid){p["coins"]=1871;p["position"]=Json{{"x",village.x+240},{"y",0},{"z",village.z+80}};}
  for(auto& r:std::get<Json::Array>(raw["resources"].value))if(r.str("scene")=="world"&&terrainResource.empty()){terrainResource=r.str("id");r["position"]=Json{{"x",village.x+240},{"y",-25},{"z",village.z+80}};}
  for(auto& m:std::get<Json::Array>(raw["monsters"].value))if(m.str("scene")=="world"&&m.str("region")=="windchime"&&terrainMonster.empty()){terrainMonster=m.str("id");m["position"]=Json{{"x",village.x+240},{"y",-25},{"z",village.z+80}};m["home"]=m.at("position");}
  std::ofstream(pathFile)<<raw.dump();
 }
 {GameRuntime full(pathFile);const auto& c=full.definitions();const auto p=full.world().players.at(pid);yes(p.coins==1871);yes(std::abs(p.position.y-worldLandHeight(p.position,c))<.00001);yes(!terrainResource.empty()&&!terrainMonster.empty());const auto r=full.world().resources.at(terrainResource);yes(std::abs(r.position.y-worldLandHeight(r.position,c))<.00001);const auto m=full.world().monsters.at(terrainMonster);yes(std::abs(m.position.y-worldLandHeight(m.position,c))<.00001);full.save();}
 {GameRuntime full(pathFile);yes(full.world().players.at(pid).coins==1871);std::ifstream f(pathFile);const auto raw=Json::parse(std::string(std::istreambuf_iterator<char>(f),{}));yes(raw.at("contentState").num("terrainVersion")==5);for(const auto& m:raw.at("monsters").arrayItems())if(m.str("id")==terrainMonster){const auto& p=m.at("home");Vec3 home{p.num("x"),p.num("y"),p.num("z")};yes(std::abs(home.y-worldLandHeight(home,full.definitions()))<.00001);}}
 // World house poses use owner authority, optimistic revisions, receipts and durable state.
 {const auto file=pathFile.parent_path()/"house-poses.json";Id ownerKey;Json pose{{"position",Json::Array{-6272,0,-14592}},{"modelPosition",Json::Array{1.13,-.73,2.08}},{"rotation",Json::Array{.234,-.81,.12}},{"scale",Json::Array{.82,.82,.82}},{"rotationOrder","XYZ"},{"layoutSeed",59231}};
  {GameRuntime full(file);auto owner=create(full),guest=create(full);ownerKey=owner.str("token");Json body{{"type","village_pose_save"},{"index",1},{"revision",0},{"pose",pose},{"request","pose-first"}};
   no([&]{full.command(guest.str("token"),body);});auto saved=full.command(ownerKey,body);yes(saved.at("content").at("villageLayout").num("revision")==1);yes(saved.at("content").at("villageLayout").at("poses").at("1").dump()==pose.dump());full.command(ownerKey,body);yes(full.catalog().at("villageLayout").num("revision")==1);
   auto before=full.exportBackup(ownerKey).dump();body["request"]="stale-pose";no([&]{full.command(ownerKey,body);});yes(before==full.exportBackup(ownerKey).dump());body["revision"]=1;body["request"]="invalid-pose";body["pose"]["scale"]=Json::Array{0,1,1};no([&]{full.command(ownerKey,body);});yes(before==full.exportBackup(ownerKey).dump());full.save();
  }
  {std::ifstream f(file);auto raw=Json::parse(std::string(std::istreambuf_iterator<char>(f),{}));f.close();raw["contentState"]["terrainVersion"]=3;std::ofstream(file)<<raw.dump();}
  {GameRuntime full(file);yes(full.snapshot(ownerKey).at("content").at("villageLayout").at("poses").at("1").dump()==pose.dump());yes(full.catalog().at("villageLayout").num("revision")==1);}
 }
 std::filesystem::remove_all(pathFile.parent_path());
 std::cout<<"OK content integration: "<<checks<<" checks\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
