#include "../world/support.hpp"
#include "aetherwild/runtime.hpp"
namespace aw {
using content_detail::playerObject;
bool GameRuntime::npcCommand(Player& p,const Json& j){
 auto type=j.str("type"),target=j.str("target"),request=j.str("request");
 if(type.rfind("npc_",0)!=0)return false;
 require(p.vitals.alive()&&p.scene=="world"&&catalog_.npcs.contains(target),"NPC 不存在或無法互動");const auto& n=catalog_.npcs.at(target);checkContentRegion(n.region);
 require(distance(p.position,n.position)<=8,"請走近NPC");require(clearSight(p.position,n.position,buildings_),"NPC被建物遮擋");
 // Conversation release is owned by the speaking player and never changes affinity.
 if(type=="npc_leave"){
  auto& life=contentState_["life"][target];
  if(life.str("talker")==p.id){life["talkUntil"]=0;life["mode"]="work";}return true;
 }
 if(type=="npc_talk"){
  auto& life=contentState_["life"][target];
  require(life.str("mode")!="retreat"||life.num("until")<=world_.now,"對方正在退避，請稍候");
  require(life.num("talkUntil")<=world_.now||life.str("talker")==p.id,"對方正在與其他旅人交談");
  life["talker"]=p.id;life["talkUntil"]=world_.now+20;life["mode"]="talk";life["speed"]=0;
  life["yaw"]=std::atan2(p.position.x-n.position.x,p.position.z-n.position.z);return true;
 }
 if(type=="npc_chat"&&contentState_.at("life").contains(target)){
  const auto& life=contentState_.at("life").at(target);
  require(life.num("talkUntil")<=world_.now||life.str("talker")==p.id,"對方正在與其他旅人交談");
 }
 auto profile=contentEntry("profiles",target);require(profile,"此NPC沒有個人設定");auto& per=playerObject(contentState_["relations"],p.id);if(!per.contains(target))per[target]=NpcSystem::initial();auto& a=per[target];double now=NpcSystem::clock();
 auto state=NpcSystem::view(a,*profile,now);
 if(type=="npc_chat"){
  auto topic=j.str("topic");bool known=false;int topicIndex=0;for(auto&t:profile->at("conversation_topics").arrayItems()){if(t.string()==topic){known=true;break;}++topicIndex;}require(known,"請選擇NPC知道的話題");require(topicIndex<2||NpcSystem::score(a)>=40,"此話題需要更多信任");
  auto day=static_cast<std::int64_t>(std::floor((now+8*3600)/86400));auto key=std::to_string(day)+":"+topic;
  if(!a.at("topics").contains(key)){NpcSystem::event(a,*profile,"chat",request,now);a["topics"][key]=true;}
  auto reply=NpcSystem::villageReply(target,topicIndex);
  if(!reply.empty())a["lastLine"]=profile->str("name")+"：「"+reply+"」";
  else if(profile->contains("topic_responses")&&profile->at("topic_responses").contains(topic))a["lastLine"]=profile->str("name")+"：「"+profile->at("topic_responses").str(topic)+"」";
  else a["lastLine"]=profile->str("name")+"：「"+topic+"」讓我想到"+(NpcSystem::score(a)>=40?profile->str("biography"):std::string("我們這裡還有很多值得慢慢了解的事。先把路上看到的記下來，下次再交換消息。"));
  return true;
 }
 if(type=="npc_attack"){
  require(distance(p.position,n.position)<=3,"蓄意攻擊NPC需要近距離");require(world_.now-motion_[p.id].lastAction>=.5,"操作太快");
  NpcSystem::event(a,*profile,"attack",request,now);motion_[p.id].lastAction=world_.now;
  contentState_["life"][target]=Json{{"mode","retreat"},{"until",world_.now+15},{"line",profile->at("dialogue").str("hostile")}};
  world_.events.push_back({"NpcRetreat",p.id,target,-25,"NPC退避並呼叫守衛；公共服務仍可使用"});return true;
 }
 if(type=="npc_private_accept"){require(state.flag("canAccept"),"信任或安全條件不足");a["questAccepted"]=true;return true;}
 if(type=="npc_private_help"){
  require(a.flag("questAccepted")&&!a.flag("questDone")&&!state.flag("restricted"),"沒有可完成的私人委託");
  auto book="aw_update_book_"+n.region;auto& books=playerObject(contentState_["booksRead"],p.id);require(books.flag(book),"先閱讀本區藏記架的地方書籍，再回來核對");
  InventorySystem::pay(p.materials,{{"herb",2}});a["questDone"]=true;NpcSystem::event(a,*profile,"private_quest",request,now);return true;
 }
 if(type=="npc_gift"){
  require(state.flag("canGift"),"尚未符合贈物條件或已領取");auto gift=profile->at("high_affinity").str("gift");playerObject(contentState_["collections"],p.id)[gift]=true;a["giftClaimed"]=true;return true;
 }
 if(type=="npc_repair"){
  require(a.flag("repair")&&now>=a.num("lockUntil"),"仍在安全限制期間");InventorySystem::pay(p.materials,{{"wood",2}});NpcSystem::event(a,*profile,"repair",request,now);return true;
 }
 throw RuleError("未知NPC操作");
}
}
