#include "support.hpp"
#include "aetherwild/runtime.hpp"
namespace aw {
using namespace content_detail;
// Read-only projection: no save, release change, or private player data.
Json GameRuntime::adminMap(const Id& key)const{
 require(profiles_.contains(key),"登入已失效");
 const auto& p=world_.players.at(profiles_.at(key).player);
 auto role=social_.accounts.at(p.accountId).role;
 require(role==Role::Owner||role==Role::Admin,"需要管理員權限");
 Json j{{"ok",true},{"expanded",contentState_.flag("expanded")},{"release",contentState_.num("release",1)},{"planned",Json::array()},{"current",Json::array()},{"roads",Json::array()}};
 j["atlas"]=contentBundle().at("atlas_layout");
 for(const auto&[id,r]:contentBundle().at("regions").objectItems()){
  j["planned"].push(Json{{"id",id},{"name",r.str("name")},{"center",r.at("center")},{"radius",r.num("radius_m",850)},{"city",catalog_.regions.contains(id)&&!catalog_.regions.at(id).wilderness},{"urbanRadius",catalog_.regions.contains(id)?urbanRadius(catalog_.regions.at(id)):0},{"layer",r.str("layer")},{"release",r.num("release",1)},{"open",contentState_.flag("expanded")&&r.num("release",1)<=contentState_.num("release",1)}});
 }
 for(const auto&[id,r]:catalog_.regions){
  auto definition=contentEntry("regions",id);
  bool open=!contentState_.flag("expanded")||(definition&&definition->num("release",1)<=contentState_.num("release",1));
  j["current"].push(Json{{"id",id},{"name",r.name},{"center",vec(r.center)},{"radius",r.radius},{"city",!r.wilderness},{"urbanRadius",urbanRadius(r)},{"open",open}});
 }
 for(const auto&[id,r]:contentBundle().at("roads").objectItems())j["roads"].push(Json{{"a",r.str("a")},{"b",r.str("b")}});
 return j;
}
void GameRuntime::applyWorldPatch(){
 const bool expanded=contentState_.flag("expanded");
 const bool remap=expanded&&contentState_.num("layoutVersion")<2;
 const bool terrainChanged=contentState_.num("terrainVersion")<4;
 contentState_["release"]=5;
 if(!expanded)startExpandedWorld();
 else if(remap){
  const auto& previous=contentBundle().at("atlas_layout").at("previousRegions");
  auto shift=[&](Vec3 p,const Id& id){if(!previous.contains(id)||!catalog_.regions.contains(id))return p;const auto& old=previous.at(id).at("center");const auto& next=catalog_.regions.at(id).center;p.x+=next.x-old.num("x");p.z+=next.z-old.num("z");p.y=std::max(p.y,next.y);p.y=groundBelow(p,catalog_);return p;};
  auto nearest=[&](Vec3 p){Id result="windchime";double best=1e30;for(const auto&[id,r]:previous.objectItems()){if(id=="aw_underlost")continue;const auto& c=r.at("center");double d=std::hypot(p.x-c.num("x"),p.z-c.num("z"));if(d<best){best=d;result=id;}}return result;};
  for(auto&[id,p]:world_.players)if(p.scene=="world")p.position=shift(p.position,nearest(p.position));
  for(auto&[id,m]:world_.monsters)if(m.scene=="world"){m.position=shift(m.position,m.region);homes_[id]=m.position;}
  for(auto&[id,r]:world_.resources)r.position=shift(r.position,nearest(r.position));
 }
 contentState_["layoutVersion"]=2;contentState_["terrainVersion"]=4;contentState_["expanded"]=true;catalog_.contentRelease=5;
 for(const auto&[rid,r]:catalog_.regions)if(rid!="aw_underlost"){int index=0;for(const auto&[code,definition]:catalog_.resources){const auto id="resource_"+rid+"_"+code;if(world_.resources.contains(id))continue;double a=++index*2.399963;Vec3 pos{r.center.x+std::cos(a)*(r.radius+30),0,r.center.z+std::sin(a)*(r.radius+30)};pos.y=worldLandHeight(pos,catalog_);ResourceNode node;node.id=id;node.definition=code;node.position=pos;world_.resources.emplace(id,node);}}

 if(!expanded||remap||terrainChanged){contentState_["npcPoses"]=Json::object();contentState_["npcHomes"]=Json::object();configureContent(true);for(auto&[id,p]:world_.players)if(p.scene=="world"){auto&m=motion_[id];m.velocityY=0;m.lift.clear();m.grounded=true;p.position.y=std::max(p.position.y,catalog_.regions.at(p.lastCity).center.y);p.position.y=groundBelow(p.position,catalog_);m.peak=p.position.y;}
  // Existing village entities must follow the same changed ground, including
  // dormant resources and monster homes which are not advanced every frame.
  const auto center=catalog_.regions.at("windchime").center;
  const auto affected=[&](Vec3 p){return planeDistance(p,center)<2000;};
  for(auto&[id,r]:world_.resources)if(r.scene=="world"&&affected(r.position))r.position.y=worldLandHeight(r.position,catalog_);
  for(auto&[id,m]:world_.monsters)if(m.scene=="world"){
   if(affected(m.position))m.position.y=worldLandHeight(m.position,catalog_);
   if(homes_.contains(id)&&affected(homes_.at(id)))homes_[id].y=worldLandHeight(homes_.at(id),catalog_);
  }
 }
}
void GameRuntime::configureContent(bool expanded){
 if(!contentState_.contains("expanded"))contentState_=initialContent();
 catalog_=makeCatalog();installContent(catalog_);if(expanded){expandContentWorld(catalog_);catalog_.contentRelease=static_cast<int>(contentState_.num("release",1));}buildings_=buildings(catalog_);
 positionNpcs();
}
void GameRuntime::checkContentRegion(const Id& region)const{
 if(!contentState_.flag("expanded"))return;
 auto r=contentEntry("regions",region);require(r&&r->num("release",1)<=contentState_.num("release",1),"此區尚未開放，請查看世界通行告示");
}
bool contentPositionAllowed(const Catalog& catalog,Vec3 p){
 if(catalog.worldLimit<=1800)return true;
 if(!finite(p)||std::abs(p.x)>catalog.worldLimit||std::abs(p.z)>catalog.worldLimit)return false;
 if(catalog.contentRelease>=5)return true;
 int release=catalog.contentRelease;const auto& regions=contentBundle().at("regions");
 double nearest=1e30;bool allowed=false;
 for(auto&[id,v]:regions.objectItems()){
  if(v.str("layer")=="underground")continue;
  auto c=v.at("center");double d=std::hypot(p.x-c.num("x"),p.z-c.num("z"));bool open=v.num("release",1)<=release;
  if(!open&&d<v.num("radius_m")+200)return false;
  if(d<nearest){nearest=d;allowed=open;}
 }
 if(allowed)return true;
 // Verified open corridors join release-one regions without entering the closed capital.
 for(auto&[id,e]:contentBundle().at("roads").objectItems()){
  if(e.num("opens_in_release",5)>release)continue;
  Json::Array points;if(e.contains("waypoints"))points=e.at("waypoints").arrayItems();else points={regions.at(e.str("a")).at("center"),regions.at(e.str("b")).at("center")};
  for(std::size_t i=1;i<points.size();i++){
   const auto&a=points[i-1];const auto&b=points[i];double dx=b.num("x")-a.num("x"),dz=b.num("z")-a.num("z"),len=dx*dx+dz*dz;
   double t=len?std::clamp(((p.x-a.num("x"))*dx+(p.z-a.num("z"))*dz)/len,0.0,1.0):0;
   if(std::hypot(p.x-a.num("x")-t*dx,p.z-a.num("z")-t*dz)<450)return true;
  }
 }
 return false;
}
bool GameRuntime::contentAllowed(Vec3 p)const{return contentPositionAllowed(catalog_,p);}
void GameRuntime::startExpandedWorld(){
 require(!contentState_.flag("expanded"),"擴建世界已啟用");
 contentState_["layoutVersion"]=2;
 auto old=catalog_;contentState_["npcPoses"]=Json::object();configureContent(true);contentState_["expanded"]=true;contentState_["npcHomes"]=Json::object();
 for(auto&[id,p]:world_.players){
  if(p.scene!="world")continue;
  auto region=old.regions.contains(p.lastCity)?p.lastCity:Id("windchime");auto a=old.regions.at(region).center,b=catalog_.regions.at(region).center;
  p.position.x+=b.x-a.x;p.position.z+=b.z-a.z;p.position.y+=b.y-a.y;
  if(!contentAllowed(p.position)){p.position=cityArrival(catalog_.regions.at("windchime"));p.lastCity="windchime";}
  auto& motion=motion_[id];motion.velocityY=0;motion.lift.clear();motion.peak=p.position.y;motion.grounded=true;
 }
 for(auto&[id,m]:world_.monsters)if(m.scene=="world"&&old.regions.contains(m.region)){
  auto a=old.regions.at(m.region).center,b=catalog_.regions.at(m.region).center;m.position.x+=b.x-a.x;m.position.z+=b.z-a.z;
  if(!m.boss){double angle=std::atan2(m.position.z-b.z,m.position.x-b.x),rad=catalog_.regions.at(m.region).radius+40;m.position.x=b.x+std::cos(angle)*rad;m.position.z=b.z+std::sin(angle)*rad;}
  m.position.y=worldLandHeight(m.position,catalog_);homes_[id]=m.position;
 }
 for(auto&[id,n]:world_.resources){
  Id region="windchime";double nearest=1e30;for(auto&[rid,r]:old.regions){auto d=planeDistance(n.position,r.center);if(d<nearest){region=rid;nearest=d;}}
  auto a=old.regions.at(region).center,b=catalog_.regions.at(region).center;n.position.x+=b.x-a.x;n.position.z+=b.z-a.z;n.position.y=worldLandHeight(n.position,catalog_);n.reservedBy.reset();
 }
 for(auto&[bid,b]:catalog_.bosses){bool found=false;for(auto&[id,m]:world_.monsters)if(m.definition==bid){found=true;m.position=b.position;homes_[id]=b.position;break;}
  if(!found){auto id=world_.nextId("raid");CombatSystem::spawn(world_,id,b.position,b.level,b.region,true,bid);auto&m=world_.monsters.at(id);m.vitals.hp=m.vitals.maxHp=b.hp;m.attack=b.attack;homes_[id]=b.position;}
 }
 // Seed sparse exploration encounters in new regions, even closed ones remain dormant.
 for(auto&[rid,r]:catalog_.regions)if(!old.regions.contains(rid)&&rid!="aw_underlost"){
  for(int i=0;i<5;i++){double a=i*1.256637;Vec3 p{r.center.x+std::cos(a)*(r.radius+45),0,r.center.z+std::sin(a)*(r.radius+45)};auto id=world_.nextId("wild");p.y=worldLandHeight(p,catalog_);CombatSystem::spawn(world_,id,p,r.level,rid);homes_[id]=p;}
 }
}
bool GameRuntime::contentCommand(Player& p,const Json& j){
 auto type=j.str("type"),target=j.str("target"),request=j.str("request");
 if(type=="village_pose_save"){
  require(social_.accounts.at(p.accountId).role==Role::Owner,"只有擁有者可保存世界房屋設定");
  auto index=j.at("index").integer();require(index>=1&&index<=5,"房屋編號不正確");
  require(j.at("revision").integer()==contentState_.num("villagePoseRevision"),"世界房屋設定已更新，請重新讀取後再保存");
  const auto& pose=j.at("pose");Json saved=Json::object();
  for(const auto& field:{"position","modelPosition","rotation","scale"}){
   const auto& a=pose.at(field).arrayItems();require(a.size()==3,"房屋向量必須有三個數值");
   for(const auto& v:a){double n=v.number();require(std::isfinite(n)&&std::abs(n)<=100000,"房屋數值超出範圍");if(std::string(field)=="scale")require(n>=.0001&&n<=1000,"房屋縮放超出範圍");}
   saved[field]=pose.at(field);
  }
  const auto& position=saved.at("position").arrayItems();require(std::abs(position[0].number())<=catalog_.worldLimit&&std::abs(position[2].number())<=catalog_.worldLimit,"房屋位置超出世界範圍");
  require(pose.str("rotationOrder","XYZ")=="XYZ","房屋旋轉順序不相容");saved["rotationOrder"]="XYZ";
  if(!contentState_.contains("villagePoses"))contentState_["villagePoses"]=Json::object();
  contentState_["villagePoses"][std::to_string(index)]=saved;contentState_["villagePoseRevision"]=contentState_.num("villagePoseRevision")+1;return true;
 }
 if(type=="skill_slot"){
  require(p.vitals.alive(),"倒下時無法調整技能欄");auto index=j.at("slot").integer();require(index>=0&&index<5,"技能欄僅有五格");auto skill=j.str("skill");
  require(skill.empty()||(p.learnedSkills.contains(skill)&&catalog_.skills.contains(skill)&&catalog_.skills.at(skill).effect!="dash"&&(!catalog_.skillOwners.contains(skill)||catalog_.skillOwners.at(skill)==p.profession)),"尚未學會或不屬於目前職業的技能");
  auto slots=equippedSkills(p);for(int i=0;i<5;i++)if(i!=index&&!skill.empty())require(slots.arrayItems()[i].string()!=skill,"同一技能不能重複裝備");Json updated=Json::array();for(int i=0;i<5;i++)updated.push(i==index?skill:slots.arrayItems()[i].string());slots=updated;
  if(!contentState_.contains("skillLoadout"))contentState_["skillLoadout"]=Json::object();
  contentState_["skillLoadout"][p.id]=slots;return true;
 }

 auto admin=[&]{auto role=social_.accounts.at(p.accountId).role;require(role==Role::Owner||role==Role::Admin,"需要管理員權限");};
 if(type=="expansion_start"){admin();startExpandedWorld();return true;}
 if(type=="content_release"){
  admin();require(contentState_.flag("expanded"),"請先啟用擴建世界");int n=static_cast<int>(j.at("release").integer());
  require(n>=contentState_.num("release",1)&&n<=5,"只允許依序開放第1至5批");require(n<=contentState_.num("release",1)+1,"一次只開放下一批五區");contentState_["release"]=n;catalog_.contentRelease=n;return true;
 }
 if(type=="story_talk"||type=="story_choice"||type=="story_inspect"){
  auto q=contentEntry("regionalQuests",target);require(q,"未知區域故事");auto region=q->str("region");checkContentRegion(region);
  require(p.vitals.alive()&&p.scene=="world"&&catalog_.npcs.contains(q->str("giver")),"此故事尚無可互動場景");
  auto& story=playerObject(contentState_["storyProgress"],p.id);
  for(auto& pre:q->at("requires").at("quest_done").arrayItems())require(story.contains(pre.string())&&story.at(pre.string()).flag("done"),"先完成此區前一段故事");
  if(!story.contains(target))story[target]=Json{{"node","opening"},{"accepted",false},{"evidence",false},{"done",false}};
  auto& progress=story[target];require(!progress.flag("done"),"此故事已完成");
  auto giver=catalog_.npcs.at(q->str("giver"));
  if(type=="story_inspect"){
   require(progress.flag("accepted"),"先與委託人交談");auto anchor=cityArrival(catalog_.regions.at(region));anchor.x+=20;anchor.z+=12;
   require(distance(p.position,anchor)<=5,"請先走到本區調查桌");progress["evidence"]=true;progress["node"]="turnin";return true;
  }
  require(distance(p.position,giver.position)<=8&&clearSight(p.position,giver.position,buildings_),"請走到故事人物旁");
  if(type=="story_talk"){progress["node"]=progress.flag("accepted")?(progress.flag("evidence")?"turnin":"progress"):"opening";return true;}
  const Json* node=nullptr;for(auto&n:q->at("dialogue").at("nodes").arrayItems())if(n.str("id")==progress.str("node"))node=&n;require(node,"無效對話節點");
  const Json* selected=nullptr;for(auto&ch:node->at("choices").arrayItems())if(ch.str("id")==j.str("choice"))selected=&ch;require(selected,"無效對話選項");
  if(selected->contains("effects"))for(auto& effect:selected->at("effects").arrayItems()){
   if(effect.str("op")=="accept_quest")progress["accepted"]=true;
   if(effect.str("op")=="complete_quest"){
    require(progress.flag("accepted")&&progress.flag("evidence"),"先完成調查再回報");progress["done"]=true;progress["choice"]=selected->str("id");
    auto prof=contentEntry("profiles",giver.id);auto& rel=playerObject(contentState_["relations"],p.id);if(!rel.contains(giver.id))rel[giver.id]=NpcSystem::initial();if(prof)NpcSystem::event(rel[giver.id],*prof,"quest","story:"+target,NpcSystem::clock());
   }
  }
  progress["node"]=selected->str("next");return true;
 }
 if(type=="read_book"){
  require(p.vitals.alive()&&p.scene=="world","請在安全的世界場景閱讀");auto b=contentEntry("regionalBooks",target);require(b,"未知書籍");auto region=b->str("region");checkContentRegion(region);
  require(catalog_.regions.contains(region),"此書尚未放置");auto pos=cityArrival(catalog_.regions.at(region));pos.x+=12;
  require(distance(p.position,pos)<=10,"請先走到地方告示／藏記架旁");playerObject(contentState_["booksRead"],p.id)[target]=true;return true;
 }
 if(type=="class_attune"){
  nearbyNpc(p,j.str("npc"),"guild",catalog_);auto code=startingWeapon(catalog_,p.profession);auto claim="attune:"+p.profession;
  require(!p.processedRewards.contains(claim),"已領取此職業的訓練戰具");require(contentEntry("classes",p.profession),"未知傳承");
  p.inventory.push_back(InventorySystem::create(world_,catalog_,code));p.processedRewards.insert(claim);return true;
 }
 return npcCommand(p,j);
}

Json GameRuntime::equippedSkills(const Player& p)const{
 Json slots=Json::array();
 if(contentState_.contains("skillLoadout")&&contentState_.at("skillLoadout").contains(p.id)){
  const auto& saved=contentState_.at("skillLoadout").at(p.id).arrayItems();for(int i=0;i<5;i++){Id id=i<static_cast<int>(saved.size())?saved[i].string():Id{};if(!p.learnedSkills.contains(id)||!catalog_.skills.contains(id)||catalog_.skills.at(id).effect=="dash"||(catalog_.skillOwners.contains(id)&&catalog_.skillOwners.at(id)!=p.profession))id.clear();slots.push(id);}return slots;
 }
 for(const auto& id:p.learnedSkills)if(catalog_.skills.contains(id)&&catalog_.skills.at(id).effect!="dash"&&(!catalog_.skillOwners.contains(id)||catalog_.skillOwners.at(id)==p.profession)&&slots.arrayItems().size()<5)slots.push(id);
 while(slots.arrayItems().size()<5)slots.push("");
 return slots;
}
Json GameRuntime::contentSnapshot(const Player& p)const{
 auto now=NpcSystem::clock();Json out{{"expanded",contentState_.flag("expanded")},{"release",contentState_.num("release",1)},{"worldLimit",catalog_.worldLimit},{"relations",Json::object()},{"npcs",Json::array()},{"books",Json::array()},{"collections",Json::object()},{"blockers",Json::array()}};
 out["villageLayout"]=Json{{"revision",contentState_.num("villagePoseRevision")},{"poses",contentState_.contains("villagePoses")?contentState_.at("villagePoses"):Json::object()}};
 const Json* rel=nullptr;if(contentState_.at("relations").contains(p.id))rel=&contentState_.at("relations").at(p.id);
 for(auto&[id,n]:catalog_.npcs){
  if(distance(p.position,n.position)>180||p.scene!="world")continue;
  if(contentState_.flag("expanded")&&contentEntry("regions",n.region)->num("release")>contentState_.num("release",1))continue;
  auto profile=contentEntry("profiles",id);if(!profile)continue;
  auto a=rel&&rel->contains(id)?rel->at(id):NpcSystem::initial();auto v=NpcSystem::view(a,*profile,now);v["name"]=n.name;v["id"]=id;out["relations"][id]=v;
  auto life=contentState_.at("life").contains(id)?contentState_.at("life").at(id):Json{{"mode","work"}};
  out["npcs"].push(Json{{"id",id},{"position",vec(n.position)},{"mode",life.str("mode","work")}});
 }
 out["stories"]=Json::array();
 for(auto&[qid,q]:contentBundle().at("regionalQuests").objectItems()){
  if(!catalog_.npcs.contains(q.str("giver")))continue;
  const auto& giver=catalog_.npcs.at(q.str("giver"));
  if(p.scene!="world"||distance(p.position,giver.position)>180||!contentRegionOpen(catalog_,giver.region))continue;
  Json progress{{"node","opening"},{"accepted",false},{"evidence",false},{"done",false}};
  if(contentState_.at("storyProgress").contains(p.id)&&contentState_.at("storyProgress").at(p.id).contains(qid))progress=contentState_.at("storyProgress").at(p.id).at(qid);
  Json node=Json::object();for(auto& n:q.at("dialogue").at("nodes").arrayItems())if(n.str("id")==progress.str("node"))node=n;
  auto anchor=cityArrival(catalog_.regions.at(q.str("region")));anchor.x+=20;anchor.z+=12;
  out["stories"].push(Json{{"id",qid},{"title",q.str("title")},{"giver",giver.id},{"objective",q.at("objective").str("description")},{"position",vec(anchor)},{"giverPosition",vec(giver.position)},{"progress",progress},{"node",node}});
 }
 if(contentState_.at("collections").contains(p.id))out["collections"]=contentState_.at("collections").at(p.id);
 for(auto&[id,b]:contentBundle().at("regionalBooks").objectItems()){
  auto region=b.str("region");if(!catalog_.regions.contains(region))continue;
  if(contentState_.flag("expanded")&&b.num("release")>contentState_.num("release",1))continue;
  auto pos=cityArrival(catalog_.regions.at(region));pos.x+=12;
  if(p.scene=="world"&&distance(p.position,pos)<180){auto v=b;v["position"]=vec(pos);v["read"]=contentState_.at("booksRead").contains(p.id)&&contentState_.at("booksRead").at(p.id).flag(id);out["books"].push(v);}
 }
 if(contentState_.flag("expanded")&&p.scene=="world"){
  out["terrainPatch"]=terrainPatchAt(p.position.x,p.position.z);
 }
 // Collision-consistent visible boundary anchors, only near a closed edge.
 if(catalog_.contentRelease<5&&contentState_.flag("expanded")&&p.scene=="world")for(int i=0;i<32;i++){
  double angle=i*6.283185307/32;Vec3 q=p.position;double low=0,high=0;
  for(double step=50;step<=1200;step+=50){q.x=p.position.x+std::cos(angle)*step;q.z=p.position.z+std::sin(angle)*step;if(!contentAllowed(q)){high=step;low=step-50;break;}}
  if(!high)continue;
  for(int n=0;n<9;n++){double mid=(low+high)/2;q.x=p.position.x+std::cos(angle)*mid;q.z=p.position.z+std::sin(angle)*mid;if(contentAllowed(q))low=mid;else high=mid;}
  q.x=p.position.x+std::cos(angle)*low;q.z=p.position.z+std::sin(angle)*low;q.y=groundBelow(q,catalog_);out["blockers"].push(Json{{"position",vec(q)},{"reason","前方道路因區域封鎖未通，請循已開放驛路前進"}});
 }
 return out;
}
void GameRuntime::contentTick(double){
 if(std::fmod(world_.now,1.0)>.06)return;
 tickNpcs();
 // A newly closed or invalid location is relocated without damage; regular movement is blocked earlier.
 if(contentState_.flag("expanded"))for(auto&[id,p]:world_.players)if(p.scene=="world"&&!contentAllowed(p.position)){
  p.position=cityArrival(catalog_.regions.at("windchime"));p.lastCity="windchime";motion_[id].velocityY=0;motion_[id].grounded=true;
 }
}
}
