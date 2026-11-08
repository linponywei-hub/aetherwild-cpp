// Consume real native snapshots to catch lost dependencies in extracted panels.
import fs from 'node:fs';
import assert from 'node:assert/strict';
import {renderTerminalPanel} from '../web/modules/ui/terminal-panels.js';
const {catalog,state}=JSON.parse(fs.readFileSync(process.argv[2],'utf8'));
globalThis.localStorage={getItem:()=>null};
const esc=value=>String(value??'').replaceAll('<','&lt;');
const context={state,catalog,selectedNpc:catalog.npcs[0],classIndex:Object.fromEntries(catalog.classes.map(c=>[c.id,c])),esc,button:label=>'<button>'+esc(label)+'</button>',row:(_id,title)=>'<p>'+esc(title)+'</p>',picture:()=>'',name:id=>String(id),cost:()=>'',filter:'',role:()=>state.role==='owner',equipmentShowcase:()=>'<div>equipment</div>',forgePanel:()=>'<div>forge</div>',talentPanel:()=>'<div>talents</div>',slotNames:['武器','胸甲','頭盔','長靴','護符'],rarities:['一般','稀有','史詩'],statLine:()=>'',CONTROL_NAMES:{attack:'攻擊'},bindingAction:null,keyLabel:key=>String(key),controls:{bindings:{attack:'Mouse0'}},backupStatus:'測試草稿',storagePersistent:false};
let checks=0;
for(const tab of ['character','inventory','crafting','quests','skills','talents','world','party','admin','npc','settings']){assert.ok(renderTerminalPanel(tab,context).length,tab);checks++;}
for(const role of ['guild','merchant','smith','healer','cartographer']){context.selectedNpc=catalog.npcs.find(n=>n.role===role);assert.ok(context.selectedNpc);assert.ok(renderTerminalPanel('npc',context).includes(context.selectedNpc.name));checks++;}
context.selectedNpc=null;assert.equal(renderTerminalPanel('npc',context),'');checks++;
state.party={id:'party',members:[state.player.id,'other'],leader:state.player.id};state.online.push({id:'other',name:'第二位旅人',level:1});state.invitations.push({id:'invitation',leader:'other'});assert.ok(renderTerminalPanel('party',context).includes('第二位旅人'));checks++;
state.role='player';assert.ok(renderTerminalPanel('admin',context).includes('需要管理員權限'));checks++;
assert.equal(renderTerminalPanel('unknown',context),'');checks++;
console.log('PANEL PROJECTION OK:',checks,'checks with native catalog and snapshot');
