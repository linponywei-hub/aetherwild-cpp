#pragma once
#include "json.hpp"
#include "aetherwild/npc/system.hpp"
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

}
