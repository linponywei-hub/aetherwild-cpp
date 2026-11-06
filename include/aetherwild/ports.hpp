#pragma once
#include "domain.hpp"
#include <functional>
#include <memory>
#include <span>
#include <string_view>
#include <cstddef>
namespace aw::ports {
    // Pure contracts for later adapters. No implementation pretends to authenticate or network.
    struct VerifiedAccount {
        Id account;
        std::set<std::string> permissions;
    };
    struct CommandEnvelope {
        Id request, session;
        std::uint64_t sequence {},
        expectedRevision {};
        std::string type, payload;
    };
    struct Snapshot {
        std::uint64_t tick {},
        revision {};
        std::vector<Event> events;
    };
    struct IAuthentication {
        virtual ~IAuthentication()=default;
        virtual std::optional<VerifiedAccount> verify(std::string_view bearer)=0;
    };
    struct ITransport {
        virtual ~ITransport()=default;
        virtual std::vector<CommandEnvelope> receive()=0;
        virtual void send(const Id& verifiedSession,const Snapshot&)=0;
    };
    struct ICharacterRepository {
        virtual ~ICharacterRepository()=default;
        virtual std::optional<Player> load(const Id& account,const Id& character)=0;
        // Transaction must atomically persist character, reward idempotency, and pending reward ACK.
        virtual bool compareAndSwap(const Player&,std::uint64_t expectedRevision,std::span<const Id> acknowledgedRewards)=0;
    };
    struct GroundContact {
        Vec3 feet;
        Id surface, layer;
        bool grounded {};
    };
    struct ICollisionWorld {
        virtual ~ICollisionWorld()=default;
        virtual GroundContact sweepCapsule(Vec3 from,Vec3 desired,double radius,double height)=0;
        virtual bool lineOfSight(Vec3 from,Vec3 to)=0;
    };
    struct IRenderer {
        virtual ~IRenderer()=default;
        virtual void display(const Snapshot&)=0;
    };
    struct IAssetStore {
        virtual ~IAssetStore()=default;
        virtual Id uploadValidatedModel(std::span<const std::byte> glb)=0;
    };
}
