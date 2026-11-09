#include "aetherwild/npc/system.hpp"
#include <chrono>
namespace aw {
double NpcSystem::clock(){return std::chrono::duration<double>(std::chrono::system_clock::now().time_since_epoch()).count();}
Json NpcSystem::initial(){return Json{{"score",0},{"lockUntil",0},{"repair",false},{"lastAttack",-1e12},{"lastChat",-1e12},{"day",-1},{"daily",0},{"chats",0},{"shopping",0},{"topics",Json::object()},{"receipts",Json::object()},{"memory",Json::array()},{"questAccepted",false},{"questDone",false},{"giftClaimed",false}};}
int NpcSystem::score(const Json& a){return static_cast<int>(a.num("score"));}
std::string NpcSystem::tier(int n){return n<=-40?"hostile":n<0?"wary":n<20?"neutral":n<40?"familiar":n<70?"trusted":"confidant";}
void NpcSystem::event(Json& a,const Json& profile,const std::string& kind,const Id& receipt,double now,int amount){
 require(std::isfinite(now)&&now>=0&&!receipt.empty(),"關係事件錯誤");if(a.at("receipts").contains(receipt))return;
 auto day=static_cast<std::int64_t>(std::floor((now+8*3600)/86400));if(a.num("day")!=day){a["day"]=day;a["daily"]=0;a["chats"]=0;a["shopping"]=0;a["topics"]=Json::object();}
 int delta=0;
 if(kind=="attack"){
  if(now-a.num("lastAttack")>=5){delta=-25;a["lastAttack"]=now;}
  a["repair"]=true;a["lockUntil"]=std::max(a.num("lockUntil"),now+86400);
 }else if(kind=="repair"){
  require(a.flag("repair")&&now>=a.num("lockUntil"),"補救期限未到");a["repair"]=false;delta=5;
 }else if(!a.flag("repair")){
  if(kind=="chat"&&now-a.num("lastChat")>=21600&&a.num("chats")<2){delta=1;a["lastChat"]=now;a["chats"]=a.num("chats")+1;}
  else if(kind=="purchase"){delta=std::min({2,std::max(0,amount/100),4-static_cast<int>(a.num("shopping"))});a["shopping"]=a.num("shopping")+delta;}
  else if(kind=="quest")delta=6;
  else if(kind=="private_quest")delta=12;
 }
 if(delta>0){delta=std::min(delta,20-static_cast<int>(a.num("daily")));a["daily"]=a.num("daily")+delta;}
 a["score"]=std::clamp(score(a)+delta,-100,100);a["receipts"][receipt]=true;
 auto& mem=std::get<Json::Array>(a["memory"].value);mem.push_back(Json{{"kind",kind},{"at",now},{"delta",delta},{"npc",profile.str("id")}});if(mem.size()>32)mem.erase(mem.begin());
}
Json NpcSystem::view(const Json& a,const Json& profile,double now){
 bool safe=!a.flag("repair")&&now>=a.num("lockUntil");auto key=safe?tier(score(a)):score(a)<=-40?"hostile":"wary";
 Json topics=Json::array();int i=0;for(auto&t:profile.at("conversation_topics").arrayItems())if(i++<2||score(a)>=40)topics.push(t);
 return Json{{"score",score(a)},{"tier",tier(score(a))},{"restricted",!safe},{"dialogue",safe&&a.contains("lastLine")?a.str("lastLine"):profile.at("dialogue").str(key)},{"topics",topics},{"canAccept",safe&&score(a)>=40&&!a.flag("questAccepted")},{"questAccepted",a.flag("questAccepted")},{"questDone",a.flag("questDone")},{"canGift",safe&&score(a)>=70&&a.flag("questDone")&&!a.flag("giftClaimed")},{"giftClaimed",a.flag("giftClaimed")},{"discount",safe&&score(a)>=20?profile.at("high_affinity").num("discount_percent_cap"):0}};
}
}
