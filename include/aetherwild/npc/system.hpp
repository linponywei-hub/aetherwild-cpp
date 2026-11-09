#pragma once
#include "aetherwild/json.hpp"
#include <string_view>
namespace aw {
const NpcDefinition& nearbyNpc(const Player&,const Id&,std::string_view role,const Catalog&);
struct NpcSystem {
 static double clock();
 static std::string villageReply(const Id&,int topic);
 static Json initial();
 static int score(const Json&);
 static std::string tier(int);
 static void event(Json&,const Json& profile,const std::string& kind,const Id& receipt,double now,int amount=0);
 static Json view(const Json&,const Json& profile,double now);
};
}
