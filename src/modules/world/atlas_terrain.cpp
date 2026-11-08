#include "aetherwild/content.hpp"
namespace aw {
namespace {
struct Point {double x,z;};
struct River {double width;std::vector<Point> points;};
struct Peak {Point center;double radius,height;};
struct Surface {std::vector<std::vector<Point>> water;std::vector<River> rivers;std::vector<Peak> peaks;};
Point point(const Json& j){const auto& a=j.arrayItems();return {a[0].number(),a[1].number()};}
const Surface& surface(){static const Surface result=[] {Surface out;const auto& model=atlasSurface();for(const auto& area:model.at("water").arrayItems()){std::vector<Point> polygon;for(const auto& p:area.arrayItems())polygon.push_back(point(p));out.water.push_back(std::move(polygon));}for(const auto& river:model.at("rivers").arrayItems()){River r{river.num("width"),{}};for(const auto& p:river.at("points").arrayItems())r.points.push_back(point(p));out.rivers.push_back(std::move(r));}for(const auto& p:model.at("peaks").arrayItems())out.peaks.push_back({point(p.at("pixel")),p.num("radius"),p.num("height")});return out;}();return result;}
bool polygon(double x,double z,const std::vector<Point>& a){bool in=false;for(std::size_t i=0,j=a.size()-1;i<a.size();j=i++){auto p=a[i],q=a[j];if((p.z>z)!=(q.z>z)&&x<(q.x-p.x)*(z-p.z)/(q.z-p.z)+p.x)in=!in;}return in;}
}
bool atlasWater(Vec3 p){double x=p.x/64+633,z=p.z/64+578;const auto& model=surface();for(const auto& area:model.water)if(polygon(x,z,area))return true;
 for(const auto& river:model.rivers)for(std::size_t i=1;i<river.points.size();i++){auto a=river.points[i-1],b=river.points[i];double dx=b.x-a.x,dz=b.z-a.z,t=std::clamp(((x-a.x)*dx+(z-a.z)*dz)/(dx*dx+dz*dz),0.0,1.0);if(std::hypot(x-a.x-t*dx,z-a.z-t*dz)*64<river.width/2)return true;}
 return false;
}
double atlasHeight(Vec3 p){if(atlasWater(p))return 0;double x=p.x/64+633,z=p.z/64+578,y=8+3*std::sin(p.x/1700)*std::cos(p.z/2000);for(const auto& peak:surface().peaks){double dx=(x-peak.center.x)/peak.radius,dz=(z-peak.center.z)/peak.radius;double value=peak.height*std::exp(-(dx*dx+dz*dz));y+=value*(.9+.1*std::sin(x*.19)*std::cos(z*.17));}return y;}
double harborGround(Vec3 p,const Catalog& c){double result=-1e9;if(c.worldLimit<=1800)return result;for(const auto& harbor:atlasSurface().at("harbors").arrayItems()){const auto& r=c.regions.at(harbor.str("region"));double x=p.x-r.center.x,z=p.z-r.center.z;if(std::abs(x)>harbor.num("extent")||std::abs(z)>harbor.num("extent"))continue;for(const auto& box:harbor.at("platforms").arrayItems())if(std::abs(x-box.num("x"))<=box.num("width")/2&&std::abs(z-box.num("z"))<=box.num("depth")/2&&box.num("height")<=p.y+1.4)result=std::max(result,box.num("height"));const auto& ramp=harbor.at("ramp");if(std::abs(x-ramp.num("x"))<=ramp.num("width")/2&&std::abs(z-ramp.num("z"))<=ramp.num("depth")/2){double h=ramp.num("height")*(ramp.num("z")+ramp.num("depth")/2-z)/ramp.num("depth");if(h<=p.y+1.4)result=std::max(result,h);}}
 return result;
}
}
