import {renderCharacterPanel} from '../progression/character-panel.js';
import {renderInventoryPanel} from '../inventory/inventory-panel.js';
import {renderCraftingPanel} from '../crafting/crafting-panel.js';
import {renderQuestsPanel} from '../quests/quests-panel.js';
import {renderSkillsPanel} from '../combat/skills-panel.js';
import {renderTalentsPanel} from '../progression/talents-panel.js';
import {renderWorldPanel} from '../world/world-panel.js';
import {renderPartyPanel} from '../social/party-panel.js';
import {renderAdminPanel} from '../admin/admin-panel.js';
import {renderNpcPanel} from '../npc/npc-panel.js';
import {renderSettingsPanel} from '../settings/settings-panel.js';
const terminalPanels={'character': renderCharacterPanel,'inventory': renderInventoryPanel,'crafting': renderCraftingPanel,'quests': renderQuestsPanel,'skills': renderSkillsPanel,'talents': renderTalentsPanel,'world': renderWorldPanel,'party': renderPartyPanel,'admin': renderAdminPanel,'npc': renderNpcPanel,'settings': renderSettingsPanel};
export function renderTerminalPanel(tab,context){return terminalPanels[tab]?.(context)||'';}
