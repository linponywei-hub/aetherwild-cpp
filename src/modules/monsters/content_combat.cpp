#include "aetherwild/runtime.hpp"
namespace aw {
bool GameRuntime::monsterAbility(Monster& m,Player* target){
 if(!target){m.castResolve=0;m.castSkill.clear();return false;}
 const Json* definition=nullptr;
 if(catalog_.bosses.contains(m.definition))definition=contentEntry("monsters",m.definition);
 else if(m.definition=="dungeon_boss"&&world_.runs.contains(m.scene))definition=contentEntry("monsters","aw_bestiary_dungeon_"+world_.runs.at(m.scene).definition);
 else{unsigned h=0;for(unsigned char ch:m.id)h=h*31+ch;definition=contentEntry("monsters","aw_species_"+m.region+"_"+std::to_string(1+h%5));}
 if(!definition||(!m.boss&&m.level<4))return false;
 if(m.castResolve>0){
  if(world_.now<m.castResolve)return true;
  auto skill=contentEntry("skills",m.castSkill);m.castResolve=0;if(!skill)return false;
  auto shape=skill->str("shape");double radius=m.boss?7:4;
  for(auto&[id,p]:world_.players){
   if(!p.vitals.alive()||p.scene!=m.scene||MovementSystem::safe(p.position,p.scene,catalog_))continue;
   double dx=p.position.x-m.position.x,dz=p.position.z-m.position.z,d=std::hypot(dx,dz);bool hit=false;
   if(shape=="annulus")hit=d>=8&&d<=20;
   else if(shape=="cone"||shape=="melee_arc"){
    double ax=m.castPoint.x-m.position.x,az=m.castPoint.z-m.position.z,len=std::max(.01,std::hypot(ax,az));hit=d<=12&&(dx*ax+dz*az)/(std::max(.01,d)*len)>.5;
   }else if(shape=="capsule"){
    double ax=m.castPoint.x-m.position.x,az=m.castPoint.z-m.position.z,len=ax*ax+az*az;double t=len?std::clamp((dx*ax+dz*az)/len,0.0,1.0):0;
    hit=std::hypot(dx-t*ax,dz-t*az)<=2.5&&d<=24;
   }else if(shape=="circle_support"||shape=="ally_circle"){if(id==target->id)m.vitals.restore(m.vitals.maxHp*.01,0);continue;}
   else hit=distance(p.position,m.castPoint)<=radius;
   if(!hit||(p.scene=="world"&&!clearSight(m.position,p.position,buildings_)))continue;
   double damage=std::max(1.0,m.attack*1.15-InventorySystem::bonuses(p,catalog_).defense);
   if(world_.now<p.shieldUntil){double absorbed=std::min(damage,p.shield);p.shield-=absorbed;damage-=absorbed;m.contribution[p.id]+=absorbed*.5;}
   auto actual=p.receiveDamage(damage);p.regenReady=world_.now+3;world_.events.push_back({"PlayerDamaged",m.id,p.id,actual,skill->str("name")});
  }
  m.castSkill.clear();return true;
 }
 if(world_.now<m.abilityReady||distance(m.position,target->position)>24)return false;
 const auto& list=definition->at("skills").arrayItems();
 int phase=m.vitals.hp/m.vitals.maxHp>.7?0:m.vitals.hp/m.vitals.maxHp>.35?1:2;
 auto index=(static_cast<std::size_t>(world_.now/6)+phase)%list.size();auto id=list[index].string();auto skill=contentEntry("skills",id);if(!skill)return false;
 // Support/summon specials remain authored designs; do not fake their advanced mechanics.
 if(skill->str("shape")=="circle_support"||skill->str("shape")=="ally_circle"){m.abilityReady=world_.now+5;return false;}
 auto windup=std::max(1.2,skill->at("animation").num("windup_s",2));m.castSkill=id;m.castPoint=target->position;m.castResolve=world_.now+windup;m.abilityReady=world_.now+windup+6;
 world_.events.push_back({"MonsterCast",m.id,target->id,windup,skill->str("name")});return true;
}
}
