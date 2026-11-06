#include "aetherwild/services.hpp"
namespace aw {
    namespace {
        double power(const Player& p,const Catalog& c) {
            return c.classes.at(p.profession).base.attack+(p.level-1)*3+InventorySystem::bonuses(p,c).attack;
        }
        void checkTarget(World& w,const Catalog& c,Player& p,Monster& m,double range) {
            p.attackIntent();
            require(m.vitals.alive(),"目標已死亡");
            require(p.scene==m.scene,"不在同一場景");
            require(!MovementSystem::safe(p.position,p.scene,c)&&!MovementSystem::safe(m.position,m.scene,c),"安全區禁止戰鬥");
            require(distance(p.position,m.position)<=range,"目標超出範圍");
            if(m.definition=="dungeon_boss") {
                auto&r=w.runs.at(m.scene);
                auto&d=c.dungeons.at(r.definition);
                int need=d.objective=="hunt"?0:d.objective=="seals"||d.objective=="braziers"?3:1;
                require(r.guardsRemaining==0&&r.objectives>=need,"必須先清除守衛及完成副本目標");
            }
        }
        void hit(World& w,const Catalog& c,Player& p,Monster& m,double damage) {
            double actual=m.receiveDamage(damage);
            m.contribution[p.id]+=actual;
            w.events.push_back( {
                "Damage",p.id,m.id,actual, {}
            });
            if(!m.vitals.alive())RewardSystem::onDeath(w,m,c);
        }
    }
    Monster CombatSystem::spawn(World& w,const Id& id,Vec3 pos,int level,const Id& region,bool boss,const Id& definition) {
        require(!w.monsters.contains(id)&&level>=1&&level<=100&&finite(pos),"怪物出生參數錯誤");
        Monster m;
        m.id=id;
        m.definition=definition;
        m.position=pos;
        m.region=region;
        m.level=level;
        m.boss=boss;
        double hp=(70+level*18+level*level*.20)*(boss?3.2:1);
        m.attack=std::round((8+level*3.8+level*level*.035)*(boss?1.55:1));
        m.defense=std::floor(level*1.2+level*level*.009)+(boss?8:0);
        if(level<=3&&!boss) {
            hp=40+level*15;
            m.attack=3+level*2;
            m.defense=0;
        }
        m.vitals= {hp,hp,0,0,0,0};
        w.monsters.emplace(id,m);
        return m;
    }
    AttackResult CombatSystem::attack(World& w,const Catalog& c,const Id& player,const Id& monster,const Id& arrow) {
        auto&p=w.players.at(player);
        auto&m=w.monsters.at(monster);
        auto&job=c.classes.at(p.profession);
        checkTarget(w,c,p,m,job.range);
        require(w.now>=p.attackReady,"攻擊冷卻尚未結束");
        double damage=power(p,c);
        double cooldown=.58;
        if(job.weapon==WeaponKind::Bow) {
            require(c.arrowBonus.contains(arrow),"未知箭矢");
            InventorySystem::pay(p.materials, {
                {arrow,1}
            });
            damage*=(1+c.arrowBonus.at(arrow))*(w.now<p.arrowHasteUntil?1.35:1);
            cooldown=w.now<p.arrowHasteUntil?.5:.8;
        }
        if(job.weapon==WeaponKind::Staff) {
            require(p.vitals.mp>=2,"魔力不足");
            p.vitals.mp-=2;
            damage*=.32;
            cooldown=.55;
        }
        damage=std::max(1.0,std::round(damage-m.defense));
        p.attackReady=w.now+cooldown;
        hit(w,c,p,m,damage);
        return {damage,!m.vitals.alive()};
    }
    double CombatSystem::monsterAttack(World& w,const Catalog& c,const Id& monster,const Id& player) {
        auto&m=w.monsters.at(monster);
        auto&p=w.players.at(player);
        require(m.vitals.alive()&&p.vitals.alive()&&p.scene==m.scene,"攻擊者、目標或場景無效");
        require(!MovementSystem::safe(p.position,p.scene,c),"安全區不能被攻擊");
        require(distance(m.position,p.position)<=5,"怪物超出攻擊距離");
        require(w.now>=m.nextAttack&&w.now>=m.frozenUntil,"怪物冷卻或冰凍中");
        double n=std::max(1.0,m.attack-InventorySystem::bonuses(p,c).defense);
        if(w.now<p.shieldUntil) {
            double absorb=std::min(n,p.shield);
            p.shield-=absorb;
            n-=absorb;
        }
        double actual=p.receiveDamage(n);
        m.nextAttack=w.now+1.5;
        p.regenReady=w.now+3;
        w.events.push_back( {
            "PlayerDamaged",monster,player,actual, {}
        });
        return actual;
    }
    void CombatSystem::cast(World& w,const Catalog& c,const Id& player,const Id& skill,const Id& target) {
        auto&p=w.players.at(player);
        p.attackIntent();
        require(p.learnedSkills.contains(skill)&&c.skills.contains(skill),"尚未學會技能");
        auto&s=c.skills.at(skill);
        require(s.effect!="dash","位移技能等待碰撞模組，暫不執行");
        require(w.now>=p.skillReady[skill],"技能冷卻中");
        require(p.vitals.mp>=s.mana,"魔力不足");
        Monster* main=nullptr;
        if(!target.empty()) {
            main=&w.monsters.at(target);
            checkTarget(w,c,p,*main,s.range>0?s.range:24);
        }
        require(main||s.heal>0||s.shield>0,"此技能需要目標");
        p.vitals.mp-=s.mana;
        p.skillReady[skill]=w.now+s.cooldown;
        p.vitals.restore(s.heal,0);
        if(s.shield>0) {
            p.shield=std::max(p.shield,s.shield);
            p.shieldUntil=w.now+(s.duration>0?s.duration:8);
        }
        if(main) {
            Vec3 center=main->position;
            for(auto&[id,m]:w.monsters) {
                if(m.scene!=p.scene||!m.vitals.alive()||MovementSystem::safe(m.position,m.scene,c))continue;
                if(id!=target&&(s.radius<=0||distance(center,m.position)>s.radius))continue;
                if(s.effect=="freeze")m.frozenUntil=std::max(m.frozenUntil,w.now+s.duration);
                if(s.damage>0)hit(w,c,p,m,std::max(1.0,s.damage-m.defense));
            }
        }
        w.events.push_back( {
            "SkillCast",player,target,s.damage,skill
        });
    }
    void CombatSystem::weaponSkill(World& w,const Catalog& c,const Id& id,const Id& target) {
        auto& p=w.players.at(id);
        p.attackIntent();
        auto&job=c.classes.at(p.profession);
        double mana=job.weapon==WeaponKind::Staff?28:18;
        require(w.now>=p.skillReady["weapon"]&&p.vitals.mp>=mana,"武器技冷卻中或魔力不足");
        if(job.weapon==WeaponKind::Bow) {
            p.arrowHasteUntil=w.now+8;
            p.vitals.mp-=mana;
            p.skillReady["weapon"]=w.now+14;
            return;
        }
        require(w.monsters.contains(target),"請瞄準怪物");
        auto& main=w.monsters.at(target);
        checkTarget(w,c,p,main,job.range);
        p.vitals.mp-=mana;
        p.skillReady["weapon"]=w.now+8;
        auto center=job.weapon==WeaponKind::Sword?p.position:main.position;
        for(auto&[mid,m]:w.monsters)if(m.scene==p.scene&&m.vitals.alive()&&!MovementSystem::safe(m.position,m.scene,c)&&distance(m.position,center)<=7) {
            if(m.definition=="dungeon_boss") {
                auto&r=w.runs.at(m.scene);
                auto&d=c.dungeons.at(r.definition);
                int need=d.objective=="hunt"?0:d.objective=="seals"||d.objective=="braziers"?3:1;
                if(r.guardsRemaining>0||r.objectives<need)continue;
            }
            hit(w,c,p,m,std::max(1.0,std::round(power(p,c)*2.3-m.defense)));
        }
        w.events.push_back( {
            "WeaponSkill",id,target,0, {}
        });
    }
    void CombatSystem::respawn(Player& p,const Catalog& c) {
        require(!p.vitals.alive(),"角色尚未死亡");
        Quantity loss=ProgressionSystem::levelXp(p.level)/10;
        p.xp=std::max<Quantity>(0,p.xp-loss);
        p.scene="world";
        p.position=c.regions.at(p.lastCity).center;
        p.attackReady=0;
        p.shield=0;
        p.shieldUntil=0;
        ProgressionSystem::refresh(p,c);
        p.vitals.hp=p.vitals.maxHp;
        p.vitals.mp=p.vitals.maxMp;
    }
}
