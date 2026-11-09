#include "aetherwild/runtime.hpp"
namespace aw {
void GameRuntime::tickNpcs(double dt){
 if(dt<=0)return;
 if(!contentState_.contains("npcHomes"))contentState_["npcHomes"]=Json::object();
 if(!contentState_.contains("npcPoses"))contentState_["npcPoses"]=Json::object();
 for(auto&[id,n]:catalog_.npcs){
  bool near=false;for(const auto&[pid,p]:world_.players)if(p.scene=="world"&&distance(p.position,n.position)<180){near=true;break;}
  if(!near)continue;
  if(!contentState_["npcHomes"].contains(id))contentState_["npcHomes"][id]=vec(n.position);
  auto profile=contentEntry("profiles",id);if(!profile)continue;
  auto& life=contentState_["life"][id];if(!std::holds_alternative<Json::Object>(life.value))life=Json::object();
  auto home=unvec(contentState_.at("npcHomes").at(id));auto goal=home;
  double hour=std::fmod(world_.now/60+9,24);
  std::string mode=hour<6||hour>=22?"rest":hour>=18?"social":"work";
  life["speed"]=0;
  // A short server lease pauses locomotion for everyone; stale/disconnected clients expire.
  auto talker=world_.players.find(life.str("talker"));
  if(life.num("talkUntil")>world_.now&&talker!=world_.players.end()&&talker->second.scene=="world"&&talker->second.vitals.alive()&&distance(talker->second.position,n.position)<=8&&clearSight(talker->second.position,n.position,buildings_)){
   life["mode"]="talk";life["yaw"]=std::atan2(talker->second.position.x-n.position.x,talker->second.position.z-n.position.z);continue;
  }
  life["talkUntil"]=0;life["talker"]="";
  bool retreat=life.str("mode")=="retreat"&&life.num("until")>world_.now;
  if(retreat){mode="retreat";goal.x+=5;}
  else if(n.region=="windchime"){
   // Short local circuits keep services reachable. Each resident has an independent phase.
   unsigned seed=0;for(unsigned char c:id)seed=seed*31+c;
   int node=static_cast<int>(life.num("route"))%4;
   double radius=n.role=="resident"?3.2:1.5;
   const double xs[]={0,1,0,-1},zs[]={0,0,1,0};
   goal.x+=xs[node]*radius;goal.z+=zs[node]*radius;
   if(mode=="rest")goal=home;
   else if(distance(n.position,goal)<.12){
    if(life.num("pauseUntil")<=0)life["pauseUntil"]=world_.now+2+seed%4;
    if(world_.now>=life.num("pauseUntil")){life["route"]=(node+1)%4;life["pauseUntil"]=0;}
   }
  }else if(mode=="social"){goal.x+=std::sin(world_.now/15+id.size())*4;goal.z+=std::cos(world_.now/15+id.size())*4;}
  double d=std::hypot(goal.x-n.position.x,goal.z-n.position.z);
  if(d>.12&&(mode=="rest"||retreat||life.num("pauseUntil")<=world_.now)){
   auto step=n.position;double speed=retreat?1.5:.85,amount=std::min(speed*dt,d);
   step.x+=(goal.x-step.x)/d*amount;step.z+=(goal.z-step.z)/d*amount;step.y=groundBelow(step,catalog_);
   if(std::abs(step.y-n.position.y)<.35&&!blocked(step,buildings_)&&clearSight(n.position,step,buildings_)&&contentAllowed(step)){
    life["yaw"]=std::atan2(step.x-n.position.x,step.z-n.position.z);life["speed"]=amount/dt;n.position=step;
   }else{life["route"]=(static_cast<int>(life.num("route"))+1)%4;life["pauseUntil"]=world_.now+2;}
  }
  life["mode"]=mode;contentState_["npcPoses"][id]=vec(n.position);
 }
}
}
namespace aw {
void GameRuntime::positionNpcs(){
 std::map<Id,int> residents;for(auto&[id,n]:catalog_.npcs){
  const auto& r=catalog_.regions.at(n.region);auto p=cityArrival(r);
  if(n.role=="guild")p.z-=5;else if(n.role=="merchant")p.x+=5;else if(n.role=="smith")p.x-=5;else if(n.role=="healer")p.z+=5;else if(n.role=="cartographer"){p.x+=5;p.z+=5;}
  else{int i=residents[n.region]++;static const double xs[]={-18,-12,-6,6,12,18};p.x+=xs[i%6];p.z+=(i/6%2?1:-1)*(14+8*(i/12));}
  if(r.id=="verdant"&&n.role!="resident"){
   p.x=r.center.x+(n.role=="guild"?-8:n.role=="merchant"?-4:n.role=="healer"?4:n.role=="cartographer"?8:0);
   p.z=r.center.z+18;
  }
  if(r.id=="verdant"&&n.role=="resident"){
   int i=residents[n.region]-1,arm=i%4;double u=((i/4)%3-1)*4.8,v=54+((i/12)%3-1)*4.8;
   p.x=r.center.x+(arm==0?u:arm==1?v:arm==2?-u:-v);
   p.z=r.center.z+(arm==0?v:arm==1?-u:arm==2?-v:u);
  }
  p.y=groundBelow(p,catalog_);n.position=p;
  if(contentState_.contains("npcPoses")&&contentState_.at("npcPoses").contains(id)){auto saved=unvec(contentState_.at("npcPoses").at(id));if(r.id!="verdant"&&distance(saved,p)<=100&&!blocked(saved,buildings_))n.position=saved;}
 }
}
}
