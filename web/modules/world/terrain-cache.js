import {gridHeight} from './landscape-model.js';
export class TerrainCache {
 constructor(max=16){this.max=max;this.tiles=new Map();}
 put(tile){if(!tile)return;if(this.version!==tile.version){this.tiles.clear();this.version=tile.version;}const key=tile.x+':'+tile.z;this.tiles.delete(key);this.tiles.set(key,tile);if(this.tiles.size>this.max)this.tiles.delete(this.tiles.keys().next().value);}
 height(x,z){for(const tile of [...this.tiles.values()].reverse()){const height=gridHeight(tile,x,z);if(height!==null)return height;}return null;}
}
