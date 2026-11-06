#pragma once
#include "services.hpp"
#include <filesystem>
namespace aw {
    // Local character snapshot, never the production database. Saves only in the world scene.
    class CharacterFile {
        public: static void save(const Player&,const std::filesystem::path&,const Catalog&);
        static Player load(const std::filesystem::path&,const Catalog&);
    };
}
