#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <map>
#include <optional>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>
namespace aw {
    using Id = std::string;
    // Stable catalogue IDs; runtime entity and instance IDs are separate.
    using Quantity = std::int64_t;
    using Materials = std::map<Id, Quantity>;
    struct RuleError : std::runtime_error {
        using std::runtime_error::runtime_error;
    };
    void require(bool condition, const std::string& reason);
    struct Vec3 {
        double x {},
        y {},
        z {};
    };
    double distance(Vec3 a, Vec3 b);
    bool finite(Vec3 v);
    enum class WeaponKind {None, Sword, Staff, Bow, ShieldBlade, Daggers, Gauntlets, Instrument, Totem, Cannon, Toolgun, Chakram, Hammer, Spear, Focus, Greatsword};
    enum class Slot {Weapon, Armor, Helm, Boots, Charm};
    enum class Rarity {Common, Rare, Epic, Excellent, Legendary, Mythic, Immortal};
    struct Stats {
        double hp {},
        mp {},
        attack {},
        defense {},
        crit {},
        speed {};
        Stats& operator+=(const Stats&);
    };
    struct Vitals {
        double hp {100},
        maxHp {100},
        mp {60},
        maxMp {60},
        stamina {100},
        maxStamina {100};
        bool alive() const {
            return hp > 0;
        }
        double receiveDamage(double amount);
        // Returns effective damage; no rewards or UI here.
        void restore(double health, double mana);
    };
    struct ItemDefinition {
        Id id;
        std::string name;
        Slot slot {};
        WeaponKind weapon {};
        Stats stats;
        int requiredLevel {1};
        Id set;
    };
    struct ItemInstance {
        Id id, definition;
        Rarity rarity {Rarity::Common};
        int requiredLevel {1},
        upgrade {};
    };
    struct ClassDefinition {
        Id id;
        std::string name;
        Stats base;
        double range {5};
        int level {1};
        Id parent, secret;
        Materials cost;
        WeaponKind weapon {WeaponKind::Sword};
    };
    struct RegionDefinition {
        Id id;
        std::string name;
        Vec3 center;
        double radius {};
        int level {};
        bool city {},
        wilderness {};
    };
    struct ResourceDefinition {
        Id id;
        std::string name, family, tool;
        int tier {},
        proficiency {},
        xp {};
        double seconds {};
    };
    struct ToolDefinition {
        Id id;
        std::string name, kind;
        int tier {};
        Materials cost;
    };
    struct Recipe {
        Id id, output;
        int tier {1};
        Materials cost;
        std::string station;
        bool equipment {};
        int count {1};
    };
    struct SkillDefinition {
        Id id;
        std::string name, effect;
        double mana {},
        cooldown {},
        damage {},
        heal {},
        range {},
        radius {},
        duration {},
        shield {};
    };
    struct DungeonDefinition {
        Id id, region;
        std::string name, objective, entrance;
        int level {1},
        guards {};
        Id skill, secret;
        Vec3 entrancePosition;
    };
    struct QuestDefinition {
        Id id;
        std::string name, type;
        Id target, npc;
        int level {1},
        count {1};
        Quantity coins {},
        xp {};
    };
    struct BossDefinition {
        Id id, region;
        std::string name;
        Vec3 position;
        int level {1};
        double hp {},
        attack {};
    };
    struct SetDefinition {
        Id id;
        Stats two, four;
    };
    struct NpcDefinition {
        Id id, region;
        std::string name, role;
        Vec3 position;
    };
    struct Catalog {
        std::map<Id, ItemDefinition> items;
        std::map<Id, ClassDefinition> classes;
        std::map<Id, RegionDefinition> regions;
        std::map<Id, ResourceDefinition> resources;
        std::map<Id, ToolDefinition> tools;
        std::map<Id, Recipe> recipes;
        std::map<Id, SkillDefinition> skills;
        std::map<Id, DungeonDefinition> dungeons;
        std::map<Id, QuestDefinition> quests;
        std::map<Id, BossDefinition> bosses;
        std::map<Id, SetDefinition> sets;
        std::map<Id, NpcDefinition> npcs;
        std::map<Id,double> arrowBonus;
        Materials prices;
        double worldLimit {1800};
        int contentRelease {5};
        std::map<Id,int> regionReleases;
        std::map<Id,std::string> classRoles;
        std::map<Id,std::vector<Id>> classSkills;
        std::map<Id,Id> skillOwners, starterItems;
        void validate() const;
    };
    Catalog makeCatalog();
    struct QuestProgress {
        int count {};
        bool claimed {};
    };
    struct Player {
        Id id, accountId;
        std::string name;
        Id profession {
            "warrior"
        };
        int level {1};
        Quantity xp {},
        coins {},
        bankCoins {};
        Vitals vitals;
        Vec3 position {0,0,24};
        Id scene {
            "world"
        },
        lastCity {
            "windchime"
        };
        Materials materials, warehouse;
        std::map<Id,int> tools;
        std::vector<ItemInstance> inventory;
        std::map<Slot,Id> equipped;
        std::set<Id> jobs {
            "warrior"
        },
        secrets, learnedSkills, unlockedMaps;
        std::map<Id,QuestProgress> quests;
        std::set<Id> processedRewards;
        Quantity gatherXp {};
        double attackReady {},
        regenReady {},
        shield {},
        shieldUntil {},
        staminaUpdatedAt {};
        bool sprintLocked {};
        double arrowHasteUntil {};
        std::map<Id,double> skillReady;
        void attackIntent() const;
        // Entity-level precondition; service resolves range, equipment, and damage.
        double receiveDamage(double amount) {
            return vitals.receiveDamage(amount);
        }
    };
    struct Monster {
        Id id, definition, region, scene {
            "world"
        };
        Vec3 position;
        int level {1};
        Vitals vitals;
        double attack {},
        defense {},
        nextAttack {},
        frozenUntil {};
        bool boss {},
        rewardIssued {};
        Id castSkill;
        Vec3 castPoint;
        double castResolve {}, abilityReady {};
        Id tauntedBy;
        double tauntUntil {};
        std::map<Id,double> contribution;
        double receiveDamage(double amount) {
            return vitals.receiveDamage(amount);
        }
    };
    struct ResourceNode {
        Id id, definition;
        Vec3 position;
        Id scene {
            "world"
        };
        double readyAt {};
        std::optional<Id> reservedBy;
        double completeAt {};
    };
    struct Reward {
        Id id, player;
        Quantity coins {},
        xp {};
        Materials materials;
        std::vector<ItemInstance> items;
    };
    struct DungeonRun {
        Id id, definition, player;
        int guardsRemaining {},
        objectives {};
        std::set<Id> interacted;
        std::set<Id> members, chestClaimed;
        std::map<Id,Vec3> returnPositions;
        bool bossDead {},
        claimed {};
        Id returnScene;
        Vec3 returnPosition;
    };
    struct Event {
        std::string type;
        Id actor, target;
        double value {};
        std::string text;
    };
    struct World {
        std::map<Id,Player> players;
        std::map<Id,Monster> monsters;
        std::map<Id,ResourceNode> resources;
        std::map<Id,DungeonRun> runs;
        std::map<Id,Reward> rewards;
        std::vector<Event> events;
        double now {};
        std::uint64_t sequence {};
        Id nextId(const std::string& prefix);
    };
    struct AttackResult {
        double damage {};
        bool killed {};
    };
}

