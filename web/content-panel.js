// Presentation only. Every mutation goes through the existing authenticated C++ command path.
let query='';
export function contentFilter(value){query=String(value||'').slice(0,80);}
export function contentPanel({tab,catalog,state,selectedNpc,esc,button,dist}){
 const c=state.content||{},p=state.player,index=catalog.contentIndex||{};let html='';
 if(tab==='codex'){
  html='<h3>界域圖鑑</h3><p class="note">包含現有與擴建設計。資料存在不代表該地區或完整機制已開放；目前戰技採基礎可測效果。</p><input id="content-search" placeholder="輸入名稱搜尋，按 Enter 或移開焦點" value="'+esc(query)+'">';
  const groups=[['怪物',index.monsters],['職業',index.classes],['裝備與紀念物',index.equipment],['招式與動畫設定',index.skills]];
  for(const [title,rows] of groups){const list=Object.values(rows||{}).filter(v=>!query||(v.name||v.proposed_name||'').includes(query));html+='<h4>'+title+' · '+list.length+'</h4>';for(const v of list.slice(0,query?30:5)){const anim=v.animation;html+='<details><summary>'+esc(v.name||v.proposed_name)+'</summary><p>'+esc(v.story||v.biography||'')+'</p>';if(anim)html+='<p>預警：'+esc(anim.telegraph)+'<br>動作：'+esc(anim.body)+'<br>特效：'+esc(anim.vfx)+'</p>';if(v.mechanic)html+='<p>'+esc(v.mechanic)+'</p>';html+='</details>';}}
 }
 if(tab==='stories'){
  html='<h3>旅途中的書與故事</h3><p class="note">這一版是調查對話版本：接取→地方調查桌取證→回原人物核對並選擇。護送、水閘等專屬關卡仍以原機制接續製作，不以點擊冒充完成。</p>';
  for(const b of c.books||[])html+='<details><summary>'+esc(b.title)+(b.read?' · 已收藏':'')+'</summary><p class="book-text">'+esc(b.text)+'</p>'+button(b.read?'重讀':'閱讀收藏','read_book',{target:b.id},dist(p.position,b.position)>10)+'</details>';
  for(const s of c.stories||[]){html+='<section class="story-card"><h4>'+esc(s.title)+(s.progress.done?' · 已完成':'')+'</h4><p>'+esc(s.objective)+'</p><p class="book-text">'+esc(s.node.text||'請先找委託人交談。')+'</p>';
   if(!s.progress.done){html+=button(s.progress.accepted?'繼續／回報':'與委託人交談','story_talk',{target:s.id},dist(p.position,s.giverPosition)>8);if(s.progress.accepted&&!s.progress.evidence)html+=button('在調查桌取證','story_inspect',{target:s.id},dist(p.position,s.position)>5);for(const ch of s.node.choices||[])html+=button(ch.text,'story_choice',{target:s.id,choice:ch.id},dist(p.position,s.giverPosition)>8);}
   html+='</section>';
  }
  if(!(c.books||[]).length)html+='<p>靠近已開放聚落的藏記架與調查桌，可發現地方故事。</p>';
 }
 if(tab==='bonds'){
  html='<h3>地方的人與承諾</h3>';for(const [id,r] of Object.entries(c.relations||{}))html+='<p><strong>'+esc(r.name)+'</strong> · 好感 '+r.score+' · '+esc(r.tier)+(r.restricted?' · 等待補救':'')+'</p><p>'+esc(r.dialogue)+'</p>';
  html+='<h4>收到的紀念物</h4>';for(const id of Object.keys(c.collections||{}))html+='<p>'+esc(index.equipment?.[id]?.proposed_name||id)+'</p>';
 }
 if(tab==='npc'&&selectedNpc){const n=selectedNpc,r=c.relations?.[n.id];if(r){html='<hr><h4>你與'+esc(n.name)+'的關係</h4><p>好感 '+r.score+' · '+esc(r.tier)+(r.restricted?' · 安全限制中':'')+'</p><p>'+esc(r.dialogue)+'</p>';for(const topic of r.topics)html+=button(topic,'npc_chat',{target:n.id,topic});html+='<p class="note">聊天需要新話題與間隔。購物、委託會自然記錄；刻意攻擊會扣分，不能靠小額購物洗掉。</p>';html+=button('聽取私人委託','npc_private_accept',{target:n.id},!r.canAccept);if(r.questAccepted&&!r.questDone)html+='<p>閱讀本區地方書籍後，帶兩份月露草協助照護並回報。</p>'+button('交付照護補給 · 月露草×2','npc_private_help',{target:n.id},(p.materials.herb||0)<2);html+=button('接受紀念贈物','npc_gift',{target:n.id},!r.canGift);if(r.restricted)html+=button('完成補救 · 木材×2','npc_repair',{target:n.id},(p.materials.wood||0)<2);html+='<details><summary>敵對行為</summary><p>這會造成關係損失並觸發安全限制。</p>'+button('蓄意攻擊此NPC','npc_attack',{target:n.id},dist(p.position,n.position)>3)+'</details>';}}
 if(tab==='character')html+='<h4>職業訓練</h4>'+button('向附近公會領取本職訓練戰具（一次）','class_attune',{npc:catalog.npcs.find(n=>n.role==='guild'&&dist(p.position,n.position)<=8)?.id||''});
 if(tab==='admin')html+='<hr><h4>擴建世界版本</h4><p class="note">只切換獨立測試世界；先退出全部副本。會保留物品與職業，搬動角色及內容座標。啟用後請重新整理畫面。</p>'+button('啟用64公里世界 · 第一批五區','expansion_start',{},!!c.expanded)+(c.expanded?'<p>目前第 '+c.release+' 批；王城在第2批開放。</p>'+button('開放下一批五區','content_release',{release:c.release+1},c.release>=5):'');
 return html;
}
