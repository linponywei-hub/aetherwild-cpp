#pragma once
#include "domain.hpp"
#include <string_view>
namespace aw {
    class ProgressionSystem {
        public: static Quantity levelXp(int level);
        static void grantXp(Player&,Quantity);
        static void refresh(Player&,const Catalog&);
        static void changeClass(Player&,const Id&,const Id& npc,const Catalog&);
    };
    class InventorySystem {
        public: static bool canPay(const Materials&,const Materials&);
        static void pay(Materials&,const Materials&);
        static void add(Materials&,const Id&,Quantity);
        static ItemInstance create(World&,const Catalog&,const Id&,Rarity=Rarity::Common,int required=0);
        static Stats bonuses(const Player&,const Catalog&);
        static void equip(Player&,const Id&,const Catalog&);
        static void unequip(Player&,const Id& instance,const Catalog&);
        static void usePotion(Player&,const Id&,const Catalog&);
        static void storeItem(Player&,const Id& instance,const Id& npc,const Catalog&,bool withdraw=false);
        static void store(Player&,const Id&,Quantity,const Id& npc,const Catalog&,bool withdraw=false);
    };
    class QuestSystem {
        public: static void accept(Player&,const Id&,const Id& npc,const Catalog&);
        static void record(Player&,const std::string& type,const Id& target,const Catalog&);
        static void claim(Player&,const Id&,const Id& npc,const Catalog&);
    };
    class RewardSystem {
        public: static bool claim(World&,Player&,const Id&,const Catalog&);
        static void onDeath(World&,Monster&,const Catalog&);
    };
    class CombatSystem {
        public: static Monster spawn(World&,const Id&,Vec3,int level,const Id& region,bool boss=false,const Id& definition="");
        static AttackResult attack(World&,const Catalog&,const Id& player,const Id& monster,const Id& arrow="woodarrow");
        static double monsterAttack(World&,const Catalog&,const Id& monster,const Id& player);
        static void cast(World&,const Catalog&,const Id& player,const Id& skill,const Id& target="");
        static void weaponSkill(World&,const Catalog&,const Id& player,const Id& target="");
        static void respawn(Player&,const Catalog&);
    };
    class GatheringSystem {
        public: static void begin(World&,Player&,const Id& node,const Catalog&);
        static void finish(World&,Player&,const Id& node,const Catalog&);
        static void cancel(World&,Player&,const Id& node);
    };
    class CraftingSystem {
        public: static Id requiredRegion(const Recipe&,const Player&,const Catalog&);
        static void craft(World&,Player&,const Id& recipe,const Id& npc,const Catalog&);
    };
    class DungeonSystem {
        public: static Id enter(World&,Player&,const Id& dungeon,const Catalog&);
        static void interactObjective(World&,Player&,const Id& objective,const Catalog&);
        static void openChest(World&,Player&,const Catalog&);
        static void leave(World&,Player&);
    };
    class EconomySystem {
        public: static void buy(World&,Player&,const Id& code,Quantity count,const Id& npc,const Catalog&);
        static void transfer(Player& from,Player& to,Quantity coins);
    };
    class MovementSystem {
        public: static bool safe(Vec3,const Id& scene,const Catalog&);
        static double landingDamage(double drop,double maxHp,double airTime);
        static double land(Player&,double drop,double airTime);
        static void move(Player&,Vec3 destination,double dt,bool sprint,const Catalog&,double now);
        static void tickStamina(Player&,double dt,bool sprint,double now);
    };
    // Validates authoritative service proximity; never trust client-supplied region or NPC role.
    const NpcDefinition& nearbyNpc(const Player&,const Id&,std::string_view role,const Catalog&);
    class Simulation {
        Catalog catalog_;
        World world_;
        public: Simulation();
        const Catalog& catalog() const {
            return catalog_;
        }
        World& world() {
            return world_;
        }
        const World& world()const {
            return world_;
        }
        Player& addPlayer(const Id&,const std::string&,const Id& profession="warrior");
        void advance(double dt);
    };
}

