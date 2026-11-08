#include "aetherwild/talents.hpp"
#include "aetherwild/persistence.hpp"
#include <fstream>
#include <iomanip>
#include <sstream>
namespace aw {
    namespace {
        void validate(const Player&p,const Catalog&c) {
            require(!p.id.empty()&&!p.name.empty()&&p.id.size()<=128&&p.name.size()<=128&&c.classes.contains(p.profession)&&p.level>=1&&p.level<=100&&p.coins>=0&&p.bankCoins>=0&&p.xp>=0&&p.gatherXp>=0,"存檔角色數值錯誤");
            require(p.scene=="world"&&c.regions.contains(p.lastCity)&&!c.regions.at(p.lastCity).wilderness&&finite(p.position)&&std::abs(p.position.x)<=c.worldLimit&&std::abs(p.position.z)<=c.worldLimit,"存檔場景錯誤");
            require(std::isfinite(p.vitals.hp)&&std::isfinite(p.vitals.mp)&&std::isfinite(p.vitals.stamina)&&p.vitals.hp>=0&&p.vitals.mp>=0&&p.vitals.stamina>=0,"存檔生命數值錯誤");
            for(auto*m: {&p.materials,&p.warehouse})for(auto&[id,n]:*m)require(!id.empty()&&n>=0&&(c.resources.contains(id)||c.arrowBonus.contains(id)||id=="potion"||id=="energy_potion"),"存檔材料錯誤");
            for(auto&[id,n]:p.tools)require((id=="axe"||id=="pick"||id=="sickle")&&n>=1&&n<=5,"存檔工具錯誤");
            std::set<Id>ids;
            for(auto* items:{&p.inventory,&p.warehouseItems})for(auto&i:*items)require(!i.id.empty()&&ids.insert(i.id).second&&c.items.contains(i.definition)&&i.requiredLevel>=1&&i.requiredLevel<=100&&i.upgrade>=0&&i.upgrade<=20&&static_cast<int>(i.rarity)>=0&&static_cast<int>(i.rarity)<=6,"存檔裝備錯誤");
            for(auto&[slot,id]:p.equipped) {
                auto it=std::find_if(p.inventory.begin(),p.inventory.end(),[&](auto&i) {
                    return i.id==id;
                });
                require(it!=p.inventory.end(),"裝備參照不存在");
                auto&d=c.items.at(it->definition);
                require(d.slot==slot&&p.level>=it->requiredLevel&&(slot!=Slot::Weapon||d.weapon==c.classes.at(p.profession).weapon),"穿戴裝備不合法");
            }
            for(auto&[id,v]:p.quests)require(c.quests.contains(id)&&v.count>=0&&v.count<=c.quests.at(id).count,"存檔任務錯誤");
            for(auto&id:p.jobs)require(c.classes.contains(id),"存檔職業錯誤");
            require(p.jobs.contains(p.profession),"未取得目前職業");
            validateTalents(p);
            for(auto&id:p.learnedSkills)require(c.skills.contains(id),"存檔技能錯誤");
            for(auto&id:p.unlockedMaps)require(c.regions.contains(id),"存檔地圖錯誤");
        }
        void writeSet(std::ostream&o,const std::set<Id>&s) {
            o<<s.size()<<' ';
            for(auto&id:s)o<<std::quoted(id)<<' ';
            o<<'\n';
        }
        std::size_t count(std::istream&i) {
            std::size_t n=0;
            i>>n;
            require(bool(i)&&n<=100000,"存檔集合數量錯誤");
            return n;
        }
        void readSet(std::istream&i,std::set<Id>&s) {
            auto n=count(i);
            for(std::size_t j=0;j<n;j++) {
                Id v;
                i>>std::quoted(v);
                require(!v.empty()&&s.insert(v).second,"存檔重複 ID");
            }
        }
        void writeMap(std::ostream&o,const Materials&m) {
            o<<m.size()<<' ';
            for(auto&[k,n]:m)o<<std::quoted(k)<<' '<<n<<' ';
            o<<'\n';
        }
        void readMap(std::istream&i,Materials&m) {
            auto n=count(i);
            for(std::size_t j=0;j<n;j++) {
                Id k;
                Quantity v {};
                i>>std::quoted(k)>>v;
                require(m.emplace(k,v).second,"存檔重複欄位");
            }
        }
    }
    void CharacterFile::save(const Player&p,const std::filesystem::path&path,const Catalog&c) {
        validate(p,c);
        auto tmp=path;
        tmp+=".tmp";
        std::ofstream o(tmp,std::ios::binary|std::ios::trunc);
        require(bool(o),"無法開啟存檔");
        o<<std::setprecision(17)<<"AWCHAR 3\n"<<std::quoted(p.id)<<' '<<std::quoted(p.accountId)<<' '<<std::quoted(p.name)<<' '<<std::quoted(p.profession)<<'\n'<<p.level<<' '<<p.xp<<' '<<p.coins<<' '<<p.bankCoins<<' '<<p.gatherXp<<'\n'<<p.vitals.hp<<' '<<p.vitals.mp<<' '<<p.vitals.stamina<<' '<<p.sprintLocked<<'\n'<<p.position.x<<' '<<p.position.y<<' '<<p.position.z<<' '<<std::quoted(p.lastCity)<<'\n';
        writeMap(o,p.materials);
        writeMap(o,p.warehouse);
        Materials tools;
        for(auto&[k,n]:p.tools)tools[k]=n;
        writeMap(o,tools);
        o<<p.inventory.size()<<'\n';
        for(auto&i:p.inventory)o<<std::quoted(i.id)<<' '<<std::quoted(i.definition)<<' '<<static_cast<int>(i.rarity)<<' '<<i.requiredLevel<<' '<<i.upgrade<<'\n';
        o<<p.equipped.size()<<'\n';
        for(auto&[s,id]:p.equipped)o<<static_cast<int>(s)<<' '<<std::quoted(id)<<'\n';
        for(auto*s: {&p.jobs,&p.secrets,&p.learnedSkills,&p.unlockedMaps,&p.processedRewards})writeSet(o,*s);
        writeSet(o,p.talents);
        o<<p.quests.size()<<'\n';
        for(auto&[id,q]:p.quests)o<<std::quoted(id)<<' '<<q.count<<' '<<q.claimed<<'\n';
        o<<p.warehouseItems.size()<<'\n';
        for(auto&i:p.warehouseItems)o<<std::quoted(i.id)<<' '<<std::quoted(i.definition)<<' '<<static_cast<int>(i.rarity)<<' '<<i.requiredLevel<<' '<<i.upgrade<<'\n';
        o<<"END\n";
        o.flush();
        require(bool(o),"存檔寫入失敗");
        o.close();
        // Keep prior bytes recoverable on platforms where rename cannot replace an existing file.
        auto backup=path;
        backup+=".bak";
        std::error_code ec;
        if(std::filesystem::exists(path)) {
            std::filesystem::copy_file(path,backup,std::filesystem::copy_options::overwrite_existing,ec);
            require(!ec,"無法備份舊存檔");
        }
        std::filesystem::rename(tmp,path,ec);
        if(ec&&std::filesystem::exists(path)) {
            std::filesystem::remove(path,ec);
            if(!ec)std::filesystem::rename(tmp,path,ec);
        }
        require(!ec,"存檔替換失敗，舊資料保留在 .bak");
    }
    Player CharacterFile::load(const std::filesystem::path&path,const Catalog&c) {
        require(std::filesystem::file_size(path)<=16*1024*1024,"存檔過大");
        std::ifstream i(path,std::ios::binary);
        std::string magic;
        int version=0;
        i>>magic>>version;
        require(magic=="AWCHAR"&&(version>=1&&version<=3),"存檔格式或版本不符");
        Player p;
        p.jobs.clear();
        i>>std::quoted(p.id)>>std::quoted(p.accountId)>>std::quoted(p.name)>>std::quoted(p.profession)>>p.level>>p.xp>>p.coins>>p.bankCoins>>p.gatherXp>>p.vitals.hp>>p.vitals.mp>>p.vitals.stamina>>p.sprintLocked>>p.position.x>>p.position.y>>p.position.z>>std::quoted(p.lastCity);
        readMap(i,p.materials);
        readMap(i,p.warehouse);
        Materials tools;
        readMap(i,tools);
        for(auto&[k,n]:tools) {
            require(n>=1&&n<=5,"工具階級錯誤");
            p.tools[k]=static_cast<int>(n);
        }
        auto n=count(i);
        for(std::size_t j=0;j<n;j++) {
            ItemInstance v;
            int r=0;
            i>>std::quoted(v.id)>>std::quoted(v.definition)>>r>>v.requiredLevel>>v.upgrade;
            v.rarity=static_cast<Rarity>(r);
            p.inventory.push_back(v);
        }
        n=count(i);
        for(std::size_t j=0;j<n;j++) {
            int slot=0;
            Id id;
            i>>slot>>std::quoted(id);
            require(slot>=0&&slot<=4&&p.equipped.emplace(static_cast<Slot>(slot),id).second,"裝備欄錯誤");
        }
        for(auto*s: {&p.jobs,&p.secrets,&p.learnedSkills,&p.unlockedMaps,&p.processedRewards})readSet(i,*s);
        if(version>=3)readSet(i,p.talents);
        validateTalents(p);
        n=count(i);
        for(std::size_t j=0;j<n;j++) {
            Id id;
            QuestProgress q;
            i>>std::quoted(id)>>q.count>>q.claimed;
            require(p.quests.emplace(id,q).second,"重複任務");
        }
        if(version>=2) {
            n=count(i);
            for(std::size_t j=0;j<n;j++) {
                ItemInstance v;
                int rarity=0;
                i>>std::quoted(v.id)>>std::quoted(v.definition)>>rarity>>v.requiredLevel>>v.upgrade;
                v.rarity=static_cast<Rarity>(rarity);
                p.warehouseItems.push_back(v);
            }
        }
        std::string end;
        i>>end;
        require(bool(i)&&end=="END","存檔損壞或不完整");
        i>>std::ws;
        require(i.eof(),"存檔包含多餘資料");
        validate(p,c);
        double hp=p.vitals.hp,mp=p.vitals.mp,st=p.vitals.stamina;
        ProgressionSystem::refresh(p,c);
        require(hp<=p.vitals.maxHp&&mp<=p.vitals.maxMp&&st<=p.vitals.maxStamina,"存檔數值超出上限");
        return p;
    }
}

