#pragma once
#include "aetherwild/domain.hpp"
namespace aw {
    class Simulation {
        Catalog catalog_;
        World world_;
        public: Simulation();
        const Catalog& catalog() const {
            return catalog_;
        }
        World& world() {
            return world_;
        }
        const World& world()const {
            return world_;
        }
        Player& addPlayer(const Id&,const std::string&,const Id& profession="warrior");
        void advance(double dt);
    };
}
