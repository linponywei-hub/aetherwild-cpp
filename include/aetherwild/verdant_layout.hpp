#pragma once
#include <cmath>
namespace aw {
// Relative city coordinates. Keep verdant-layout.js in sync.
inline bool verdantDeck(double x,double z){
 if(std::abs(x)<=21.6&&std::abs(z)<=21.6&&(std::abs(x)>=7.2||std::abs(z)>=7.2))return true;
 for(int arm=0;arm<4;arm++){
  const double u=arm%2?z:x,v=(arm==0?z:arm==1?x:arm==2?-z:-x)-54;
  if(std::abs(v)>16.8)continue;
  const double half=std::abs(v)>=12?7.2:12;
  if(std::abs(u)<=half)return true;
 }
 return false;
}
inline double verdantSurface(double x,double z){
 if(verdantDeck(x,z))return 0;
 for(int arm=0;arm<4;arm++){
  const double u=arm%2?z:x,v=arm==0?z:arm==1?x:arm==2?-z:-x;
  if(v>=21.3&&v<=37.5&&std::abs(u)<=1.9){const double t=(v-29.4)/8.1;return -.5*(1-t*t);}
 }
 return -1e9;
}
}
