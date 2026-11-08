export function appearanceSeed(id){let h=0;for(const ch of id)h=(h*31+ch.charCodeAt(0))>>>0;return h;}
const part=(shape,x,y,z,sx,sy,sz,color)=>({shape,x,y,z,sx,sy,sz,color});
export function weaponParts(type){const steel='#cdd5d6',gold='#b99c64',wood='#705742';
 if(type==='unarmed'||!type)return [];
 if(type==='bow'||type==='crossbow')return [part('arc',0,.1,0,.75,.95,.8,wood),part('box',0,.1,0,.012,.94,.012,'#d9d0b2')];
 if(['staff','focus','totem','instrument'].includes(type))return [part('cylinder',0,0,0,.07,1.35,.07,wood),part('dome',0,.7,0,.22,.25,.22,'#9dbae3')];
 if(type==='daggers')return [-1,1].flatMap(side=>[part('box',side*.36,.04,.05,.06,.36,.035,steel),part('box',side*.36,-.2,.05,.075,.15,.065,wood)]);
 if(type==='cannon'||type==='toolgun')return [part('box',0,.02,.15,.22,.2,.6,'#62716c'),part('cylinder',0,.12,0,.12,.25,.12,gold)];
 const parts=[part('box',0,.3,0,type==='greatsword'?.13:.07,type==='spear'?1.35:.85,.035,steel),part('box',0,-.18,0,.25,.045,.08,gold),part('cylinder',0,-.3,0,.07,.2,.07,wood)];
 if(type==='shieldblade')parts.push(part('box',-.65,.05,0,.36,.55,.09,'#81938a'));
 if(type==='hammer')parts.push(part('box',0,.65,0,.45,.25,.23,steel));return parts;
}
export function humanoidParts(role,id){const seed=appearanceSeed(id),skin=['#d1ad87','#b98a68','#e0c4a0','#967052'][seed%4],hair=['#453e34','#777064','#a2977c','#594536'][seed%4];const coat=({guild:'#b19b68',merchant:'#658b68',smith:'#80664e',healer:'#bfc8bd',cartographer:'#6b8c9f'})[role]||['#697f73','#7a6d87','#9a835e','#7b8e9a'][seed%4];
 const p=[part('box',0,1.16,0,.5,.7,.28,coat),part('dome',0,1.77,.015,.38,.45,.36,skin),part('dome',0,1.96,-.035,.39,.2,.35,hair),part('box',0,.82,0,.51,.08,.3,'#5d5140')];
 for(const s of [-1,1])p.push(part('box',s*.34,1.15,0,.14,.6,.17,coat),part('box',s*.34,.82,.025,.13,.14,.14,skin),part('box',s*.085,1.8,.188,.035,.035,.02,'#292e2c'));
 if(role==='merchant')p.push(part('box',-.28,.83,-.18,.2,.28,.24,'#9d8058'));
 if(role==='smith')p.push(part('box',.35,.85,.12,.3,.12,.12,'#9b9e92'));
 if(role==='healer')p.push(part('box',0,1.2,.15,.22,.06,.025,'#f0df9c'),part('box',0,1.2,.16,.06,.25,.025,'#f0df9c'));
 if(role==='cartographer')p.push(part('cylinder',.34,.84,.15,.12,.34,.12,'#d7c6a2'));
 if(role==='guild')p.push(part('box',0,1.3,.17,.12,.17,.035,'#d6bf81'));
 return {body:p,legs:[-1,1].map(s=>[part('box',s*.13,.45,0,.18,.68,.2,'#50594e'),part('box',s*.13,.12,.07,.19,.13,.31,'#454c41')])};
}
export function nameplateVisible(position,camera,distance=80){return Math.hypot(position.x-camera.x,position.y-camera.y,position.z-camera.z)<=distance;}
