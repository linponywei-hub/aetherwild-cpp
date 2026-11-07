#include "aetherwild/content.hpp"
#include <chrono>
namespace aw {
const Json* contentEntry(const std::string& section,const Id& id) {
 const auto& b=contentBundle(); if(!b.contains(section)||!b.at(section).contains(id))return nullptr;return &b.at(section).at(id);
}
namespace {
std::string nullable(const Json& j,const std::string& k){return j.contains(k)&&std::holds_alternative<std::string>(j.at(k).value)?j.str(k):"";}
WeaponKind weaponType(const std::string& s){
 static const std::map<std::string,WeaponKind> kinds={{"sword",WeaponKind::Sword},{"staff",WeaponKind::Staff},{"bow",WeaponKind::Bow},{"sword_shield",WeaponKind::ShieldBlade},{"tower_shield",WeaponKind::ShieldBlade},{"hammer_shield",WeaponKind::ShieldBlade},{"spear_shield",WeaponKind::ShieldBlade},{"thorn_shield",WeaponKind::ShieldBlade},{"dagger_pair",WeaponKind::Daggers},{"gauntlets",WeaponKind::Gauntlets},{"instrument",WeaponKind::Instrument},{"totem",WeaponKind::Totem},{"cannon",WeaponKind::Cannon},{"toolgun",WeaponKind::Toolgun},{"chakram_pair",WeaponKind::Chakram},{"runic_hammer",WeaponKind::Hammer},{"spear",WeaponKind::Spear},{"focus_staff",WeaponKind::Focus},{"seal_focus",WeaponKind::Focus},{"book_focus",WeaponKind::Focus},{"greatsword",WeaponKind::Greatsword}};
 require(kinds.contains(s),"未註冊武器類型："+s);return kinds.at(s);
}
}
std::string weaponName(WeaponKind k){
 const char* names[]={"none","sword","staff","bow","shieldblade","daggers","gauntlets","instrument","totem","cannon","toolgun","chakram","hammer","spear","focus","greatsword"};
 auto i=static_cast<int>(k);require(i>=0&&i<16,"武器類型錯誤");return names[i];
}
bool magicWeapon(WeaponKind k){return k==WeaponKind::Staff||k==WeaponKind::Instrument||k==WeaponKind::Totem||k==WeaponKind::Focus;}
bool rangedWeapon(WeaponKind k){return magicWeapon(k)||k==WeaponKind::Bow||k==WeaponKind::Cannon||k==WeaponKind::Toolgun;}
Id startingWeapon(const Catalog& c,const Id& job){if(c.starterItems.contains(job))return c.starterItems.at(job);auto k=c.classes.at(job).weapon;return k==WeaponKind::Staff?"oakstaff":k==WeaponKind::Bow?"windbow":"ironblade";}
void installContent(Catalog& c){
 const auto& b=contentBundle();
 for(auto&[id,v]:b.at("classes").objectItems()){
  const auto role=v.str("role");c.classRoles[id]=role;
  if(!c.classes.contains(id)){
   ClassDefinition d;d.id=id;d.name=v.str("name");d.parent=nullable(v,"parent");d.level=v.flag("starting_class")?1:15;
   d.weapon=weaponType(v.at("weapon_types").arrayItems().at(0).string());
   d.base={role=="tank"?190.0:role=="assassin"?115.0:140.0,role=="healer"?160.0:110.0,role=="tank"?18.0:24.0,0,0,0};d.range=rangedWeapon(d.weapon)?24:5;
   c.classes.emplace(id,d);
  }
  auto& skills=c.classSkills[id];if(skills.empty())for(auto&s:v.at("skills").arrayItems())skills.push_back(s.string());
 }
 for(auto&[id,v]:b.at("skills").objectItems()){
  auto owner=v.str("owner");if(!c.classes.contains(owner)||v.str("shape")=="passive")continue;
  c.skillOwners[id]=owner;if(c.skills.contains(id))continue;
  auto index=std::find(c.classSkills.at(owner).begin(),c.classSkills.at(owner).end(),id)-c.classSkills.at(owner).begin();
  SkillDefinition s;s.id=id;s.name=v.str("name");s.mana=12+index*2;s.cooldown=6+index*2;s.range=24;s.radius=index==5?8:4;s.duration=5;
  if(index==1||index==4){s.effect="shield";s.shield=35+index*10;}
  else if(index==3){s.effect="guard";s.shield=20;s.cooldown=10;} // No fake dash through collisions.
  else{s.effect="blast";s.damage=35+index*15;}
  if(c.classRoles.at(owner)=="healer"&&(index==1||index==4||index==5)){s.effect="heal";s.heal=55+index*12;s.damage=0;}
  if(c.classRoles.at(owner)=="tank"&&index==2){s.effect="taunt";s.damage=0;s.radius=12;s.duration=4;}
  if(c.classRoles.at(owner)=="assassin"&&index==2){s.effect="freeze";s.duration=1.5;s.damage=35;}
  c.skills.emplace(id,s);
 }
 for(auto&[id,v]:b.at("equipment").objectItems()){
  if(c.items.contains(id)){c.items.at(id).name=v.str("proposed_name");continue;}
  if(v.str("slot")=="collectible")continue;
  auto slot=v.str("slot");ItemDefinition d;d.id=id;d.name=v.str("proposed_name");
  d.slot=slot=="weapon"?Slot::Weapon:slot=="armor"?Slot::Armor:slot=="helm"?Slot::Helm:slot=="boots"?Slot::Boots:Slot::Charm;
  if(slot=="weapon"){d.weapon=weaponType(v.str("type"));d.stats.attack=10;if(magicWeapon(d.weapon))d.stats.mp=12;}
  else{d.stats.hp=20;d.stats.defense=3;}
  auto cls=nullable(v.at("links"),"class");if(!cls.empty()){d.requiredLevel=c.classes.at(cls).level;if(slot=="weapon")c.starterItems[cls]=id;}
  auto set=nullable(v.at("links"),"set");if(!set.empty()){d.set=set;c.sets.try_emplace(set,SetDefinition{set,{},{}});}
  c.items.emplace(id,d);
 }
 // Weapon-compatible original world-boss drops for newly registered weapon kinds.
 for(auto&[bid,boss]:c.bosses)for(auto&[cid,cl]:c.classes){
  if(static_cast<int>(cl.weapon)<=3)continue;
  auto id=bid+"_"+weaponName(cl.weapon);if(c.items.contains(id))continue;
  ItemDefinition d;d.id=id;d.name=boss.name+"・"+cl.name+"戰具";d.slot=Slot::Weapon;d.weapon=cl.weapon;d.stats.attack=10+boss.level*.7;d.requiredLevel=boss.level;d.set=c.sets.contains("set_"+boss.region)?"set_"+boss.region:"aw_set_"+boss.region;c.items.emplace(id,d);
 }
 // Existing regions gain their authored resident identities; future-region NPCs wait for activation.
 int n=0;for(auto&[id,v]:b.at("profiles").objectItems()){
  auto region=v.str("region");if(!c.regions.contains(region)||c.npcs.contains(id))continue;
  auto pos=c.regions.at(region).center;double a=n++*2.399963;pos.x+=std::cos(a)*36;pos.z+=std::sin(a)*36;
  c.npcs.emplace(id,NpcDefinition{id,region,v.str("name"),v.str("service_role","resident"),pos});
 }
 c.validate();
}
void expandContentWorld(Catalog& c){
 const auto& b=contentBundle();const auto old=c.regions;c.worldLimit=32000;c.contentRelease=1;
 for(auto&[id,v]:b.at("regions").objectItems()){
  // The underground city is a separate instance, never a second overworld at 0,0.
  c.regionReleases[id]=static_cast<int>(v.num("release",1));
  auto p=v.at("center");bool under=v.str("layer")=="underground";
  auto center=Vec3{p.num("x"),id=="frosthold"?260.0:0,p.num("z")};
  if(under)center={800,-200,800}; // metadata anchor only; unavailable overworld service.
  c.regions[id]={id,v.str("name"),center,v.num("radius_m"),static_cast<int>(v.at("level_band").arrayItems()[0].integer()),v.str("kind")=="city",v.str("kind")=="wild"||under};
 }
 for(auto&[id,d]:c.dungeons)if(old.contains(d.region)){
  auto a=old.at(d.region).center,z=c.regions.at(d.region).center;d.entrancePosition.x+=z.x-a.x;d.entrancePosition.z+=z.z-a.z;
 }
 for(auto&[id,bb]:c.bosses)if(old.contains(bb.region)){
  auto a=old.at(bb.region).center,z=c.regions.at(bb.region).center;bb.position.x+=z.x-a.x;bb.position.z+=z.z-a.z;
 }
 // Every authored new dungeon has a server-owned entrance and ordinary objective rules.
 for(auto&[id,v]:b.at("newDungeons").objectItems()){
  if(c.dungeons.contains(id))continue;
  auto region=v.str("region");auto p=v.at("suggested_anchor");
  c.dungeons.emplace(id,DungeonDefinition{id,region,v.str("name"),"seals","cave",std::clamp(static_cast<int>(v.num("level_hint",1)),1,100),4,"","",{p.num("x"),0,p.num("z")}});
 }
 for(auto&[id,v]:b.at("monsters").objectItems())if((v.str("kind")=="world_boss"||v.str("kind")=="optional_world_boss")&&!c.bosses.contains(id)){
  auto r=c.regions.at(v.str("region"));auto pos=r.center;pos.x+=r.radius+100;int level=std::min(95,r.level+4);
  c.bosses.emplace(id,BossDefinition{id,r.id,v.str("name"),pos,level,18000+level*1800.0,65+level*5.0});
 }
 for(auto&[id,boss]:c.bosses){auto&r=c.regions.at(boss.region);if(!r.wilderness){double dx=boss.position.x-r.center.x,dz=boss.position.z-r.center.z,len=std::max(1.0,std::hypot(dx,dz));boss.position.x=r.center.x+dx/len*(r.radius+100);boss.position.z=r.center.z+dz/len*(r.radius+100);}boss.position.y=worldLandHeight(boss.position,c);}
 for(auto&[id,n]:c.npcs){auto r=c.regions.at(n.region);n.position=r.center;}
 installContent(c);
}
double worldLandHeight(Vec3 p,const Catalog& c){
 if(c.worldLimit<=1800)return 0;
 auto hill=[&](double x,double z,double width,double height){double dx=(p.x-x)/width,dz=(p.z-z)/width;return height*std::exp(-(dx*dx+dz*dz));};
 double y=5*std::sin(p.x/1300)*std::cos(p.z/1700)+2*std::sin((p.x+p.z)/600);
 y+=hill(-15500,-20500,5200,460)+hill(-6500,-27000,4400,180)+hill(-17000,-10500,4200,130);
 y+=hill(9500,8500,2200,65)+hill(23500,-25000,3500,65);
 // Preserve low-relief dark forest; major routes are blended into gentle ground.
 double dark=std::clamp((5000-std::hypot(p.x+25000,p.z+2000))/2000.0,0.0,1.0);y*=1-dark;
 for(auto&[id,r]:c.regions){if(r.wilderness)continue;double d=std::hypot(p.x-r.center.x,p.z-r.center.z);double t=std::clamp((d-r.radius-30)/1000,0.0,1.0);t=t*t*(3-2*t);y=r.center.y*(1-t)+y*t;}
 return y;
}
bool contentRegionOpen(const Catalog& c,const Id& id){return !c.regionReleases.contains(id)||c.regionReleases.at(id)<=c.contentRelease;}
double NpcSystem::clock(){return std::chrono::duration<double>(std::chrono::system_clock::now().time_since_epoch()).count();}
Json NpcSystem::initial(){return Json{{"score",0},{"lockUntil",0},{"repair",false},{"lastAttack",-1e12},{"lastChat",-1e12},{"day",-1},{"daily",0},{"chats",0},{"shopping",0},{"topics",Json::object()},{"receipts",Json::object()},{"memory",Json::array()},{"questAccepted",false},{"questDone",false},{"giftClaimed",false}};}
int NpcSystem::score(const Json& a){return static_cast<int>(a.num("score"));}
std::string NpcSystem::tier(int n){return n<=-40?"hostile":n<0?"wary":n<20?"neutral":n<40?"familiar":n<70?"trusted":"confidant";}
void NpcSystem::event(Json& a,const Json& profile,const std::string& kind,const Id& receipt,double now,int amount){
 require(std::isfinite(now)&&now>=0&&!receipt.empty(),"關係事件錯誤");if(a.at("receipts").contains(receipt))return;
 auto day=static_cast<std::int64_t>(std::floor((now+8*3600)/86400));if(a.num("day")!=day){a["day"]=day;a["daily"]=0;a["chats"]=0;a["shopping"]=0;a["topics"]=Json::object();}
 int delta=0;
 if(kind=="attack"){
  if(now-a.num("lastAttack")>=5){delta=-25;a["lastAttack"]=now;}
  a["repair"]=true;a["lockUntil"]=std::max(a.num("lockUntil"),now+86400);
 }else if(kind=="repair"){
  require(a.flag("repair")&&now>=a.num("lockUntil"),"補救期限未到");a["repair"]=false;delta=5;
 }else if(!a.flag("repair")){
  if(kind=="chat"&&now-a.num("lastChat")>=21600&&a.num("chats")<2){delta=1;a["lastChat"]=now;a["chats"]=a.num("chats")+1;}
  else if(kind=="purchase"){delta=std::min({2,std::max(0,amount/100),4-static_cast<int>(a.num("shopping"))});a["shopping"]=a.num("shopping")+delta;}
  else if(kind=="quest")delta=6;
  else if(kind=="private_quest")delta=12;
 }
 if(delta>0){delta=std::min(delta,20-static_cast<int>(a.num("daily")));a["daily"]=a.num("daily")+delta;}
 a["score"]=std::clamp(score(a)+delta,-100,100);a["receipts"][receipt]=true;
 auto& mem=std::get<Json::Array>(a["memory"].value);mem.push_back(Json{{"kind",kind},{"at",now},{"delta",delta},{"npc",profile.str("id")}});if(mem.size()>32)mem.erase(mem.begin());
}
Json NpcSystem::view(const Json& a,const Json& profile,double now){
 bool safe=!a.flag("repair")&&now>=a.num("lockUntil");auto key=safe?tier(score(a)):score(a)<=-40?"hostile":"wary";
 Json topics=Json::array();int i=0;for(auto&t:profile.at("conversation_topics").arrayItems())if(i++<2||score(a)>=40)topics.push(t);
 return Json{{"score",score(a)},{"tier",tier(score(a))},{"restricted",!safe},{"dialogue",safe&&a.contains("lastLine")?a.str("lastLine"):profile.at("dialogue").str(key)},{"topics",topics},{"canAccept",safe&&score(a)>=40&&!a.flag("questAccepted")},{"questAccepted",a.flag("questAccepted")},{"questDone",a.flag("questDone")},{"canGift",safe&&score(a)>=70&&a.flag("questDone")&&!a.flag("giftClaimed")},{"giftClaimed",a.flag("giftClaimed")},{"discount",safe&&score(a)>=20?profile.at("high_affinity").num("discount_percent_cap"):0}};
}
}
