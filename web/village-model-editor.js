// In-game village GLB transform editor. Local overrides are deliberately not server-authoritative.
const KEY='aw-village-glb-transforms-v1';
const read=()=>{try{return JSON.parse(localStorage.getItem(KEY)||'{}')}catch{return {}}};
export function applyVillageTransform(index,group,model){
 const t=read()[index];if(!t)return;
 if(Array.isArray(t.position))group.position.fromArray(t.position);
 if(Array.isArray(t.rotation))model.rotation.set(...t.rotation);
 if(Array.isArray(t.scale))model.scale.set(...t.scale);
 group.updateMatrixWorld(true);
}
export function installVillageEditor(view){
 if(document.getElementById('aw-model-editor'))return;
 const root=document.createElement('section');root.id='aw-model-editor';
 root.innerHTML=`<button id="aw-edit-toggle" type="button">模型編輯器</button><div id="aw-edit-body" hidden><strong>GLB 房屋位置／角度編輯</strong><p>選擇房屋，調整 XYZ。變更即時預覽並儲存於此瀏覽器。</p><label>房屋 <select id="aw-edit-house"></select></label><div id="aw-edit-fields"></div><div class="aw-edit-actions"><button id="aw-edit-ground">底部貼地</button><button id="aw-edit-reset">重設此棟</button><button id="aw-edit-export">匯出 JSON</button><button id="aw-edit-close">關閉</button></div><small id="aw-edit-status"></small></div>`;
 const css=document.createElement('style');css.textContent=`#aw-model-editor{position:fixed;z-index:10000;right:12px;top:12px;color:#fff;font:13px sans-serif}#aw-edit-toggle,#aw-edit-body button{background:#283d48;color:white;border:1px solid #94aeb7;border-radius:5px;padding:8px;cursor:pointer}#aw-edit-body{width:min(340px,calc(100vw - 24px));max-height:85vh;overflow:auto;padding:15px;margin-top:6px;border:1px solid #97aab6;border-radius:8px;background:#15222bf2;box-shadow:0 8px 30px #0009}#aw-edit-body[hidden]{display:none}#aw-edit-body p{font-size:11px;line-height:1.5}#aw-edit-body label{display:block;margin:8px 0}#aw-edit-body input{width:100%;box-sizing:border-box;padding:6px;color:#fff;background:#263740;border:1px solid #617984}#aw-edit-body select{padding:6px}#aw-edit-fields{display:grid;grid-template-columns:repeat(3,minmax(0,1fr));gap:6px}.aw-edit-actions{display:flex;flex-wrap:wrap;gap:6px;margin-top:12px}#aw-edit-status{display:block;margin-top:8px;color:#d5dfb2}`;
 document.head.append(css);document.body.append(root);
 const body=root.querySelector('#aw-edit-body'),select=root.querySelector('#aw-edit-house'),fields=root.querySelector('#aw-edit-fields'),status=root.querySelector('#aw-edit-status');
 const houses=()=>[...view.root.children].filter(g=>g.userData?.villageModel).sort((a,b)=>a.userData.villageIndex-b.userData.villageIndex);
 const current=()=>houses().find(g=>String(g.userData.villageIndex)===select.value);
 const model=g=>g?.userData?.villageGLB;
 const persist=()=>{const g=current(),m=model(g);if(!m)return;const data=read();data[g.userData.villageIndex]={position:g.position.toArray(),rotation:[m.rotation.x,m.rotation.y,m.rotation.z],scale:m.scale.toArray()};localStorage.setItem(KEY,JSON.stringify(data));view.refreshVillageCollision?.(g);status.textContent='已儲存本機設定；匯出 JSON 可交給開發者套用至正式版。'};
 const axes=['X','Y','Z'];
 const refresh=()=>{
  const g=current(),m=model(g);fields.replaceChildren();if(!m)return;
  for(const [label,obj,prop,step] of [['位置',g,'position','0.1'],['旋轉（度）',m,'rotation','1'],['縮放',m,'scale','0.01']]){
   axes.forEach((axis,i)=>{const l=document.createElement('label');l.textContent=label+' '+axis;const input=document.createElement('input');input.type='number';input.step=step;input.value=prop==='rotation'?(obj[prop][axis.toLowerCase()]*180/Math.PI).toFixed(2):obj[prop][axis.toLowerCase()].toFixed(3);
    input.addEventListener('change',()=>{const v=Number(input.value);if(!Number.isFinite(v))return;obj[prop][axis.toLowerCase()]=prop==='rotation'?v*Math.PI/180:v;persist()});l.append(input);fields.append(l)});
  }
 };
 const updateList=()=>{const id=select.value;select.replaceChildren();for(const g of houses()){const o=document.createElement('option');o.value=g.userData.villageIndex;o.textContent='房屋 '+String(g.userData.villageIndex).padStart(2,'0');select.append(o)}if(id&&[...select.options].some(o=>o.value===id))select.value=id;refresh()};
 root.querySelector('#aw-edit-toggle').onclick=()=>{body.hidden=!body.hidden;if(!body.hidden){document.exitPointerLock?.();updateList()}};
 root.querySelector('#aw-edit-close').onclick=()=>body.hidden=true;
 select.onchange=refresh;
 root.querySelector('#aw-edit-reset').onclick=()=>{const g=current(),m=model(g);if(!m)return;const data=read();delete data[g.userData.villageIndex];localStorage.setItem(KEY,JSON.stringify(data));const baseline=g.userData.villageBaseline;if(baseline){g.position.fromArray(baseline.position);m.rotation.set(...baseline.rotation);m.scale.fromArray(baseline.scale)}persist();refresh()};
 root.querySelector('#aw-edit-ground').onclick=()=>{const g=current(),m=model(g);if(!m)return;const box=new view.THREE.Box3().setFromObject(m);const h=view.landHeight(g.position.x,g.position.z);if(Number.isFinite(box.min.y)&&Number.isFinite(h)){g.position.y+=h-box.min.y-.08;persist();refresh()}};
 root.querySelector('#aw-edit-export').onclick=()=>{const blob=new Blob([JSON.stringify(read(),null,2)],{type:'application/json'});const a=document.createElement('a');a.href=URL.createObjectURL(blob);a.download='aetherwild-village-model-transforms.json';a.click();setTimeout(()=>URL.revokeObjectURL(a.href),1000)};
 view.villageEditorRefresh=()=>{if(!body.hidden)updateList()};
}
