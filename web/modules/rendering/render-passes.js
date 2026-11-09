// Finish every world pass before drawing the first-person layer.
export function renderFrame(renderer,world,camera,weaponScene,weaponCamera){
 renderer.render(world,camera);
 if(renderer.software)return;
 const previous=renderer.autoClear;
 try{renderer.autoClear=false;renderer.clearDepth();renderer.render(weaponScene,weaponCamera);}
 finally{renderer.autoClear=previous;}
}

export function attachViewmodel(hand,camera,software=false){camera.add(hand);hand.traverse(o=>{if(!o.material)return;o.frustumCulled=false;o.renderOrder=10000;for(const material of Array.isArray(o.material)?o.material:[o.material]){material.fog=false;material.depthTest=!software;material.depthWrite=!software;}});}
