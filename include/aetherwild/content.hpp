#pragma once
#include "json.hpp"
namespace aw {
const Json& contentBundle();
const Json& atlasSurface();
bool atlasWater(Vec3);
double atlasHeight(Vec3);
double harborGround(Vec3,const Catalog&);
const Json* contentEntry(const std::string& section,const Id& id);
void installContent(Catalog&);
void expandContentWorld(Catalog&);
bool contentRegionOpen(const Catalog&,const Id&);
bool contentPositionAllowed(const Catalog&,Vec3);
double worldLandHeight(Vec3,const Catalog&);
std::string weaponName(WeaponKind);
bool magicWeapon(WeaponKind);
bool rangedWeapon(WeaponKind);
Id startingWeapon(const Catalog&,const Id&);
struct NpcSystem {
 static double clock();
 static Json initial();
 static int score(const Json&);
 static std::string tier(int);
 static void event(Json&,const Json& profile,const std::string& kind,const Id& receipt,double now,int amount=0);
 static Json view(const Json&,const Json& profile,double now);
};
}
