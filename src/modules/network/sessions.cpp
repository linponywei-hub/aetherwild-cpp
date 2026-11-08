#include "auth.hpp"
#include "aetherwild/talents.hpp"
#include "aetherwild/runtime.hpp"
#include <fstream>
#include <random>
#include <cstdlib>
#include <chrono>
#include "../runtime/support.hpp"
namespace aw {
using namespace runtime_detail;
using auth_detail::sameCode;
namespace {
        std::string tokenValue() {
            std::random_device r;
            std::ostringstream o;
            for(int i=0;i<8;i++)o<<std::hex<<std::setw(8)<<std::setfill('0')<<r();
            return o.str();
        }

}
    Id GameRuntime::token() {
        Id v;
        do {
            v=tokenValue();
        }
        while(profiles_.contains(v));
        return v;
    }
    Json GameRuntime::createSession(const Json&request) {
        const char* ownerCode=std::getenv("GAME_OWNER_CODE");
        const char* inviteCode=std::getenv("GAME_INVITE_CODE");
        bool protectedMode=(ownerCode&&*ownerCode)||(inviteCode&&*inviteCode);
        bool verifiedOwner=sameCode(request.str("ownerCode"),ownerCode);
        if(protectedMode)require(verifiedOwner||sameCode(request.str("inviteCode"),inviteCode),"邀請碼或擁有者驗證碼不正確");
        auto name=request.str("name","旅人");
        auto job=request.str("profession","warrior");
        require(!name.empty()&&name.size()<=60,"角色名稱需1至20個中文字");
        for(unsigned char ch:name)require(ch>=32,"名稱含控制字元");
        require(catalog_.classes.contains(job)&&catalog_.classes.at(job).level==1&&catalog_.classes.at(job).parent.empty(),"請選擇基礎職業");
        require(profiles_.size()<100,"此本機工作站角色數已達上限");
        auto id=world_.nextId("player");
        Player p;
        p.id=id;
        p.accountId="workstation:"+id;
        p.name=name;
        p.profession=job;
        p.jobs= {job};
        ProgressionSystem::refresh(p,catalog_);
        p.vitals.hp=p.vitals.maxHp;
        p.vitals.mp=p.vitals.maxMp;
        auto type=catalog_.classes.at(job).weapon;
        auto code=startingWeapon(catalog_,job);
        p.inventory.push_back(InventorySystem::create(world_,catalog_,code));
        InventorySystem::equip(p,p.inventory.front().id,catalog_);
        p.materials["potion"]=3;
        if(type==WeaponKind::Bow)p.materials["woodarrow"]=40;
        if(contentState_.flag("expanded"))p.position=cityArrival(catalog_.regions.at("windchime"));
        p.coins=60;
        world_.players.emplace(id,p);
        auto key=token();
        Role role=protectedMode?(verifiedOwner?Role::Owner:Role::Player):(profiles_.empty()?Role::Owner:Role::Player);
        social_.accounts[p.accountId]= {p.accountId,role};
        profiles_[key]= {
            id, {}
        };
        motion_[id].lastSeen=world_.now;
        motion_[id].lastInput=-100;
        save();
        return Json {
            {
                "ok",true
            },
            {
                "token",key
            },
            {
                "player",playerJson(p)
            },
            {
                "role",roleName(role)
            }
        };
    }
}
