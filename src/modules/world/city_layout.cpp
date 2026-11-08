#include "aetherwild/geometry.hpp"
#include "aetherwild/content.hpp"
namespace aw {
double urbanRadius(const RegionDefinition& r){
 if(r.wilderness)return 0;
 double desired=r.id=="aw_crownreach"?1150:r.id=="windchime"?160:r.id=="verdant"||r.id=="astral"?90:r.id=="frosthold"?180:340;
 return std::min(desired,r.radius*.82);
}
std::vector<Building> cityBuildings(const Catalog& c){
 std::vector<Building> out;
 for(const auto&[id,r]:c.regions){
  if(r.wilderness)continue;
  double u=urbanRadius(r);int serial=0;
  auto add=[&](std::string kind,double x,double z,double w,double d,double h,double y=-1e8){
   Vec3 p{r.center.x+x,r.center.y+cityElevation(id),r.center.z+z};if(y>-1e7)p.y=y;else if(id!="verdant"&&id!="astral")p.y=worldLandHeight(p,c);
   out.push_back({id+"_urban_"+std::to_string(serial++),id,p,w,d,h,id+"|"+kind});
  };
  const bool harbor=id=="tidehaven"||id=="aw_westport"||id=="aw_southmill";
  if(harbor){
   for(const auto& port:atlasSurface().at("harbors").arrayItems())if(port.str("region")==id)for(const auto& b:port.at("platforms").arrayItems()){
    double x=b.num("x"),z=b.num("z"),w=b.num("width"),d=b.num("depth"),y=b.num("height");
    if(b.str("kind")=="ship")for(double side:{-1.,1.})for(int row=-1;row<=1;row++)add(row==0?"shop":"cabin",x+side*std::min(120.,w*.22),z+row*std::min(200.,d*.22),24,20,12,y);
    else if(b.str("kind")=="quay"&&w>60){add("warehouse",x-w*.3,z+d*.23,18,14,12,y);add("warehouse",x+w*.3,z+d*.23,18,14,12,y);}
    for(double side:{-1.,1.})if(w>35&&d>40)add("lamp",x+side*(w/2-4),z,.45,.45,5,y);
   }
   for(double side:{-1.,1.})for(double z:{-32.,-10.,10.,32.})add("lamp",side*12,z,.45,.45,5,harborGround({r.center.x+side*12,10000,r.center.z+z},c));
   continue;
  }
  if(id=="aw_crownreach"){
   add("palace",0,-u*.38,170,105,78);add("tower",-115,-u*.38,34,34,105);add("tower",115,-u*.38,34,34,105);
   const double edge=u*.65,gap=45,length=edge-gap;
   for(double side:{-1.,1.})for(double offset:{-1.,1.}){
    add("wall",offset*(edge+gap)/2,side*edge,length,8,18);
    add("wall",side*edge,offset*(edge+gap)/2,8,length,18);
   }
   for(double x:{-1.,1.})for(double z:{-1.,1.})add("tower",x*edge,z*edge,24,24,38);
  }
  int rings=u<100?1:id=="windchime"||id=="frosthold"?2:3;
  for(int ring=0;ring<rings;ring++)for(int i=0;i<12;i++){
   double a=(i+.5)*6.283185307/12,rad=u*(u<100?.60:.24+ring*.15),x=std::cos(a)*rad,z=std::sin(a)*rad;
   if(id=="aw_crownreach"&&z< -u*.25&&std::abs(x)<210)continue;
   if(id=="cinder"&&std::hypot(x+u*.44,z+u*.32)<42)continue;
   if(id=="sundune"&&std::abs(x-u*.4)<34&&std::abs(z+u*.35)<31)continue;
   if(id=="aw_northwatch"&&std::abs(x)<34&&std::abs(z+u*.48)<30)continue;
   if(id=="aw_eastglass"&&std::abs(x)<26&&std::abs(z+u*.47)<27)continue;
   double size=u<100?9:id=="aw_crownreach"?28:14;
   std::string kind=ring==0?(i%3==0?"guild":i%3==1?"shop":"house"):"house";
   add(kind,x,z,size+(i%2)*2,size, id=="frosthold"?9:id=="cinder"?18:13+(i%3)*3);
  }
  if(id=="cinder")add("foundry",-u*.44,-u*.32,34,30,34);
  if(id=="sundune")add("temple",u*.4,-u*.35,38,32,28);
  if(id=="aw_northwatch")add("keep",0,-u*.48,45,24,32);
  if(id=="aw_eastglass")add("spire",0,-u*.47,24,24,45);
  for(double x:{-12.,12.})for(double z:{-10.,10.})add("lamp",x,z,.45,.45,5);
  // Lamps stand beside the central roads, never in the service/NPC plaza.
  for(int i=0;i<(u>600?12:u>150?5:2);i++)for(double side:{-1.,1.}){
   double along=32+i*std::min(45.,u*.075);if(along>u*.6)continue;
   add("lamp",side*12,along,.45,.45,5);add("lamp",side*12,-along,.45,.45,5);
  }
 }
 return out;
}
}
