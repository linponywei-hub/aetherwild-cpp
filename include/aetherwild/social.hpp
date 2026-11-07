#pragma once
#include "services.hpp"
namespace aw {
    enum class Role {Player, Admin, Owner};
    struct Account {
        Id id;
        Role role {Role::Player};
    };
    // Created by trusted authentication adapter.
    struct Party {
        Id id, leader;
        std::set<Id> members, invited;
    };
    struct SessionState {
        std::map<Id,Account> accounts;
        std::map<Id,Party> parties;
        std::map<Id,Id> memberParty;
        bool pvpEnabled {false};
    };
    class PartySystem {
        public: static Id create(World&,SessionState&,const Id& leader);
        static void invite(const World&,SessionState&,const Id& leader,const Id& target);
        static void join(const World&,SessionState&,const Id& player,const Id& party);
        static void leave(SessionState&,const Id& player);
    };
    class PvpSystem {
        public: static double attack(World&,const Catalog&,const SessionState&,const Id& from,const Id& to,double bowPower=1);
    };
    class AdministrationSystem {
        public: static void setPvp(World&,SessionState&,const Id& actorAccount,bool enabled);
        static void grantAdmin(World&,SessionState&,const Id& actorAccount,const Id& targetAccount);
        static void teleportToCity(World&,const Catalog&,const SessionState&,const Id& actorAccount,const Id& player,const Id& city);
    };
}
