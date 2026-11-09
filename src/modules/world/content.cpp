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
 const auto& b=contentBundle();const auto old=c.regions;c.worldLimit=48000;c.contentRelease=1;
 for(auto&[id,v]:b.at("regions").objectItems()){
  // The underground city is a separate instance, never a second overworld at 0,0.
  c.regionReleases[id]=static_cast<int>(v.num("release",1));
  auto p=v.at("center");bool under=v.str("layer")=="underground";
  auto center=Vec3{p.num("x"),id=="frosthold"?850.0:(id=="cinder"?80.0:(id=="tidehaven"||id=="aw_westport"||id=="aw_southmill"?4.0:0)),p.num("z")};
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
 for(auto&[id,d]:c.dungeons)d.entrancePosition.y=worldLandHeight(d.entrancePosition,c);
 for(auto&[id,boss]:c.bosses){auto&r=c.regions.at(boss.region);if(!r.wilderness){double dx=boss.position.x-r.center.x,dz=boss.position.z-r.center.z,len=std::max(1.0,std::hypot(dx,dz));boss.position.x=r.center.x+dx/len*(r.radius+100);boss.position.z=r.center.z+dz/len*(r.radius+100);}boss.position.y=worldLandHeight(boss.position,c);}
 for(auto&[id,n]:c.npcs){auto r=c.regions.at(n.region);n.position=r.center;}
 installContent(c);
}
namespace {
struct VillagePad {double x,z,w,d,feather;};
struct VillageMound {double x,z,height,radius;};
struct VillageWall {double x,z,width,depth;};
struct VillageStream {double x,meander,period,minZ,maxZ,halfWidth,bankWidth,bedY,bridgeDepth,bridgeWidth;};
struct VillageRelief {Id region;double flat,blend,radius,safe;VillageStream stream;std::vector<VillagePad> pads;std::vector<VillageMound> mounds;std::vector<VillageWall> walls;};
const VillageRelief& villageRelief(){
 static const VillageRelief data=[] {const auto& v=atlasSurface().at("starterVillage");const auto& r=v.at("stream");VillageRelief out;
  out.region=v.str("region");out.flat=v.num("flatRadius");out.blend=v.num("blendMeters");out.radius=v.num("detailRadius");out.safe=v.num("safeRadius");
  out.stream={r.num("x"),r.num("meander"),r.num("period"),r.num("zMin"),r.num("zMax"),r.num("halfWidth"),r.num("bankWidth"),r.num("bedY"),r.num("bridgeHalfDepth"),r.num("bridgeHalfWidth")};
  for(const auto& p:v.at("foundationPads").arrayItems())out.pads.push_back({p.num("x"),p.num("z"),p.num("halfWidth"),p.num("halfDepth"),p.num("feather")});
  for(const auto& p:v.at("mounds").arrayItems())out.mounds.push_back({p.num("x"),p.num("z"),p.num("height"),p.num("radius")});
  for(const auto& p:v.at("walls").arrayItems())out.walls.push_back({p.num("x"),p.num("z"),p.num("width"),p.num("depth")});
  return out;
 }();return data;
}
double villageSmooth(double a,double b,double q){double t=std::clamp((q-a)/(b-a),0.0,1.0);return t*t*(3-2*t);}
}
double worldLandHeight(Vec3 p,const Catalog& c){
 if(c.worldLimit<=1800)return 0;
 const auto& v=villageRelief();double y=atlasHeight(p);
 for(auto&[id,r]:c.regions){if(r.wilderness||id=="tidehaven"||id=="aw_westport"||id=="aw_southmill")continue;double d=std::hypot(p.x-r.center.x,p.z-r.center.z);const bool village=id==v.region;double t=std::clamp((d-(village?v.flat:r.radius+30))/(village?v.blend:1000),0.0,1.0);t=t*t*(3-2*t);y=r.center.y*(1-t)+y*t;}
 const auto center=c.regions.at(v.region).center;double x=p.x-center.x,z=p.z-center.z,distance=std::hypot(x,z);if(distance>=v.radius||distance<=v.safe)return y;
 double mask=villageSmooth(v.safe,v.safe+8,distance)*villageSmooth(5,9,std::min(std::abs(x),std::abs(z)));
 for(const auto& pad:v.pads){double edge=std::max(std::abs(x-pad.x)-pad.w,std::abs(z-pad.z)-pad.d);mask*=villageSmooth(0,pad.feather,edge);}
 if(mask>0){double rise=0;for(const auto& mound:v.mounds){double dx=(x-mound.x)/mound.radius,dz=(z-mound.z)/mound.radius;rise+=mound.height*std::exp(-(dx*dx+dz*dz)*1.5);}y+=rise*mask;}
 const auto& r=v.stream;double cx=r.x+r.meander*std::sin(z/r.period);const bool bridge=std::abs(z)<=r.bridgeDepth&&std::abs(x-r.x)<=r.bridgeWidth;
 if(z>r.minZ&&z<r.maxZ&&!bridge){double bank=1-villageSmooth(r.halfWidth*.65,r.halfWidth+r.bankWidth,std::abs(x-cx));double end=villageSmooth(r.minZ,r.minZ+15,z)*(1-villageSmooth(r.maxZ-15,r.maxZ,z));y=y*(1-bank*end)+r.bedY*bank*end;}
 if(bridge)y=center.y;
 for(const auto& w:v.walls){double edge=std::max(std::abs(x-w.x)-w.width/2-4,std::abs(z-w.z)-w.depth/2-4);double flat=1-villageSmooth(0,4,edge);y=y*(1-flat)+center.y*flat;}
 return y;
}
bool contentRegionOpen(const Catalog& c,const Id& id){return !c.regionReleases.contains(id)||c.regionReleases.at(id)<=c.contentRelease;}
}
