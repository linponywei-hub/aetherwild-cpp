#include "../network/auth.hpp"
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
    Json GameRuntime::exportBackup(const Id& key)const {
        require(profiles_.contains(key),"登入已失效");
        const auto& p=world_.players.at(profiles_.at(key).player);
        require(social_.accounts.at(p.accountId).role==Role::Owner,"只有擁有者能備份世界");
        return Json{{"format","aetherwild-world-backup"},{"version",1},{"world",state()}};
    }
    Json GameRuntime::importBackup(const Id& key,const Json& body) {
        bool owner=false;
        if(profiles_.contains(key)){const auto& p=world_.players.at(profiles_.at(key).player);owner=social_.accounts.at(p.accountId).role==Role::Owner;}
        require(owner||sameCode(body.str("ownerCode"),std::getenv("GAME_OWNER_CODE")),"只有擁有者能還原世界");
        require(body.str("confirm")=="RESTORE_WORLD","請確認還原世界備份");
        const auto& backup=body.at("backup");require(backup.str("format")=="aetherwild-world-backup"&&backup.num("version")==1,"備份格式不符");
        auto before=state();
        try {
            restore(backup.at("world"));applyWorldPatch();
            Id restoredKey;for(const auto&[token,profile]:profiles_){const auto& player=world_.players.at(profile.player);if(social_.accounts.at(player.accountId).role==Role::Owner){restoredKey=token;break;}}
            require(!restoredKey.empty(),"備份內沒有擁有者角色");
            if(!savePath_.empty()){std::filesystem::create_directories(savePath_.parent_path());auto recovery=savePath_;recovery+=".pre-restore";std::ofstream out(recovery,std::ios::binary|std::ios::trunc);require(bool(out),"無法建立還原前備份");out<<before.dump();out.flush();require(bool(out),"還原前備份寫入失敗");}
            save();return Json{{"ok",true},{"token",restoredKey},{"players",world_.players.size()}};
        }catch(...){restore(before);throw;}
    }
}
