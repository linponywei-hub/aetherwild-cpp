#include "aetherwild/talents.hpp"
#include "aetherwild/runtime.hpp"
#include <fstream>
#include <random>
#include <cstdlib>
#include <chrono>
#include "../runtime/support.hpp"
namespace aw {
using namespace runtime_detail;
    void GameRuntime::settleRewards() {
        for(auto&[id,r]:world_.rewards)if(world_.players.contains(r.player)) {
            auto&p=world_.players.at(r.player);
            if(!p.processedRewards.contains(id))RewardSystem::claim(world_,p,id,catalog_);
        }
    }
}
