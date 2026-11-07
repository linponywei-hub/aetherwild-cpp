export const DEFAULT_BINDINGS=Object.freeze({forward:'KeyW',back:'KeyS',left:'KeyA',right:'KeyD',sprint:'ShiftLeft',jump:'Space',interact:'KeyE',weapon:'KeyQ',skill:'KeyF',potion:'KeyR',terminal:'KeyB',settings:'KeyP',chat:'KeyC'});
export const CONTROL_NAMES={forward:'前進',back:'後退',left:'向左',right:'向右',sprint:'奔跑',jump:'跳躍',interact:'互動',weapon:'武器技',skill:'副本技能',potion:'喝藥',terminal:'旅人終端',settings:'設定',chat:'聊天'};
export function keyLabel(code){return ({Space:'空白鍵',ShiftLeft:'左 Shift',ShiftRight:'右 Shift',ControlLeft:'左 Ctrl',ControlRight:'右 Ctrl',ArrowUp:'↑',ArrowDown:'↓',ArrowLeft:'←',ArrowRight:'→'})[code]||code.replace(/^Key|^Digit/,'');}
const valid=code=>typeof code==='string'&&/^(Key[A-Z]|Digit[0-9]|Arrow(Up|Down|Left|Right)|Space|Shift(Left|Right)|Control(Left|Right))$/.test(code);
export class ControlBindings {
 constructor(storage){this.storage=storage;this.bindings={...DEFAULT_BINDINGS};try{const saved=JSON.parse(storage?.getItem('aw-keybindings')||'null');if(saved?.version===1){const merged={...DEFAULT_BINDINGS};for(const key of Object.keys(merged))if(valid(saved.bindings?.[key]))merged[key]=saved.bindings[key];if(new Set(Object.values(merged)).size===Object.keys(merged).length)this.bindings=merged;}}catch{}}
 action(code){const direct=Object.keys(this.bindings).find(key=>this.bindings[key]===code);if(direct)return direct;const alias={ArrowUp:'forward',ArrowDown:'back',ArrowLeft:'left',ArrowRight:'right',ShiftRight:'sprint',KeyV:'terminal'}[code];return alias&&this.bindings[alias]===DEFAULT_BINDINGS[alias]?alias:null;}
 bind(action,code){if(!Object.hasOwn(DEFAULT_BINDINGS,action)||!valid(code))throw Error('請使用字母、數字、方向鍵、空白鍵、Shift 或 Ctrl');const conflict=Object.keys(this.bindings).find(key=>key!==action&&this.bindings[key]===code);if(conflict)throw Error('此按鍵已用於「'+CONTROL_NAMES[conflict]+'」');this.bindings[action]=code;this.save();}
 reset(){this.bindings={...DEFAULT_BINDINGS};this.save();}
 save(){this.storage?.setItem('aw-keybindings',JSON.stringify({version:1,bindings:this.bindings}));}
}
