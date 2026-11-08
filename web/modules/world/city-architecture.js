// Appearance recipes only; every solid building footprint is authored by C++.
export function cityTheme(region){return ({windchime:'village',verdant:'tree',cinder:'industrial',frosthold:'snow',astral:'astral',sundune:'desert',tidehaven:'harbor',aw_westport:'harbor',aw_southmill:'harbor',aw_crownreach:'royal',aw_northwatch:'fortress',aw_eastglass:'glass'})[region]||'village';}
export function buildingParts(b){
 const theme=cityTheme(b.region),kind=b.style.split('|')[1]||'house',w=b.width,d=b.depth,h=b.height;
 const palette={village:['#c9bea1','#79664c','#696d52'],tree:['#a5b28b','#5a7452','#94704a'],industrial:['#7b776b','#484e4c','#a46d48'],snow:['#a9bcc0','#e4ece9','#657c88'],astral:['#d6d8dd','#8793ba','#a99462'],desert:['#d4bc8d','#b38d61','#e2d5b0'],harbor:['#b3ad94','#577979','#83674a'],royal:['#c9c7b5','#627b92','#d3b87b'],fortress:['#89948c','#505f61','#ad9a77'],glass:['#b3c6cb','#79aeb9','#ccb88b']}[theme];
 const parts=[],add=(shape,x,y,z,sx,sy,sz,color,glow=false)=>parts.push({shape,x,y,z,sx,sy,sz,color,glow});
 if(kind==='lamp'){add('cylinder',0,h*.45,0,.22,h*.9,.22,'#4b5650');add('box',0,h*.88,0,.9,.12,.9,'#9f8e64');add('box',0,h*.96,0,.65,.55,.65,'#ffe1a0',true);return parts;}
 if(kind==='wall'){add('box',0,h*.42,0,w,h*.84,d,palette[0]);const n=Math.min(16,Math.max(2,Math.round(Math.max(w,d)/8)));for(let i=0;i<n;i++){const t=(i+.5)/n-.5;add('box',w>d?t*w:0,h*.93,w>d?0:t*d,w>d?w/n*.58:w,h*.14,w>d?d:d/n*.58,palette[0]);}return parts;}
 if(['tower','spire'].includes(kind)){add('cylinder',0,h*.42,0,w,h*.84,d,palette[0]);add('cone',0,h*.92,0,w,h*.16,d,palette[1]);for(const side of [-1,1])add('box',side*w*.35,h*.65,d*.45,w*.12,h*.1,.1,palette[2],true);return parts;}
 add('box',0,h*.045,0,w,h*.09,d,palette[2]);add('box',0,h*.4,0,w*.96,h*.7,d*.96,palette[0]);
 if(theme==='industrial'||kind==='foundry'){add('box',0,h*.8,0,w,h*.1,d,palette[1]);for(const side of [-1,1]){add('cylinder',side*w*.3,h*.86,-d*.2,w*.12,h*.28,d*.12,palette[1]);add('box',side*w*.32,h*.3,d*.49,w*.18,h*.23,.12,'#e5a15e',true);}}
 else if(theme==='desert'){add('box',0,h*.78,0,w,h*.1,d,palette[2]);add('dome',0,h*.84,0,w*.65,h*.3,d*.65,palette[1]);}
 else{add('roof',0,h*.84,0,w,h*.32,d,palette[1]);if(theme==='snow')add('cylinder',w*.28,h*.85,-d*.2,.8,h*.3,.8,palette[2]);}
 if(kind==='palace'||kind==='keep'||kind==='temple'){
  for(const side of [-1,1]){add('box',side*w*.34,h*.45,d*.44,w*.18,h*.7,d*.1,palette[2]);add('cylinder',side*w*.42,h*.52,-d*.3,w*.12,h*.85,d*.18,palette[0]);add('cone',side*w*.42,h*.96,-d*.3,w*.15,h*.12,d*.22,palette[1]);}
 }
 // Doors are recessed appearance, not a promise of an enterable interior.
 add('box',0,h*.19,d*.49,Math.min(3,w*.22),h*.29,.12,'#514b3e');
 for(const side of [-1,1]){add('box',side*w*.28,h*.47,d*.493,w*.16,h*.16,.14,'#d5d5ad',true);add('box',side*w*.28,h*.47,d*.505,.1,h*.17,.05,palette[2]);}
 if(kind==='shop')add('box',0,h*.42,d*.51,w*.8,.2,d*.16,palette[1]);
 if(kind==='guild'){add('box',-w*.4,h*.55,d*.5,.15,h*.3,.15,palette[2]);add('box',-w*.3,h*.62,d*.52,w*.2,h*.17,.1,'#a0824c');}
 if(theme==='tree'){for(const side of [-1,1])add('cylinder',side*w*.43,h*.35,0,.8,h*.7,.8,palette[2]);}
 return parts;
}
export function chunkKey(x,z,size=128){return Math.floor(x/size)+':'+Math.floor(z/size);}
export function desiredCells(x,z,size=160){const a=Math.floor(x/size),b=Math.floor(z/size),result=[];for(let dz=-1;dz<=1;dz++)for(let dx=-1;dx<=1;dx++)result.push({x:a+dx,z:b+dz,d:dx*dx+dz*dz});return result.sort((a,b)=>a.d-b.d);}
