#include "aetherwild/talents.hpp"
#include "aetherwild/runtime.hpp"
#include <fstream>
#include <random>
#include <cstdlib>
#include <chrono>
#include "support.hpp"
namespace aw {
using namespace runtime_detail;
    GameRuntime::GameRuntime(std::filesystem::path path,bool legacyTestWorld):catalog_(makeCatalog()),buildings_(buildings(catalog_)),savePath_(std::move(path)) {
        configureContent(false);
        const char* publicMode=std::getenv("AETHERWILD_PUBLIC");
        if(publicMode&&std::string(publicMode)=="1") {
            const char* ownerCode=std::getenv("GAME_OWNER_CODE");
            const char* inviteCode=std::getenv("GAME_INVITE_CODE");
            require(ownerCode&&std::string(ownerCode).size()>=24&&inviteCode&&std::string(inviteCode).size()>=24,"雲端測試站需先設定足夠長度的擁有者驗證碼與邀請碼");
        }
        features_= {
            {
                "combat",true
            },
            {
                "gathering",true
            },
            {
                "crafting",true
            },
            {
                "dungeons",true
            },
            {
                "chat",true
            }
        };
        for(auto&[id,n]:catalog_.npcs) {
            auto&r=catalog_.regions.at(n.region);
            if(n.role=="resident")continue;
            n.position=cityArrival(r);
            if(n.role=="guild")n.position.z-=5;
            else if(n.role=="merchant")n.position.x+=5;
            else if(n.role=="smith")n.position.x-=5;
            else if(n.role=="healer")n.position.z+=5;
            else {
                n.position.x+=5;
                n.position.z+=5;
            }
        }
        lifts_["astral"]= {0,0,-100,false};
        if(!savePath_.empty()&&std::filesystem::exists(savePath_)) {
            std::ifstream in(savePath_,std::ios::binary);
            std::string text((std::istreambuf_iterator<char>(in)), {});
            restore(Json::parse(text));
        }
        else populate();
        if(!legacyTestWorld){applyWorldPatch();
            for(auto&[id,p]:world_.players)if(p.scene=="world"&&blocked(p.position,buildings_)){bool found=false;const auto old=p.position;for(int rad=2;rad<=60&&!found;rad+=2)for(int n=0;n<16;n++){Vec3 v{old.x+std::cos(n*tau/16)*rad,old.y+2,old.z+std::sin(n*tau/16)*rad};v.y=groundBelow(v,catalog_);if(!blocked(v,buildings_)&&contentAllowed(v)){p.position=v;found=true;break;}}if(!found)p.position=cityArrival(catalog_.regions.at(p.lastCity));motion_[id].velocityY=0;}
        }
    }
    void GameRuntime::tick(double dt) {
        require(std::isfinite(dt)&&dt>=0&&dt<=.25,"工作站tick時間錯誤");
        world_.now+=dt;
        for(auto&[id,p]:world_.players) {
            auto&m=motion_[id];
            if(world_.now-m.lastSeen<8)physics(p,m,dt);
            if(p.vitals.alive()&&MovementSystem::safe(p.position,p.scene,catalog_))p.vitals.restore(dt*2,dt*3);
        }
        for(auto&[id,l]:lifts_)if(l.moving&&world_.now-l.start>=6) {
            l.moving=false;
            l.from=l.to;
        }
        for(auto&[id,n]:world_.resources)if(n.reservedBy&&world_.now>=n.completeAt) {
            if(world_.players.contains(*n.reservedBy))try {
                GatheringSystem::finish(world_,world_.players.at(*n.reservedBy),id,catalog_);
            }
            catch(const RuleError&) {
                n.reservedBy.reset();
            }
            else n.reservedBy.reset();
        }
        contentTick(dt);
        ai(dt);
        settleRewards();
        if(world_.events.size()>2000)world_.events.erase(world_.events.begin(),world_.events.begin()+1000);
        if(world_.now-savedAt_>=10)save();
    }
}
