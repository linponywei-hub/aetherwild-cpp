#pragma once
#include "aetherwild/runtime.hpp"
namespace aw::content_detail {

inline Json initialContent(){return Json{{"expanded",false},{"release",1},{"relations",Json::object()},{"collections",Json::object()},{"booksRead",Json::object()},{"life",Json::object()},{"npcHomes",Json::object()},{"npcPoses",Json::object()},{"storyProgress",Json::object()}};}
inline Json& playerObject(Json& root,const Id& id){if(!root.contains(id))root[id]=Json::object();return root[id];}
inline double planeDistance(Vec3 a,Vec3 b){return std::hypot(a.x-b.x,a.z-b.z);}
}
