#include "aetherwild/domain.hpp"
namespace aw {
    void require(bool c,const std::string& r) {
        if(!c)throw RuleError(r);
    }
    bool finite(Vec3 v) {
        return std::isfinite(v.x)&&std::isfinite(v.y)&&std::isfinite(v.z);
    }
    double distance(Vec3 a,Vec3 b) {
        return std::hypot(std::hypot(a.x-b.x,a.z-b.z),a.y-b.y);
    }
    Stats& Stats::operator+=(const Stats& s) {
        hp+=s.hp;
        mp+=s.mp;
        attack+=s.attack;
        defense+=s.defense;
        crit+=s.crit;
        speed+=s.speed;
        return *this;
    }
    double Vitals::receiveDamage(double n) {
        require(std::isfinite(n)&&n>=0,"傷害必須是非負有限數");
        double actual=std::min(hp,n);
        hp-=actual;
        return actual;
    }
    void Vitals::restore(double h,double m) {
        require(std::isfinite(h)&&std::isfinite(m)&&h>=0&&m>=0,"回復數值錯誤");
        hp=std::min(maxHp,hp+h);
        mp=std::min(maxMp,mp+m);
    }
    void Player::attackIntent() const {
        require(vitals.alive(),"死亡角色無法攻擊");
    }
    Id World::nextId(const std::string& p) {
        require(sequence<UINT64_MAX,"ID 序列已滿");
        return p+"_"+std::to_string(++sequence);
    }
    void Catalog::validate()const {
        require(!classes.empty()&&!regions.empty(),"目錄不可為空");
        for(auto& [id,i]:items) {
            require(id==i.id&&i.requiredLevel>=1,"裝備定義錯誤");
            require(i.set.empty()||sets.contains(i.set),"裝備套裝不存在");
        }
        for(auto& [id,c]:classes) {
            require(c.level>=1&&(c.parent.empty()||classes.contains(c.parent)),"職業父節點錯誤");
        }
        for(auto& [id,d]:dungeons) {
            require(regions.contains(d.region)&&d.guards>=0,"副本區域錯誤");
            require(d.skill.empty()||skills.contains(d.skill),"副本技能不存在");
        }
        for(auto& [id,b]:bosses)require(regions.contains(b.region),"首領區域不存在");
        for(auto& [id,r]:recipes) {
            require(!r.cost.empty()&&r.count>0,"配方錯誤");
            require(!r.equipment||items.contains(r.output),"配方裝備不存在");
            for(auto& [k,n]:r.cost)require(n>0&&resources.contains(k),"配方材料不存在");
        }
        for(auto& [id,n]:npcs)require(regions.contains(n.region),"NPC 區域不存在");
    }
}
