#include "aetherwild/runtime.hpp"
namespace aw {
Json GameRuntime::terrainPatchAt(double x,double z)const {
 const auto cell=std::pair{static_cast<int>(std::floor(x/512)),static_cast<int>(std::floor(z/512))};
 if(!terrainCache_.contains(cell)){
  if(terrainCache_.size()>=64)terrainCache_.clear();
  const double originX=cell.first*512.0-256,originZ=cell.second*512.0-256;
  Json heights=Json::array();
  for(int row=0;row<65;row++)for(int col=0;col<65;col++)heights.push(std::round(worldLandHeight({originX+col*16,0,originZ+row*16},catalog_)*1000)/1000);
  terrainCache_[cell]=Json{{"side",65},{"step",16},{"x",originX},{"z",originZ},{"heights",heights},{"version",atlasSurface().num("version")}};
 }
 return terrainCache_.at(cell);
}
Json GameRuntime::terrain(const Json& body)const {
 const double x=body.at("x").number(),z=body.at("z").number();
 require(std::isfinite(x)&&std::isfinite(z)&&std::abs(x)<=catalog_.worldLimit&&std::abs(z)<=catalog_.worldLimit,"地形位置超出世界範圍");
 // Public, bounded, read-only authoring data. No session or world mutation.
 return Json{{"ok",true},{"terrain",terrainPatchAt(x,z)},{"worldLimit",catalog_.worldLimit}};
}
}
