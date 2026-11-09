// Metres relative to Verdant's elevated city origin. Mirrored by verdant_layout.hpp.
export const TILE=4.8,DECK_TOP=.70593435;
let deckTiles;
export function verdantDeckTiles(){
 if(deckTiles)return deckTiles;
 const tiles=[];
 for(let x=-4;x<=4;x++)for(let z=-4;z<=4;z++)if(Math.abs(x)>1||Math.abs(z)>1)tiles.push({x:x*TILE,z:z*TILE,y:0});
 for(let arm=0;arm<4;arm++)for(let row=-3;row<=3;row++){
  const half=Math.abs(row)===3?1:2;
  for(let col=-half;col<=half;col++){
   const u=col*TILE,v=54+row*TILE,a=arm*Math.PI/2;
   tiles.push({x:u*Math.cos(a)+v*Math.sin(a),z:v*Math.cos(a)-u*Math.sin(a),y:0});
  }
 }
 return deckTiles=tiles;
}
export const verdantBridges=Array.from({length:4},(_,i)=>({angle:i*Math.PI/2,start:21.3,end:37.5,width:3.8}));
export function verdantSurface(x,z,y){
 if(y< -1.4)return null;
 for(const tile of verdantDeckTiles())if(Math.abs(x-tile.x)<=2.4&&Math.abs(z-tile.z)<=2.4)return 0;
 for(const b of verdantBridges){const along=x*Math.sin(b.angle)+z*Math.cos(b.angle),across=x*Math.cos(b.angle)-z*Math.sin(b.angle);if(along>=b.start&&along<=b.end&&Math.abs(across)<=b.width/2){const t=(along-(b.start+b.end)/2)/((b.end-b.start)/2);return -.5*(1-t*t);}}
 return null;
}
