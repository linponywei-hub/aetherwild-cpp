#include "aetherwild/content.hpp"
namespace aw {
namespace {
struct Point {double x,z;};
struct River {double width;std::vector<Point> points;};
struct Peak {Point center;double radius,height;};
struct Biome {Id id;std::vector<Point> points;};
struct FlatZone {Point center;double radius,feather,base,amplitude;};
struct Surface {std::vector<std::vector<Point>> water;std::vector<River> rivers;std::vector<Peak> peaks;std::vector<Biome> biomes;std::vector<FlatZone> flat;std::uint32_t seed{};double coast=560,bank=320;};
Point point(const Json& j){const auto& a=j.arrayItems();return {a[0].number(),a[1].number()};}
const Surface& surface(){static const Surface result=[] {
 Surface out;const auto& model=atlasSurface();
 for(const auto& area:model.at("water").arrayItems()){std::vector<Point> polygon;for(const auto& p:area.arrayItems())polygon.push_back(point(p));out.water.push_back(std::move(polygon));}
 for(const auto& river:model.at("rivers").arrayItems()){River r{river.num("width"),{}};for(const auto& p:river.at("points").arrayItems())r.points.push_back(point(p));out.rivers.push_back(std::move(r));}
 for(const auto& p:model.at("peaks").arrayItems())out.peaks.push_back({point(p.at("pixel")),p.num("radius"),p.num("height")});
 for(const auto& b:model.at("biomes").arrayItems()){Biome biome{b.str("id"),{}};for(const auto& p:b.at("polygon").arrayItems())biome.points.push_back(point(p));out.biomes.push_back(std::move(biome));}
 const auto& relief=model.at("relief");out.seed=static_cast<std::uint32_t>(relief.num("seed"));out.coast=relief.num("coastBlendMeters");out.bank=relief.num("riverBankMeters");
 for(const auto& z:relief.at("flatZones").arrayItems())out.flat.push_back({point(z.at("pixel")),z.num("radius"),z.num("feather"),z.num("base"),z.num("amplitude")});
 return out;}();return result;}
bool polygon(double x,double z,const std::vector<Point>& a){bool in=false;for(std::size_t i=0,j=a.size()-1;i<a.size();j=i++){auto p=a[i],q=a[j];if((p.z>z)!=(q.z>z)&&x<(q.x-p.x)*(z-p.z)/(q.z-p.z)+p.x)in=!in;}return in;}
double segmentDistance(Point p,Point a,Point b){double dx=b.x-a.x,dz=b.z-a.z,len=dx*dx+dz*dz;double t=len?std::clamp(((p.x-a.x)*dx+(p.z-a.z)*dz)/len,0.0,1.0):0;return std::hypot(p.x-a.x-t*dx,p.z-a.z-t*dz);}
double edgeDistance(Point p,const std::vector<Point>& polygon){double d=1e20;for(std::size_t i=0,j=polygon.size()-1;i<polygon.size();j=i++)d=std::min(d,segmentDistance(p,polygon[j],polygon[i]));return d*64;}
double smooth(double a,double b,double x){double t=std::clamp((x-a)/(b-a),0.0,1.0);return t*t*(3-2*t);}
double mix(double a,double b,double t){return a+(b-a)*t;}
std::uint32_t hash(std::uint32_t x,std::uint32_t z,std::uint32_t seed){std::uint32_t h=x*0x8da6b343U^z*0xd8163841U^seed;h^=h>>16;h*=0x7feb352dU;h^=h>>15;h*=0x846ca68bU;return h^(h>>16);}
double noise(double x,double z,std::uint32_t seed){auto ix=static_cast<std::int32_t>(std::floor(x)),iz=static_cast<std::int32_t>(std::floor(z));double tx=x-ix,tz=z-iz;tx=tx*tx*(3-2*tx);tz=tz*tz*(3-2*tz);auto value=[&](int dx,int dz){return hash(static_cast<std::uint32_t>(ix+dx),static_cast<std::uint32_t>(iz+dz),seed)/4294967295.0;};return mix(mix(value(0,0),value(1,0),tx),mix(value(0,1),value(1,1),tx),tz);}
double fractal(double x,double z,std::uint32_t seed){double value=0,weight=0,amplitude=1;for(int i=0;i<4;i++){value+=amplitude*noise(x,z,seed+static_cast<std::uint32_t>(i)*1013U);weight+=amplitude;amplitude*=.5;x*=2.02;z*=2.02;}return value/weight;}
}
bool atlasWater(Vec3 p){const Point q{p.x/64+633,p.z/64+578};const auto& model=surface();for(const auto& area:model.water)if(polygon(q.x,q.z,area))return true;
 for(const auto& river:model.rivers)for(std::size_t i=1;i<river.points.size();i++)if(segmentDistance(q,river.points[i-1],river.points[i])*64<river.width/2)return true;
 return false;
}
double atlasHeight(Vec3 p){
 if(atlasWater(p))return 0;
 const auto& model=surface();const Point pixel{p.x/64+633,p.z/64+578};
 const double broad=fractal(p.x/1900,p.z/1900,model.seed),detail=fractal(p.x/180,p.z/180,model.seed+7);
 double y=6+18*broad+2*(detail-.5);
 // Authored peak envelopes keep mountains in the reference map. Domain warping
 // and ridges break the old smooth domes without moving their geographic anchors.
 const double warpX=(noise(p.x/2400,p.z/2400,model.seed+11)-.5)*380;
 const double warpZ=(noise(p.x/2400,p.z/2400,model.seed+13)-.5)*380;
 const double ridgeSignal=fractal((p.x+warpX)/720,(p.z+warpZ)/720,model.seed+23)*2-1;
 // Smooth ridge crests instead of a sharp absolute-value cusp. This keeps
 // adjacent 8m terrain samples continuous and prevents needle-like peaks.
 const double ridge=1-std::sqrt(ridgeSignal*ridgeSignal+.035*.035);
 for(const auto& peak:model.peaks){double dx=(pixel.x-peak.center.x)/peak.radius,dz=(pixel.z-peak.center.z)/peak.radius;double envelope=std::exp(-(dx*dx+dz*dz));y+=peak.height*envelope*(.69+.31*ridge)+peak.height*envelope*.045*(detail-.5);}
 for(const auto& biome:model.biomes){if(!polygon(pixel.x,pixel.z,biome.points))continue;double w=smooth(0,500,edgeDistance(pixel,biome.points));
  if(biome.id=="desert"){double phase=p.x/210+p.z/410+3*broad;double dunes=std::sin(phase)*.7+std::sin(phase*.53+1.7)*.3; y=mix(y,8+10*broad+14*std::pow(.5+.5*dunes,2),w);}
  if(biome.id=="mesa"){double t=fractal(p.x/1500,p.z/1500,model.seed+31)*4;double terrace=(std::floor(t)+smooth(.25,.75,t-std::floor(t)))/4;y=mix(y,32+240*terrace+4*(detail-.5),w*.85);}
  if(biome.id=="marsh")y=mix(y,1.5+.8*broad,w);
 }
 for(const auto& flat:model.flat){double d=std::hypot(pixel.x-flat.center.x,pixel.z-flat.center.z);double w=1-smooth(flat.radius,flat.radius+flat.feather,d);y=mix(y,flat.base+flat.amplitude*(2*detail-1),w);}
 double shore=1e20;for(const auto& water:model.water)shore=std::min(shore,edgeDistance(pixel,water));
 double bank=1e20;for(const auto& r:model.rivers)for(std::size_t i=1;i<r.points.size();i++)bank=std::min(bank,segmentDistance(pixel,r.points[i-1],r.points[i])*64-r.width/2);
 y*=std::min(smooth(0,model.coast,shore),smooth(0,model.bank,bank));
 return std::max(0.0,y);
}
double harborGround(Vec3 p,const Catalog& c){double result=-1e9;if(c.worldLimit<=1800)return result;for(const auto& harbor:atlasSurface().at("harbors").arrayItems()){const auto& r=c.regions.at(harbor.str("region"));double x=p.x-r.center.x,z=p.z-r.center.z;if(std::abs(x)>harbor.num("extent")||std::abs(z)>harbor.num("extent"))continue;for(const auto& box:harbor.at("platforms").arrayItems())if(std::abs(x-box.num("x"))<=box.num("width")/2&&std::abs(z-box.num("z"))<=box.num("depth")/2&&box.num("height")<=p.y+1.4)result=std::max(result,box.num("height"));const auto& ramp=harbor.at("ramp");if(std::abs(x-ramp.num("x"))<=ramp.num("width")/2&&std::abs(z-ramp.num("z"))<=ramp.num("depth")/2){double h=ramp.num("height")*(ramp.num("z")+ramp.num("depth")/2-z)/ramp.num("depth");if(h<=p.y+1.4)result=std::max(result,h);}}
 return result;
}
}
