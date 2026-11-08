#pragma once
#include "geometry.hpp"
#include "json.hpp"
#include "social.hpp"
#include "content.hpp"
#include <filesystem>
namespace aw {
    struct Motion {
        double x {},
        z {},
        yaw {},
        velocityY {},
        airTime {},
        peak {};
        bool grounded {true},
        jump {},
        sprint {};
        double lastInput {},
        lastAction {-100},
        lastSeen {};
        std::uint64_t sequence {};
        Id lift;
        double bowDrawAt {-1};
        Id bowScene, bowClass;
    };
    struct LocalProfile {
        Id player;
        std::set<Id> receipts;
    };
    struct Lift {
        double from {},
        to {},
        start {-100};
        bool moving {};
    };
    class GameRuntime {
        Catalog catalog_;
        World world_;
        SessionState social_;
        std::vector<Building> buildings_;
        std::map<Id,Motion> motion_;
        std::map<Id,LocalProfile> profiles_;
        std::map<Id,double> deadAt_;
        std::map<Id,Vec3> homes_;
        std::map<Id,Lift> lifts_;
        std::map<Id,bool> features_;
        Json chat_ {Json::array()};
        Json contentState_ {Json::object()};
        void configureContent(bool expanded);
        bool contentCommand(Player&,const Json&);
        bool npcCommand(Player&,const Json&);
        void tickNpcs();
        void positionNpcs();
        void contentTick(double);
        bool monsterAbility(Monster&,Player*);
        Json contentSnapshot(const Player&)const;
        Json equippedSkills(const Player&)const;
        bool contentAllowed(Vec3)const;
        void checkContentRegion(const Id&)const;
        void startExpandedWorld();
        void applyWorldPatch();
        mutable std::map<std::pair<int,int>,Json> terrainCache_;
        std::filesystem::path savePath_;
        double savedAt_ {};
        Id token();
        void populate();
        void settleRewards();
        void execute(Player&,const Json&);
        void physics(Player&,Motion&,double);
        void ai(double);
        Json state()const;
        void restore(const Json&);
        public: explicit GameRuntime(std::filesystem::path savePath= {},bool legacyTestWorld=false);
        Json createSession(const Json&);
        Json snapshot(const Id& token)const;
        Json catalog()const;
        Json adminMap(const Id& token)const;
        Json exportBackup(const Id& token)const;
        Json importBackup(const Id& token,const Json& body);
        Json command(const Id& token,const Json&);
        void input(const Id& token,const Json&);
        void tick(double);
        void save();
        World& world() {
            return world_;
        }
        const Catalog& definitions()const {
            return catalog_;
        }
        SessionState& sessions() {
            return social_;
        }
        std::size_t profileCount()const {
            return profiles_.size();
        }
    };
    Json vec(Vec3);
    Vec3 unvec(const Json&);
    Json itemJson(const ItemInstance&);
    ItemInstance itemFromJson(const Json&);
    Json playerJson(const Player&,bool privateData=true);
    Player playerFromJson(const Json&,const Catalog&);
}

