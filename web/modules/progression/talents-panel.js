// Pure HTML renderer; current state and helpers are supplied by the client.
export function renderTalentsPanel({state, talentPanel}) {
 const p=state.player;let html="";
html=talentPanel(p);
 return html;
}
