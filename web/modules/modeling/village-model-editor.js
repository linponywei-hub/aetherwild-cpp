import {TransformControls} from '../../vendor/TransformControls.js';
// In-game village GLB transform editor. Local overrides are deliberately not server-authoritative.
import {VILLAGE_KEY,readVillagePoses,validateVillagePoses,resolveVillagePose,captureVillagePose,applyPose,saveLocalPose} from './village-pose.js';
const KEY=VILLAGE_KEY;
const read=()=>readVillagePoses();
export function applyVillageTransform(index,group,model,layout){
 const resolved=resolveVillagePose(index,layout);if(!resolved)return;
 applyPose(resolved.pose,group,model);delete group.userData.poseError;group.userData.poseSource=resolved.source;
 // Upgrade legacy values only after their exact angles and scale have been restored.
 if(resolved.source==='local'&&!resolved.pose.modelPosition)saveLocalPose(index,captureVillagePose(group,model));
}
export function syncVillageLayout(view,layout){
 if(!layout||view.modelEditing||view.villageLayoutRevision===layout.revision)return;
 view.villageLayoutRevision=layout.revision;
 for(const g of view.root.children)if(g.userData?.villageGLB){try{applyVillageTransform(g.userData.villageIndex,g,g.userData.villageGLB,layout);view.refreshVillageCollision?.(g);}catch(e){g.userData.poseError=e.message;}}
 view.villageEditorRefresh?.();
}
export function installVillageEditor(view){
 if(document.getElementById('aw-model-editor'))return;
 const root=document.createElement('section');root.id='aw-model-editor';
 root.innerHTML=`<button id="aw-edit-toggle" type="button">模型編輯器</button><div id="aw-edit-body" hidden><strong>GLB 房屋位置／角度編輯</strong><p>選擇房屋，調整 XYZ。變更即時預覽並儲存於此瀏覽器。</p><label>房屋 <select id="aw-edit-house"></select></label><label>操作器<select id="aw-edit-mode"><option value="rotate">旋轉</option><option value="translate">移動</option><option value="scale">縮放</option></select></label><div id="aw-edit-fields"></div><div class="aw-edit-actions"><button id="aw-edit-ground">底部貼地</button><button id="aw-edit-reset">重設此棟</button><button id="aw-edit-export">匯出 JSON</button><label>匯入設定<input id="aw-edit-import" type="file" accept=".json" hidden></label><button id="aw-edit-publish">保存至伺服器世界設定</button><button id="aw-edit-close">關閉</button></div><small id="aw-edit-status"></small></div>`;
 const css=document.createElement('style');css.textContent=`#aw-model-editor{position:fixed;z-index:10000;right:12px;top:12px;color:#fff;font:13px sans-serif}#aw-edit-toggle,#aw-edit-body button{background:#283d48;color:white;border:1px solid #94aeb7;border-radius:5px;padding:8px;cursor:pointer}#aw-edit-body{width:min(340px,calc(100vw - 24px));max-height:85vh;overflow:auto;padding:15px;margin-top:6px;border:1px solid #97aab6;border-radius:8px;background:#15222bf2;box-shadow:0 8px 30px #0009}#aw-edit-body[hidden]{display:none}#aw-edit-body p{font-size:11px;line-height:1.5}#aw-edit-body label{display:block;margin:8px 0}#aw-edit-body input{width:100%;box-sizing:border-box;padding:6px;color:#fff;background:#263740;border:1px solid #617984}#aw-edit-body select{padding:6px}#aw-edit-fields{display:grid;grid-template-columns:repeat(3,minmax(0,1fr));gap:6px}.aw-edit-actions{display:flex;flex-wrap:wrap;gap:6px;margin-top:12px}.aw-model-editing #aw-mobile-controls,.aw-model-editing #explore-hint{display:none!important}#aw-edit-status{display:block;margin-top:8px;color:#d5dfb2}`;
 document.head.append(css);document.body.append(root);
 const worldLink=document.createElement('a');worldLink.href='/world-editor.html';worldLink.target='_blank';worldLink.rel='noopener';worldLink.textContent='開啟世界編輯器：匯入 GLB／設定碰撞箱 ↗';worldLink.style.cssText='display:block;color:#d5dfb2;margin:12px 0;line-height:1.6';root.querySelector('#aw-edit-body strong').after(worldLink);
 const body=root.querySelector('#aw-edit-body'),select=root.querySelector('#aw-edit-house'),fields=root.querySelector('#aw-edit-fields'),status=root.querySelector('#aw-edit-status');
 const houses=()=>[...view.root.children].filter(g=>g.userData?.villageModel).sort((a,b)=>a.userData.villageIndex-b.userData.villageIndex);
 const current=()=>houses().find(g=>String(g.userData.villageIndex)===select.value);
 const model=g=>g?.userData?.villageGLB;
 const persist=()=>{const g=current(),m=model(g);if(!m)return;try{saveLocalPose(g.userData.villageIndex,captureVillagePose(g,m));view.refreshVillageCollision?.(g);g.userData.poseSource='local';status.textContent='已儲存此瀏覽器的完整姿態；可匯出，或由擁有者保存至伺服器。';}catch(e){status.textContent='儲存失敗：'+e.message;}};
 const gizmo=view.renderer.software?null:new TransformControls(view.camera,view.canvas);if(gizmo){gizmo.enabled=false;gizmo.visible=false;view.world.add(gizmo);}let beforeDrag;
 const attach=()=>{if(!gizmo)return;const g=current(),m=model(g);if(body.hidden||!m){gizmo.detach();gizmo.enabled=false;return;}gizmo.enabled=true;gizmo.setMode(root.querySelector('#aw-edit-mode').value);gizmo.setSpace('local');gizmo.attach(gizmo.mode==='translate'?g:m);};
 const axes=['X','Y','Z'];
 const refresh=()=>{
  const g=current(),m=model(g);fields.replaceChildren();if(!m)return;attach();root.querySelector('#aw-edit-publish').disabled=view.state?.role!=='owner';status.textContent=g.userData.poseError||({local:'已套用此瀏覽器的手動姿態',server:'已套用伺服器世界姿態'}[g.userData.poseSource]||'目前使用預設姿態；沒有讀取到手動設定');
  for(const [label,obj,prop,step] of [['位置',g,'position','0.1'],['旋轉（度）',m,'rotation','1'],['縮放',m,'scale','0.01']]){
   axes.forEach((axis,i)=>{const l=document.createElement('label');l.textContent=label+' '+axis;const input=document.createElement('input');input.type='number';input.step=step;input.value=prop==='rotation'?(obj[prop][axis.toLowerCase()]*180/Math.PI).toFixed(2):obj[prop][axis.toLowerCase()].toFixed(3);
    input.addEventListener('change',()=>{const v=Number(input.value);if(!input.value.trim()||!Number.isFinite(v)||Math.abs(v)>100000||(prop==='scale'&&(v<.0001||v>1000))){refresh();return;}obj[prop][axis.toLowerCase()]=prop==='rotation'?v*Math.PI/180:v;persist()});l.append(input);fields.append(l)});
  }
 };
 const updateList=()=>{const id=select.value;select.replaceChildren();for(const g of houses()){const o=document.createElement('option');o.value=g.userData.villageIndex;o.textContent='房屋 '+String(g.userData.villageIndex).padStart(2,'0');select.append(o)}if(id&&[...select.options].some(o=>o.value===id))select.value=id;refresh()};
 root.querySelector('#aw-edit-toggle').onclick=()=>{body.hidden=!body.hidden;view.modelEditing=!body.hidden;document.body.classList.toggle('aw-model-editing',view.modelEditing);view.keys.clear();view.lookInput?.reset();if(!body.hidden){document.exitPointerLock?.();updateList()}else attach();};
 root.querySelector('#aw-edit-close').onclick=()=>{body.hidden=true;view.modelEditing=false;document.body.classList.remove('aw-model-editing');attach();};
 root.querySelector('#aw-edit-mode').onchange=attach;
 document.addEventListener('keydown',e=>{if(e.code==='Escape'&&!body.hidden){body.hidden=true;view.modelEditing=false;document.body.classList.remove('aw-model-editing');attach();}});
 gizmo?.addEventListener('mouseDown',()=>{beforeDrag=captureVillagePose(current(),model(current()));});
 gizmo?.addEventListener('mouseUp',()=>{try{captureVillagePose(current(),model(current()));persist();}catch(error){if(beforeDrag)applyPose(beforeDrag,current(),model(current()));status.textContent=error.message;}beforeDrag=null;refresh();});
 select.onchange=refresh;
 root.querySelector('#aw-edit-reset').onclick=()=>{const g=current(),m=model(g);if(!m)return;const baseline=g.userData.villageBaseline;if(!baseline)return;applyPose(baseline,g,m);persist();refresh();status.textContent='已預覽預設姿態；伺服器設定尚未更動';};
 root.querySelector('#aw-edit-ground').onclick=()=>{const g=current(),m=model(g);if(!m)return;const box=new view.THREE.Box3().setFromObject(m);const h=view.landHeight(g.position.x,g.position.z);if(Number.isFinite(box.min.y)&&Number.isFinite(h)){g.position.y+=h-box.min.y-.08;persist();refresh()}};
 root.querySelector('#aw-edit-export').onclick=()=>{try{const blob=new Blob([JSON.stringify({format:'aetherwild-village-poses',version:2,poses:read()},null,2)],{type:'application/json'});const a=document.createElement('a');a.href=URL.createObjectURL(blob);a.download='aetherwild-village-model-transforms.json';a.click();setTimeout(()=>URL.revokeObjectURL(a.href),1000);}catch(e){status.textContent=e.message;}};
 root.querySelector('#aw-edit-import').onchange=async e=>{const file=e.target.files[0];e.target.value='';if(!file)return;try{if(file.size>65536)throw Error('設定檔超過 64 KB');const parsed=JSON.parse(await file.text()),poses=validateVillagePoses(parsed.format==='aetherwild-village-poses'?parsed.poses:parsed);const combined={...read(),...poses};localStorage.setItem(KEY+'-backup',localStorage.getItem(KEY)||'{}');localStorage.setItem(KEY,JSON.stringify(combined));for(const g of houses()){applyVillageTransform(g.userData.villageIndex,g,model(g),view.state?.content?.villageLayout);view.refreshVillageCollision?.(g);}refresh();status.textContent='已還原匯入姿態；原設定另存本機備份，尚未發布至伺服器';}catch(err){status.textContent='匯入失敗：'+err.message;}};
 root.querySelector('#aw-edit-publish').onclick=async()=>{const g=current(),m=model(g),button=root.querySelector('#aw-edit-publish');if(!m||view.state?.role!=='owner')return;button.disabled=true;try{const pose=captureVillagePose(g,m),revision=view.state.content?.villageLayout?.revision||0;const ok=await view.cb.publishVillagePose?.({index:g.userData.villageIndex,revision,pose});if(!ok)throw Error('伺服器未接受變更，請查看提示並重新讀取');pose.serverRevision=view.state.content.villageLayout.revision;saveLocalPose(g.userData.villageIndex,pose);status.textContent='已保存至伺服器世界設定，會包含於世界備份；建議另匯出 JSON';}catch(err){status.textContent='未發布：'+err.message;}finally{button.disabled=view.state?.role!=='owner';}};
 view.villageEditorRefresh=()=>{if(!body.hidden)updateList()};
}
