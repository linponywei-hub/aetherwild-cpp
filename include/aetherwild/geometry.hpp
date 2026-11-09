#pragma once
#include "domain.hpp"
namespace aw {
    struct Building {
        Id id, region;
        Vec3 position;
        double width {},
        depth {},
        height {};
        std::string style;
        double yaw{};
    };
    double cityElevation(const Id&);
    Vec3 cityArrival(const RegionDefinition&);
    double urbanRadius(const RegionDefinition&);
    std::vector<Building> cityBuildings(const Catalog&);
    std::vector<Building> buildings(const Catalog&);
    bool blocked(Vec3,const std::vector<Building>&,double radius=.45);
    double groundBelow(Vec3 feet,const Catalog&);
    bool onCityFloor(Vec3,const RegionDefinition&);
    bool clearSight(Vec3,Vec3,const std::vector<Building>&);
}
