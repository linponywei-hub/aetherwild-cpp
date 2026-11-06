-- NEW DATABASE DESIGN ONLY. Not wired into the current runtime.
-- Never run against the original production database. SQLite-compatible baseline.
PRAGMA foreign_keys = ON;
CREATE TABLE accounts (
  id TEXT PRIMARY KEY, provider_subject TEXT UNIQUE,
  role TEXT NOT NULL DEFAULT 'player' CHECK(role IN ('player','admin','owner'))
);
CREATE TABLE characters (
  id TEXT PRIMARY KEY, account_id TEXT NOT NULL REFERENCES accounts(id), name TEXT NOT NULL,
  profession TEXT NOT NULL, level INTEGER NOT NULL CHECK(level BETWEEN 1 AND 100),
  xp INTEGER NOT NULL CHECK(xp>=0), hp REAL NOT NULL CHECK(hp>=0), mp REAL NOT NULL CHECK(mp>=0),
  stamina REAL NOT NULL CHECK(stamina>=0), x REAL NOT NULL, y REAL NOT NULL, z REAL NOT NULL,
  scene_id TEXT NOT NULL, last_city TEXT NOT NULL,
  gather_xp INTEGER NOT NULL DEFAULT 0 CHECK(gather_xp>=0), revision INTEGER NOT NULL DEFAULT 0 CHECK(revision>=0)
);
CREATE TABLE wallets (
  character_id TEXT PRIMARY KEY REFERENCES characters(id),
  coins INTEGER NOT NULL DEFAULT 0 CHECK(coins>=0), bank_coins INTEGER NOT NULL DEFAULT 0 CHECK(bank_coins>=0)
);
CREATE TABLE item_definitions (
  id TEXT PRIMARY KEY, name TEXT NOT NULL, slot TEXT NOT NULL,
  weapon_kind TEXT NOT NULL, required_level INTEGER NOT NULL CHECK(required_level>=1),
  stats_json TEXT NOT NULL, set_id TEXT
);
CREATE TABLE item_instances (
  id TEXT PRIMARY KEY, definition_id TEXT NOT NULL REFERENCES item_definitions(id),
  owner_id TEXT NOT NULL REFERENCES characters(id), rarity INTEGER NOT NULL CHECK(rarity BETWEEN 0 AND 6),
  required_level INTEGER NOT NULL CHECK(required_level BETWEEN 1 AND 100),
  upgrade INTEGER NOT NULL DEFAULT 0 CHECK(upgrade BETWEEN 0 AND 20), location TEXT NOT NULL CHECK(location IN ('bag','warehouse')),
  UNIQUE(owner_id,id)
);
CREATE TABLE equipment (
  character_id TEXT NOT NULL REFERENCES characters(id),
  slot TEXT NOT NULL CHECK(slot IN ('weapon','armor','helm','boots','charm')),
  instance_id TEXT NOT NULL UNIQUE,
  PRIMARY KEY(character_id,slot),
  FOREIGN KEY(character_id,instance_id) REFERENCES item_instances(owner_id,id)
);
CREATE TABLE material_stacks (
  character_id TEXT NOT NULL REFERENCES characters(id), code TEXT NOT NULL,
  location TEXT NOT NULL CHECK(location IN ('bag','warehouse')), quantity INTEGER NOT NULL CHECK(quantity>=0),
  PRIMARY KEY(character_id,code,location)
);
CREATE TABLE character_unlocks (
  character_id TEXT NOT NULL REFERENCES characters(id),
  kind TEXT NOT NULL CHECK(kind IN ('job','secret','skill','map')), code TEXT NOT NULL,
  PRIMARY KEY(character_id,kind,code)
);
CREATE TABLE character_tools (
  character_id TEXT NOT NULL REFERENCES characters(id), kind TEXT NOT NULL,
  tier INTEGER NOT NULL CHECK(tier BETWEEN 1 AND 5), PRIMARY KEY(character_id,kind)
);
CREATE TABLE quest_progress (
  character_id TEXT NOT NULL REFERENCES characters(id), quest_id TEXT NOT NULL,
  count INTEGER NOT NULL CHECK(count>=0), claimed INTEGER NOT NULL DEFAULT 0 CHECK(claimed IN (0,1)),
  PRIMARY KEY(character_id,quest_id)
);
CREATE TABLE encounters (
  id TEXT PRIMARY KEY, definition_id TEXT NOT NULL, hp REAL NOT NULL CHECK(hp>=0),
  state TEXT NOT NULL, revision INTEGER NOT NULL DEFAULT 0, reward_plan_json TEXT
);
CREATE TABLE contributions (
  encounter_id TEXT NOT NULL REFERENCES encounters(id), character_id TEXT NOT NULL REFERENCES characters(id),
  damage REAL NOT NULL CHECK(damage>=0), PRIMARY KEY(encounter_id,character_id)
);
CREATE TABLE reward_events (
  id TEXT PRIMARY KEY, character_id TEXT NOT NULL REFERENCES characters(id),
  encounter_id TEXT REFERENCES encounters(id), payload_json TEXT NOT NULL, created_at INTEGER NOT NULL,
  UNIQUE(character_id,id)
);
CREATE TABLE reward_claims (
  character_id TEXT NOT NULL REFERENCES characters(id), event_id TEXT NOT NULL UNIQUE,
  claimed_at INTEGER NOT NULL, PRIMARY KEY(character_id,event_id),
  FOREIGN KEY(character_id,event_id) REFERENCES reward_events(character_id,id)
);
CREATE TABLE resource_nodes (
  id TEXT PRIMARY KEY, definition_id TEXT NOT NULL, scene_id TEXT NOT NULL,
  x REAL NOT NULL,y REAL NOT NULL,z REAL NOT NULL,ready_at REAL NOT NULL,
  reserved_by TEXT REFERENCES characters(id),complete_at REAL,revision INTEGER NOT NULL DEFAULT 0
);
CREATE TABLE dungeon_runs (
  id TEXT PRIMARY KEY, definition_id TEXT NOT NULL, guards_remaining INTEGER NOT NULL CHECK(guards_remaining>=0),
  objectives_json TEXT NOT NULL, state TEXT NOT NULL, revision INTEGER NOT NULL DEFAULT 0
);
CREATE TABLE dungeon_members (
  run_id TEXT NOT NULL REFERENCES dungeon_runs(id), character_id TEXT NOT NULL REFERENCES characters(id),
  return_json TEXT NOT NULL, PRIMARY KEY(run_id,character_id)
);
CREATE TABLE parties (id TEXT PRIMARY KEY, leader_id TEXT NOT NULL REFERENCES characters(id));
CREATE TABLE party_members (
  party_id TEXT NOT NULL REFERENCES parties(id), character_id TEXT NOT NULL UNIQUE REFERENCES characters(id),
  PRIMARY KEY(party_id,character_id)
);
CREATE TABLE command_receipts (
  session_id TEXT NOT NULL, request_id TEXT NOT NULL, sequence INTEGER NOT NULL,
  result_json TEXT NOT NULL, PRIMARY KEY(session_id,request_id), UNIQUE(session_id,sequence)
);
CREATE TABLE outbox (
  id TEXT PRIMARY KEY, event_type TEXT NOT NULL, payload_json TEXT NOT NULL,
  created_at INTEGER NOT NULL, delivered_at INTEGER
);
CREATE TABLE admin_audit (
  id TEXT PRIMARY KEY, actor_account_id TEXT NOT NULL REFERENCES accounts(id),
  operation TEXT NOT NULL, target_id TEXT, detail_json TEXT NOT NULL, created_at INTEGER NOT NULL
);
-- Service transactions must additionally validate slot/definition match, party size,
-- permission changes, integer overflow, legal revisions, and reward ownership.
