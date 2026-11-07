// Presentation only. Every mutation goes through the existing authenticated C++ command path.
let query='';
export function contentFilter(value){query=String(value||'').slice(0,80);}
export function contentPanel({tab,catalog,state,selectedNpc,esc,button,dist}){
 const c=state.content||{},p=state.player,index=catalog.contentIndex||{};let html='';
 if(tab==='codex'){
  html='<h3>界域圖鑑</h3><p class="note">世界中的生物與裝備記錄。</p><input id="content-search" placeholder="輸入名稱搜尋，按 Enter 或移開焦點" value="'+esc(query)+'">';
  const groups=[['怪物',index.monsters],['裝備與紀念物',index.equipment]];
  for(const [title,rows] of groups){const list=Object.values(rows||{}).filter(v=>!query||(v.name||v.proposed_name||'').includes(query));html+='<h4>'+title+' · '+list.length+'</h4>';for(const v of list.slice(0,query?30:5)){const anim=v.animation;html+='<details><summary>'+esc(v.name||v.proposed_name)+'</summary><p>'+esc(v.story||v.biography||'')+'</p>';if(anim)html+='<p>預警：'+esc(anim.telegraph)+'<br>動作：'+esc(anim.body)+'<br>特效：'+esc(anim.vfx)+'</p>';if(v.mechanic)html+='<p>'+esc(v.mechanic)+'</p>';html+='</details>';}}
 }
 if(tab==='stories'){
  html='<h3>旅途中的書與故事</h3><p class="note">這一版是調查對話版本：接取→地方調查桌取證→回原人物核對並選擇。護送、水閘等專屬關卡仍以原機制接續製作，不以點擊冒充完成。</p>';
  for(const b of c.books||[])html+='<details><summary>'+esc(b.title)+(b.read?' · 已收藏':'')+'</summary><p class="book-text">'+esc(b.text)+'</p>'+button(b.read?'重讀':'閱讀收藏','read_book',{target:b.id},dist(p.position,b.position)>10)+'</details>';
  for(const s of (c.stories||[]).filter(s=>s.progress.accepted)){html+='<section class="story-card"><h4>'+esc(s.title)+(s.progress.done?' · 已完成':'')+'</h4><p>'+esc(s.objective)+'</p><p class="book-text">'+esc(s.node.text||'請先找委託人交談。')+'</p>';
   if(!s.progress.done){html+=button(s.progress.accepted?'繼續／回報':'與委託人交談','story_talk',{target:s.id},dist(p.position,s.giverPosition)>8);if(s.progress.accepted&&!s.progress.evidence)html+=button('在調查桌取證','story_inspect',{target:s.id},dist(p.position,s.position)>5);for(const ch of s.node.choices||[])html+=button(ch.text,'story_choice',{target:s.id,choice:ch.id},dist(p.position,s.giverPosition)>8);}
   html+='</section>';
  }
  if(!(c.books||[]).length)html+='<p>靠近已開放聚落的藏記架與調查桌，可發現地方故事。</p>';
 }
 if(tab==='npc'&&selectedNpc){const n=selectedNpc,r=c.relations?.[n.id];for(const s of (c.stories||[]).filter(s=>s.giver===n.id&&!s.progress.done))html+='<p>'+esc(s.title)+'</p>'+button('聽取地方故事','story_talk',{target:s.id});if(r){html+='<hr><p>'+esc(r.dialogue)+'</p>';for(const topic of r.topics)html+=button(topic,'npc_chat',{target:n.id,topic});if(r.canAccept)html+=button('聽聽對方的請求','npc_private_accept',{target:n.id});if(r.questAccepted&&!r.questDone)html+='<p>閱讀本區地方書籍後，帶兩份月露草協助照護並回報。</p>'+button('交付照護補給','npc_private_help',{target:n.id},(p.materials.herb||0)<2);if(r.canGift)html+=button('接受贈物','npc_gift',{target:n.id});if(r.restricted)html+=button('補救 · 木材×2','npc_repair',{target:n.id},(p.materials.wood||0)<2);}}
 if(tab==='character')html+='<h4>職業訓練</h4>'+button('向附近公會領取本職訓練戰具（一次）','class_attune',{npc:catalog.npcs.find(n=>n.role==='guild'&&dist(p.position,n.position)<=8)?.id||''});
 return html;
}
