// Pure HTML renderer; current state and helpers are supplied by the client.
export function renderPartyPanel({state, esc, button, row}) {
 const p=state.player;let html="";
html='<h3>一起穿越界域</h3><p class="note">隊伍上限5人。隊友在入口8公尺內、符合等級時，可一起進入同一個副本；世界王依有效貢獻共享裝備。</p>';if(state.party){html+='<span class="badge">'+esc(state.party.id)+'</span>'+button('離開隊伍','party_leave');for(let id of state.party.members){let other=state.online.find(x=>x.id===id);html+=row('',other?.name||id,id===state.party.leader?'隊長':'隊友','',false);}}else html+=button('建立隊伍','party_create');for(let inv of state.invitations)html+=row('','收到隊伍邀請',inv.leader,button('加入','party_join',{target:inv.id}),false);html+='<h4>在線旅人</h4>';for(let other of state.online.filter(x=>x.id!==p.id))html+=row('',other.name,'Lv.'+other.level+' · '+other.id,button('邀請','party_invite',{target:other.id},!state.party||state.party.leader!==p.id),false);if(state.online.length<2)html+='<p class="note">目前只有你在線。另一個視窗可在登入畫面建立另一名測試角色，以測試同步、聊天與組隊。</p>';
 return html;
}
