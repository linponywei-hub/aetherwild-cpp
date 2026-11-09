#pragma once
#include "aetherwild/domain.hpp"
namespace aw {
    class QuestSystem {
        public: static void accept(Player&,const Id&,const Id& npc,const Catalog&);
        static void record(Player&,const std::string& type,const Id& target,const Catalog&);
        static void claim(Player&,const Id&,const Id& npc,const Catalog&);
    };
}
