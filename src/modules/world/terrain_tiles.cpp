#include "aetherwild/runtime.hpp"
namespace aw {
namespace {
Json sampledTile(double x,double z,int side,const Catalog& catalog){
 Json heights=Json::array();for(int row=0;row<side;row++)for(int col=0;col<side;col++)heights.push(std::round(worldLandHeight({x+col*8,0,z+row*8},catalog)*1000)/1000);
 return Json{{"side",side},{"step",8},{"x",x},{"z",z},{"heights",heights},{"version",atlasSurface().num("version")}};
}
}
Json GameRuntime::terrainPatchAt(double x,double z)const {
 const auto cell=std::pair{static_cast<int>(std::floor(x/256)),static_cast<int>(std::floor(z/256))};
 if(!terrainCache_.contains(cell)){if(terrainCache_.size()>=64)terrainCache_.clear();terrainCache_[cell]=sampledTile(cell.first*256.0-128,cell.second*256.0-128,65,catalog_);}
 return terrainCache_.at(cell);
}
Json GameRuntime::terrain(const Json& body)const {
 const double x=body.at("x").number(),z=body.at("z").number();
 require(std::isfinite(x)&&std::isfinite(z)&&std::abs(x)<=catalog_.worldLimit&&std::abs(z)<=catalog_.worldLimit,"地形位置超出世界範圍");
 // Larger render-only tiles are requested once per cell, never attached to each
 // realtime snapshot. The bounded cache uses a disjoint key range for them.
 if(body.flag("detail")){
  const int cx=static_cast<int>(std::floor(x/256)),cz=static_cast<int>(std::floor(z/256));const auto key=std::pair{cx,cz+1000000};
  if(!terrainCache_.contains(key)){if(terrainCache_.size()>=64)terrainCache_.clear();terrainCache_[key]=sampledTile(cx*256.0-384,cz*256.0-384,129,catalog_);}
  return Json{{"ok",true},{"terrain",terrainCache_.at(key)},{"worldLimit",catalog_.worldLimit}};
 }
 return Json{{"ok",true},{"terrain",terrainPatchAt(x,z)},{"worldLimit",catalog_.worldLimit}};
}
}
