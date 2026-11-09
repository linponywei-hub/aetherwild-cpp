#include "aetherwild/geometry.hpp"
#include "aetherwild/content.hpp"
namespace aw {
    double cityElevation(const Id&id) {
        return id=="astral"?62:id=="verdant"?32:0;
    }
    Vec3 cityArrival(const RegionDefinition&r) {
        auto p=r.center;
        p.y=r.center.y+cityElevation(r.id);
        return p;
    }
    std::vector<Building> buildings(const Catalog&c) {
        if(c.worldLimit>1800)return cityBuildings(c);
        std::vector<Building> v;
        for(auto&[id,r]:c.regions) {
            if(r.wilderness)continue;
            for(int i=0;i<(r.city?12:8);i++) {
                double a=i*6.283185307/(r.city?12:8),rad=r.city?48:28;
                v.push_back( {
                    id+"_house_"+std::to_string(i),id, {r.center.x+std::cos(a)*rad,r.center.y+cityElevation(id),r.center.z+std::sin(a)*rad},
                    r.city?12.0:8.0,r.city?10.0:7.0,r.city?12.0:7.0,id
                });
            }
        }
        return v;
    }
    bool blocked(Vec3 p,const std::vector<Building>& bs,double radius) {
        for(const auto& b:bs){
            if(b.region=="windchime"&&!b.style.ends_with("|starter_wall")&&!b.style.ends_with("|starter_fence"))continue;
            if(p.y+1.7<=b.position.y||p.y>=b.position.y+b.height)continue;
            const double dx=p.x-b.position.x,dz=p.z-b.position.z,co=std::cos(b.yaw),si=std::sin(b.yaw);
            if(std::abs(co*dx-si*dz)<b.width/2+radius&&std::abs(si*dx+co*dz)<b.depth/2+radius)return true;
        }
        return false;
    }
    bool onCityFloor(Vec3 p,const RegionDefinition&r) {
        double dx=p.x-r.center.x,dz=p.z-r.center.z;
        if(cityElevation(r.id)==0)return std::hypot(dx,dz)<=r.radius&&std::abs(p.y-r.center.y)<6;
        if(std::abs(p.y-cityElevation(r.id))>4)return false;
        if(std::hypot(dx,dz)<=94) {
            if(r.id=="verdant"&&std::abs(dx)<8&&dz>30)return false;
            return true;
        }
        return r.id=="astral"&&std::abs(dx)<7&&dz>=85&&dz<=116;
    }
    double groundBelow(Vec3 p,const Catalog&c) {
        double y=std::max(worldLandHeight(p,c),harborGround(p,c));
        if(c.worldLimit>1800){
            struct Wall {double x,z,w,d,height,co,si,base;};
            static const auto walls=[&] {std::vector<Wall> out;const auto center=c.regions.at("windchime").center;
                for(const auto& key:{"walls","fences"})for(const auto& b:atlasSurface().at("starterVillage").at(key).arrayItems()){double x=b.num("x"),z=b.num("z"),yaw=b.num("yaw");out.push_back({x,z,b.num("width")/2,b.num("depth")/2,b.num("height"),std::cos(yaw),std::sin(yaw),worldLandHeight({center.x+x,0,center.z+z},c)});}
                return out;
            }();
            const auto center=c.regions.at("windchime").center;for(const auto& b:walls){double dx=p.x-center.x-b.x,dz=p.z-center.z-b.z,top=b.base+b.height;if(std::abs(b.co*dx-b.si*dz)<=b.w&&std::abs(b.si*dx+b.co*dz)<=b.d&&top<=p.y+1.4)y=std::max(y,top);}

        }
        for(auto&[id,r]:c.regions) {
            double dx=p.x-r.center.x,dz=p.z-r.center.z,h=cityElevation(id);
            if(!h)continue;
            if(id=="verdant"&&std::abs(dx)<7&&dz>=30&&dz<=150) {
                double ramp=(150-dz)*32/120;
                if(ramp<=p.y+1.4)y=std::max(y,ramp);
            }
            bool disc=std::hypot(dx,dz)<=94&&!(id=="verdant"&&std::abs(dx)<8&&dz>30);
            bool bridge=id=="astral"&&std::abs(dx)<7&&dz>=85&&dz<=116;
            if((disc||bridge)&&h<=p.y+1.4)y=std::max(y,h);
        }
        return y;
    }
    bool clearSight(Vec3 a,Vec3 b,const std::vector<Building>&v) {
        double d=distance(a,b);
        for(double t=1;t<d;t+=1.5) {
            double k=t/d;
            if(blocked( {a.x+(b.x-a.x)*k,a.y+(b.y-a.y)*k,a.z+(b.z-a.z)*k},
            v,.05))return false;
        }
        return true;
    }
}

