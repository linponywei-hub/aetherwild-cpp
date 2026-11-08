#pragma once
#include "aetherwild/domain.hpp"
namespace aw {
    class GatheringSystem {
        public: static void begin(World&,Player&,const Id& node,const Catalog&);
        static void finish(World&,Player&,const Id& node,const Catalog&);
        static void cancel(World&,Player&,const Id& node);
    };
}
