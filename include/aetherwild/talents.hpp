#pragma once
#include "aetherwild/services.hpp"
namespace aw {
struct TalentDefinition { Id id; std::string name, branch; Id parent; std::string description; Stats bonus; int level; };
inline const std::vector<TalentDefinition>& talentDefinitions() { static const std::vector<TalentDefinition> definitions{
{"edge","利刃研習","offense","","攻擊力 +4。",{0,0,4,0,0,0},2},
{"execution","斬殺直覺","offense","edge","目標生命低於 30% 時，傷害提高 20%。",{0,0,0,0,0,0},5},
{"focus","戰鬥專注","offense","execution","職業技能冷卻縮短 2 秒。",{0,0,0,0,0,0},8},
{"vigor","生命淬鍊","survival","","最大生命 +30。",{30,0,0,0,0,0},2},
{"bulwark","堅韌防線","survival","vigor","承受傷害減少 4 點。",{0,0,0,4,0,0},5},
{"renewal","回復精通","survival","bulwark","藥水回復量增加 25%。",{0,0,0,0,0,0},8},
{"forage","自然饋贈","craft","","採集時有 25% 機率額外取得 1 份材料。",{0,0,0,0,0,0},2},
{"insight","匠心領悟","craft","forage","製作獲得的經驗增加 50%。",{0,0,0,0,0,0},5},
{"alchemy","藥劑增效","craft","insight","製作治療藥水時，額外獲得 1 瓶。",{0,0,0,0,0,0},8},
{"keen","精準磨刃","offense","edge","攻擊 +6。",{0,0,6,0,0,0},5},
{"arcane","星紋共鳴","offense","edge","魔力 +20、攻擊 +3。",{0,20,3,0,0,0},5},
{"pursuit","疾風追獵","offense","keen","攻擊 +5、移速 +0.15。",{0,0,5,0,0,0.15},8},
{"resolve","破陣決心","offense","execution","攻擊 +9。",{0,0,9,0,0,0},8},
{"flow","魔流循環","offense","arcane","魔力 +35。",{0,35,0,0,0,0},8},
{"duelist","決鬥研習","offense","focus","攻擊 +12、防禦 +2。",{0,0,12,2,0,0},11},
{"spellblade","秘法刃心","offense","flow","攻擊 +10、魔力 +25。",{0,25,10,0,0,0},11},
{"tempest","暴風行者","offense","pursuit","攻擊 +8、移速 +0.25。",{0,0,8,0,0,0.25},11},
{"apex","終焉戰意","offense","duelist","攻擊 +20、生命 +40。",{40,0,20,0,0,0},14},
{"bark","樹皮護佑","survival","vigor","防禦 +3。",{0,0,0,3,0,0},5},
{"reservoir","靈泉儲備","survival","vigor","魔力 +25。",{0,25,0,0,0,0},5},
{"endure","不屈體魄","survival","bark","生命 +50。",{50,0,0,0,0,0},8},
{"ward","符印守望","survival","bulwark","防禦 +5、魔力 +15。",{0,15,0,5,0,0},8},
{"stride","輕裝步伐","survival","reservoir","移速 +0.2。",{0,0,0,0,0,0.2},8},
{"bastion","磐石之軀","survival","endure","生命 +70、防禦 +3。",{70,0,0,3,0,0},11},
{"sanctuary","靈光庇所","survival","renewal","生命 +40、魔力 +40。",{40,40,0,0,0,0},11},
{"sentinel","永夜守衛","survival","ward","防禦 +8。",{0,0,0,8,0,0},11},
{"immortal","生息不滅","survival","bastion","生命 +120、防禦 +5。",{120,0,0,5,0,0},14},
{"trail","荒野尋徑","craft","forage","移速 +0.15。",{0,0,0,0,0,0.15},5},
{"herbal","藥草識讀","craft","forage","生命 +20、魔力 +15。",{20,15,0,0,0,0},5},
{"porter","遠行體能","craft","trail","生命 +35、移速 +0.15。",{35,0,0,0,0,0.15},8},
{"forge","淬火匠魂","craft","insight","攻擊 +6、防禦 +2。",{0,0,6,2,0,0},8},
{"distill","蒸餾奧義","craft","herbal","魔力 +35。",{0,35,0,0,0,0},8},
{"expedition","遠征行家","craft","porter","移速 +0.25、生命 +35。",{35,0,0,0,0,0.25},11},
{"masterwork","精工傳承","craft","forge","攻擊 +10、防禦 +4。",{0,0,10,4,0,0},11},
{"elixir","靈藥賢者","craft","alchemy","生命 +45、魔力 +45。",{45,45,0,0,0,0},11},
{"worldwalker","界域行者","craft","expedition","生命 +60、魔力 +30、移速 +0.3。",{60,30,0,0,0,0.3},14},
}; return definitions; }
inline int talentPoints(const Player& p) { return std::max(0,p.level-1-static_cast<int>(p.talents.size())); }
inline void validateTalents(const Player& p) { require(p.talents.size()<=static_cast<size_t>(std::max(0,p.level-1)),"天賦點數超額"); for(const auto& id:p.talents) { auto& ds=talentDefinitions();auto it=std::find_if(ds.begin(),ds.end(),[&](const auto& d){return d.id==id;});require(it!=ds.end(),"未知天賦");require(p.level>=it->level&&(it->parent.empty()||p.talents.contains(it->parent)),"天賦前置未達成"); }}
inline void learnTalent(Player& p,const Id& id,const Catalog& c) { require(p.vitals.alive(),"死亡時無法學習天賦");auto& ds=talentDefinitions();auto it=std::find_if(ds.begin(),ds.end(),[&](const auto& d){return d.id==id;});require(it!=ds.end(),"未知天賦");require(!p.talents.contains(id),"天賦已學習");require(talentPoints(p)>0,"天賦點數不足");require(p.level>=it->level&&(it->parent.empty()||p.talents.contains(it->parent)),"天賦前置未達成");p.talents.insert(id);ProgressionSystem::refresh(p,c); }
}
