#include "aetherwild/json.hpp"
namespace aw {
    namespace {
        std::string escaped(const std::string&s) {
            std::ostringstream o;
            o<<'"';
            for(unsigned char c:s) {
                if(c=='"')o<<"\\\"";
                else if(c=='\\')o<<"\\\\";
                else if(c=='\n')o<<"\\n";
                else if(c=='\r')o<<"\\r";
                else if(c=='\t')o<<"\\t";
                else if(c<32)o<<"\\u"<<std::hex<<std::setw(4)<<std::setfill('0')<<int(c)<<std::dec;
                else o<<c;
            }
            o<<'"';
            return o.str();
        }
        void utf8(std::string&o,unsigned cp) {
            if(cp<128)o+=char(cp);
            else if(cp<2048) {
                o+=char(0xc0|(cp>>6));
                o+=char(0x80|(cp&63));
            }
            else if(cp<65536) {
                o+=char(0xe0|(cp>>12));
                o+=char(0x80|((cp>>6)&63));
                o+=char(0x80|(cp&63));
            }
            else {
                o+=char(0xf0|(cp>>18));
                o+=char(0x80|((cp>>12)&63));
                o+=char(0x80|((cp>>6)&63));
                o+=char(0x80|(cp&63));
            }
        }
        class Parser {
            const std::string&s;
            std::size_t i {};
            void space() {
                while(i<s.size()&&(s[i]==' '||s[i]=='\n'||s[i]=='\r'||s[i]=='\t'))++i;
            }
            char take() {
                require(i<s.size(),"JSON不完整");
                return s[i++];
            }
            unsigned hex() {
                unsigned n=0;
                for(int j=0;j<4;j++) {
                    char c=take();
                    require((c>='0'&&c<='9')||(c>='a'&&c<='f')||(c>='A'&&c<='F'),"JSON Unicode錯誤");
                    n=n*16+(c<='9'?c-'0':(c|32)-'a'+10);
                }
                return n;
            }
            std::string text() {
                require(take()=='"',"JSON字串錯誤");
                std::string o;
                while(true) {
                    char c=take();
                    if(c=='"')break;
                    require(static_cast<unsigned char>(c)>=32,"JSON控制字元錯誤");
                    if(c!='\\') {
                        o+=c;
                        continue;
                    }
                    char e=take();
                    if(e=='u') {
                        unsigned cp=hex();
                        if(cp>=0xd800&&cp<=0xdbff) {
                            require(take()=='\\'&&take()=='u',"JSON surrogate錯誤");
                            auto lo=hex();
                            require(lo>=0xdc00&&lo<=0xdfff,"JSON surrogate錯誤");
                            cp=0x10000+((cp-0xd800)<<10)+(lo-0xdc00);
                        }
                        else require(cp<0xdc00||cp>0xdfff,"JSON surrogate錯誤");
                        utf8(o,cp);
                    }
                    else {
                        std::string a="\"\\/bfnrt",b="\"\\/\b\f\n\r\t";
                        auto p=a.find(e);
                        require(p!=std::string::npos,"JSON跳脫錯誤");
                        o+=b[p];
                    }
                }
                return o;
            }
            Json read(int depth) {
                require(depth<64,"JSON過深");
                space();
                require(i<s.size(),"JSON不完整");
                char c=s[i];
                if(c=='"')return text();
                if(c=='{') {
                    ++i;
                    Json::Object o;
                    space();
                    if(i<s.size()&&s[i]=='}') {
                        ++i;
                        return o;
                    }
                    while(true) {
                        space();
                        auto k=text();
                        space();
                        require(take()==':',"JSON缺少冒號");
                        auto v=read(depth+1);
                        require(o.emplace(k,std::move(v)).second,"JSON重複欄位");
                        space();
                        char e=take();
                        if(e=='}')break;
                        require(e==',',"JSON缺少逗號");
                    }
                    return o;
                }
                if(c=='[') {
                    ++i;
                    Json a=Json::array();
                    space();
                    if(i<s.size()&&s[i]==']') {
                        ++i;
                        return a;
                    }
                    while(true) {
                        a.push(read(depth+1));
                        space();
                        char e=take();
                        if(e==']')break;
                        require(e==',',"JSON缺少逗號");
                    }
                    return a;
                }
                for(auto [word,v]:std::vector<std::pair<std::string,Json>> {
                    {
                        "true",true
                    },
                    {
                        "false",false
                    },
                    {
                        "null",Json()
                    }
                })if(s.compare(i,word.size(),word)==0) {
                    i+=word.size();
                    return v;
                }
                std::size_t start=i;
                if(s[i]=='-')++i;
                require(i<s.size()&&s[i]>='0'&&s[i]<='9',"JSON數值錯誤");
                if(s[i]=='0')++i;
                else while(i<s.size()&&s[i]>='0'&&s[i]<='9')++i;
                bool whole=true;
                if(i<s.size()&&s[i]=='.') {
                    whole=false;
                    ++i;
                    auto b=i;
                    while(i<s.size()&&s[i]>='0'&&s[i]<='9')++i;
                    require(i>b,"JSON小數錯誤");
                }
                if(i<s.size()&&(s[i]=='e'||s[i]=='E')) {
                    whole=false;
                    ++i;
                    if(i<s.size()&&(s[i]=='+'||s[i]=='-'))++i;
                    auto b=i;
                    while(i<s.size()&&s[i]>='0'&&s[i]<='9')++i;
                    require(i>b,"JSON指數錯誤");
                }
                auto token=s.substr(start,i-start);
                if(whole) {
                    std::int64_t n=0;
                    auto r=std::from_chars(token.data(),token.data()+token.size(),n);
                    require(r.ec==std::errc {},
                    "JSON整數溢位");
                    return n;
                }
                double n=0;
                try {
                    n=std::stod(token);
                }
                catch(...) {
                    throw RuleError("JSON數值溢位");
                }
                return Json(n);
            }
            public:explicit Parser(const std::string&v):s(v) {}
            Json parse() {
                auto o=read(0);
                space();
                require(i==s.size(),"JSON多餘內容");
                return o;
            }
        };
    }
    std::string Json::dump()const {
        if(std::holds_alternative<std::monostate>(value))return "null";
        if(auto p=std::get_if<bool>(&value))return *p?"true":"false";
        if(auto p=std::get_if<std::int64_t>(&value))return std::to_string(*p);
        if(auto p=std::get_if<double>(&value)) {
            require(std::isfinite(*p),"JSON數值錯誤");
            std::ostringstream o;
            o<<std::setprecision(17)<<*p;
            return o.str();
        }
        if(auto p=std::get_if<std::string>(&value))return escaped(*p);
        std::string o;
        if(auto p=std::get_if<Array>(&value)) {
            o="[";
            for(auto&v:*p) {
                if(o.size()>1)o+=',';
                o+=v.dump();
            }
            return o+"]";
        }
        o="{";
        for(auto&[k,v]:std::get<Object>(value)) {
            if(o.size()>1)o+=',';
            o+=escaped(k)+":"+v.dump();
        }
        return o+"}";
    }
    Json Json::parse(const std::string&s) {
        require(s.size()<=16*1024*1024,"JSON過大");
        return Parser(s).parse();
    }
}
