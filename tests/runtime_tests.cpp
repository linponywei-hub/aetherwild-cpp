#include "aetherwild/runtime.hpp"
#include <filesystem>
#include <iostream>
using namespace aw;
namespace {
    int checks=0,groups=0;
    void check(bool c,const char*e) {
        checks++;
        if(!c)throw std::runtime_error(e);
    }
    template<class F>void no(F f) {
        checks++;
        try {
            f();
        }
        catch(const RuleError&) {
            return;
        }
        throw std::runtime_error("expected refusal");
    }
    template<class F>void test(const char*s,F f) {
        f();
        groups++;
        std::cout<<"PASS "<<s<<'\n';
    }
    Json create(GameRuntime&g,const char*name="QA") {
        return g.createSession(Json {
            {
                "name",name
            },
            {
                "profession","warrior"
            }
        });
    }
    Json cmd(GameRuntime&g,const Json&session,const char*type,const Id&target="",const Id&request="") {
        static int n=0;
        return g.command(session.str("token"),Json {
            {
                "type",type
            },
            {
                "target",target
            },
            {
                "request",request.empty()?"req_"+std::to_string(++n):request
            }
        });
    }
    void time(GameRuntime&g,double n) {
        for(int i=0;i<static_cast<int>(n*20);i++)g.tick(.05);
    }
}
#define CHECK(x) check(bool(x),#x)
int main() {
    try {
        test("JSON strict parser Unicode and int64",[] {
            auto j=Json::parse("{\"name\":\"\\u754c\\u57df\\ud83d\\ude00\",\"big\":9223372036854775807,\"list\":[true,null,1.25]}");
            CHECK(j.str("name")=="界域😀");
            CHECK(j.at("big").integer()==INT64_MAX);
            CHECK(Json::parse(j.dump()).dump()==j.dump());
            no([] {
                Json::parse("{\"a\":1,\"a\":2}");
            });
            no([] {
                Json::parse("[01]");
            });
            no([] {
                Json::parse("1e9999");
            });
            no([] {
                Json::parse("\"\\ud800\"");
            });
            no([] {
                Json::parse("{\"a\":true}junk");
            });
        });
        test("layered ground and city safety",[] {
            auto c=makeCatalog();
            auto&r=c.regions.at("astral");
            auto top=cityArrival(r),under=r.center;
            CHECK(top.y==62);
            CHECK(onCityFloor(top,r));
            CHECK(!onCityFloor(under,r));
            CHECK(!MovementSystem::safe(under,"world",c));
            CHECK(groundBelow(under,c)==0);
            CHECK(groundBelow(top,c)==62);
            auto&t=c.regions.at("verdant");
            auto pos=t.center;
            pos.z+=90;
            pos.y=17;
            CHECK(std::abs(groundBelow(pos,c)-16)<.001);
        });
        test("verified actor ownership and permissions",[] {
            GameRuntime g;
            auto p=create(g,"owner"),q=create(g,"visitor");
            CHECK(g.snapshot(p.str("token")).str("role")=="owner");
            CHECK(g.snapshot(q.str("token")).str("role")=="player");
            no([&] {
                cmd(g,q,"admin_supply");
            });
            CHECK(g.snapshot(q.str("token")).at("player").num("level")==1);
            cmd(g,p,"admin_grant",q.at("player").str("id"));
            CHECK(g.snapshot(q.str("token")).str("role")=="admin");
            no([&] {
                g.snapshot("invalid");
            });
        });
        test("idempotent commands and rollback",[] {
            GameRuntime g;
            auto p=create(g);
            auto first=cmd(g,p,"buy","potion","once");
            auto coins=first.at("player").num("coins");
            CHECK(coins==40);
            auto repeated=cmd(g,p,"buy","potion","once");
            CHECK(repeated.at("player").num("coins")==coins);
            CHECK(repeated.at("player").at("materials").num("potion")==4);
            no([&] {
                cmd(g,p,"craft","gear_cindergreatsword");
            });
            CHECK(g.snapshot(p.str("token")).at("player").num("coins")==coins);
        });
        test("authoritative normalized movement building collision",[] {
            GameRuntime g;
            auto p=create(g);
            auto key=p.str("token"),id=p.at("player").str("id");
            auto before=g.world().players.at(id).position;
            g.input(key,Json {
                {
                    "sequence",1
                },
                {
                    "x",1
                },
                {
                    "z",1
                },
                {
                    "yaw",0
                }
            });
            time(g,.2);
            auto pos=g.world().players.at(id).position;
            CHECK(distance(before,pos)<=1.01);
            CHECK(distance(before,pos)>.7);
            no([&] {
                g.input(key,Json {
                    {
                        "sequence",2
                    },
                    {
                        "x",20
                    },
                    {
                        "z",0
                    },
                    {
                        "yaw",0
                    }
                });
            });
            auto b=buildings(g.definitions()).front();
            CHECK(blocked( {b.position.x,b.position.y,b.position.z},
            buildings(g.definitions())));
        });
        test("craft forging location and material atomicity",[] {
            GameRuntime g;
            auto p=create(g);
            cmd(g,p,"admin_supply");
            auto key=p.str("token"),id=p.at("player").str("id");
            auto before=g.world().players.at(id).materials;
            no([&] {
                cmd(g,p,"craft","gear_cindergreatsword");
            });
            CHECK(g.world().players.at(id).materials==before);
            cmd(g,p,"admin_travel","cinder");
            auto result=cmd(g,p,"craft","gear_cindergreatsword");
            CHECK(result.at("player").at("inventory").arrayItems().size()==2);
        });
        test("shared dungeon progression chest ownership and restart",[] {
            auto folder=std::filesystem::temp_directory_path()/"aetherwild-runtime-unit";
            std::filesystem::create_directories(folder);
            auto path=folder/"world.json";
            std::filesystem::remove(path);
            std::filesystem::remove(path.string()+".bak");
            GameRuntime g(path);
            auto p=create(g,"leader"),q=create(g,"member");
            auto pid=p.at("player").str("id"),qid=q.at("player").str("id");
            cmd(g,p,"admin_supply");
            auto party=cmd(g,p,"party_create").at("party").str("id");
            cmd(g,p,"party_invite",qid);
            cmd(g,q,"party_join",party);
            cmd(g,p,"admin_entrance","ruins");
            g.world().players.at(qid).position=g.world().players.at(pid).position;
            auto entered=cmd(g,p,"enter","ruins");
            auto run=entered.at("player").str("scene");
            CHECK(g.world().players.at(qid).scene==run);
            CHECK(g.world().runs.at(run).members.size()==2);
            std::vector<Id> guards;
            Id boss;
            for(auto&[id,m]:g.world().monsters)if(m.scene==run) {
                if(m.definition=="dungeon_boss")boss=id;
                else guards.push_back(id);
            }
            auto aim=[&](const Id&id) {
                auto&m=g.world().monsters.at(id);
                g.world().players.at(pid).position= {m.position.x,0,m.position.z+2};
            };
            aim(boss);
            no([&] {
                cmd(g,p,"attack",boss);
            });
            for(auto&id:guards) {
                aim(id);
                cmd(g,p,"attack",id);
                time(g,.8);
            }
            CHECK(g.world().runs.at(run).guardsRemaining==0);
            Vec3 points[]= {
                {-12,0,-12},
                {12,0,-12},
                {0,0,-4}
            };
            for(int i=0;i<3;i++) {
                g.world().players.at(pid).position=points[i];
                cmd(g,p,"objective","objective_"+std::to_string(i+1));
            }
            while(g.world().monsters.at(boss).vitals.alive()) {
                aim(boss);
                cmd(g,p,"attack",boss);
                time(g,.8);
            }
            g.world().players.at(pid).position= {0,0,-25};
            g.world().players.at(qid).position= {0,0,-25};
            cmd(g,p,"chest");
            cmd(g,q,"chest");
            CHECK(g.world().runs.at(run).chestClaimed.size()==2);
            CHECK(g.world().players.at(qid).learnedSkills.contains("wind"));
            auto gold=g.world().players.at(pid).coins;
            g.save();
            GameRuntime restarted(path);
            CHECK(restarted.world().players.at(pid).coins==gold);
            CHECK(restarted.world().runs.at(run).members.size()==2);
            no([&] {
                cmd(restarted,p,"chest");
            });
            CHECK(restarted.world().players.at(pid).coins==gold);
            std::filesystem::remove_all(folder);
        });
        test("lift call ride destination remains grounded",[] {
            GameRuntime g;
            auto p=create(g);
            auto id=p.at("player").str("id"),key=p.str("token");
            cmd(g,p,"admin_travel","astral");
            auto&player=g.world().players.at(id);
            player.position.z+=110;
            cmd(g,p,"lift");
            time(g,6.3);
            CHECK(std::abs(player.position.y-62)<.01);
            cmd(g,p,"lift");
            time(g,6.3);
            CHECK(std::abs(player.position.y)<.01);
            CHECK(player.vitals.alive());
            cmd(g,p,"lift");
            time(g,6.3);
            CHECK(std::abs(player.position.y-62)<.01);
            time(g,.6);
            CHECK(std::abs(player.position.y-62)<.01);
        });
        test("restart during lift keeps destination and passenger",[] {
            auto folder=std::filesystem::temp_directory_path()/"aw-lift-save-unit";
            std::filesystem::remove_all(folder);
            GameRuntime g(folder/"world.json");
            auto p=create(g);
            auto id=p.at("player").str("id");
            cmd(g,p,"admin_travel","astral");
            g.world().players.at(id).position.z+=110;
            cmd(g,p,"lift");
            time(g,6.3);
            cmd(g,p,"lift");
            time(g,2);
            g.save();
            GameRuntime restored(folder/"world.json");
            restored.input(p.str("token"),Json {
                {
                    "sequence",1
                },
                {
                    "x",0
                },
                {
                    "z",0
                },
                {
                    "yaw",0
                }
            });
            time(restored,4.5);
            CHECK(std::abs(restored.world().players.at(id).position.y)<.01);
            CHECK(restored.world().players.at(id).vitals.alive());
            std::filesystem::remove_all(folder);
        });
        std::cout<<"OK "<<groups<<" workstation groups, "<<checks<<" checks\n";
        return 0;
    }
    catch(const std::exception&e) {
        std::cerr<<"FAIL after "<<groups<<" groups: "<<e.what()<<'\n';
        return 1;
    }
}
