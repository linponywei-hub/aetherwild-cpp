#include "aetherwild/runtime.hpp"
namespace aw {
void GameRuntime::tickNpcs(){
 for(auto&[id,n]:catalog_.npcs){
  bool near=false;for(auto&[pid,p]:world_.players)if(p.scene=="world"&&distance(p.position,n.position)<180){near=true;break;}
  if(!near)continue;
  if(!contentState_.contains("npcHomes"))contentState_["npcHomes"]=Json::object();
  if(!contentState_["npcHomes"].contains(id))contentState_["npcHomes"][id]=vec(n.position);
  auto profile=contentEntry("profiles",id);if(!profile)continue;
  if(contentState_.at("life").contains(id)&&contentState_.at("life").at(id).num("until")>world_.now){
   if(contentState_.at("life").at(id).str("mode")=="retreat"){auto next=n.position;next.x+=1;if(!blocked(next,buildings_)&&contentAllowed(next))n.position=next;}continue;
  }
  double hour=std::fmod(world_.now/60+9,24);std::string mode=hour<6||hour>=22?"rest":hour>=18?(profile->at("personality").num("warmth")>.55?"social":"personal_work"):"work";
  {
   auto home=unvec(contentState_.at("npcHomes").at(id));auto goal=home;
   if(mode=="social"||mode=="personal_work"){goal.x+=std::sin(world_.now/15+id.size())*4;goal.z+=std::cos(world_.now/15+id.size())*4;}
   auto d=distance(n.position,goal);if(d>.1){auto step=n.position;step.x+=(goal.x-step.x)/d*std::min(1.0,d);step.z+=(goal.z-step.z)/d*std::min(1.0,d);if(!blocked(step,buildings_)&&contentAllowed(step))n.position=step;}
  }
  if(!contentState_.contains("npcPoses"))contentState_["npcPoses"]=Json::object();
  contentState_["npcPoses"][id]=vec(n.position);
  contentState_["life"][id]=Json{{"mode",mode},{"until",world_.now+1}};
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
