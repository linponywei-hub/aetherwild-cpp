// Finish every world pass before drawing the first-person layer.
export function renderFrame(renderer,world,camera,weaponScene,weaponCamera){
 renderer.render(world,camera);
 if(renderer.software)return;
 const previous=renderer.autoClear;
 try{renderer.autoClear=false;renderer.clearDepth();renderer.render(weaponScene,weaponCamera);}
 finally{renderer.autoClear=previous;}
}
