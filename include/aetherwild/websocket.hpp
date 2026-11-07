#pragma once
#include "domain.hpp"
#include <array>
#include <bit>
#include <optional>
namespace aw::ws {
// SHA-1 is required only for the RFC 6455 public handshake, never for passwords.
inline std::string acceptKey(const std::string& key) {
    require(key.size()==24&&key.substr(22)=="=="&&std::all_of(key.begin(),key.begin()+22,[](unsigned char c){return (c>='A'&&c<='Z')||(c>='a'&&c<='z')||(c>='0'&&c<='9')||c=='+'||c=='/';}),"WebSocket key格式錯誤");
    std::string data=key+"258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
    const std::uint64_t bits=data.size()*8;data.push_back(static_cast<char>(0x80));
    while(data.size()%64!=56)data.push_back(0);
    for(int i=7;i>=0;i--)data.push_back(static_cast<char>(bits>>(i*8)));
    std::array<std::uint32_t,5> hash{0x67452301,0xefcdab89,0x98badcfe,0x10325476,0xc3d2e1f0};
    for(std::size_t offset=0;offset<data.size();offset+=64) {
        std::array<std::uint32_t,80> w{};
        for(int i=0;i<16;i++)for(int j=0;j<4;j++)w[i]=(w[i]<<8)|static_cast<unsigned char>(data[offset+i*4+j]);
        for(int i=16;i<80;i++)w[i]=std::rotl(w[i-3]^w[i-8]^w[i-14]^w[i-16],1);
        auto [a,b,c,d,e]=hash;
        for(int i=0;i<80;i++) {
            const auto f=i<20?((b&c)|(~b&d)):i<40?(b^c^d):i<60?((b&c)|(b&d)|(c&d)):(b^c^d);
            const std::uint32_t k=i<20?0x5a827999:i<40?0x6ed9eba1:i<60?0x8f1bbcdc:0xca62c1d6;
            const auto next=std::rotl(a,5)+f+e+k+w[i];e=d;d=c;c=std::rotl(b,30);b=a;a=next;
        }
        hash[0]+=a;hash[1]+=b;hash[2]+=c;hash[3]+=d;hash[4]+=e;
    }
    std::string raw;for(auto word:hash)for(int i=3;i>=0;i--)raw.push_back(static_cast<char>(word>>(i*8)));
    constexpr char alphabet[]="ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::string out;
    for(std::size_t i=0;i<raw.size();i+=3) {
        const auto n=(std::uint32_t(static_cast<unsigned char>(raw[i]))<<16)|(i+1<raw.size()?std::uint32_t(static_cast<unsigned char>(raw[i+1]))<<8:0)|(i+2<raw.size()?static_cast<unsigned char>(raw[i+2]):0);
        out+=alphabet[(n>>18)&63];out+=alphabet[(n>>12)&63];out+=i+1<raw.size()?alphabet[(n>>6)&63]:'=';out+=i+2<raw.size()?alphabet[n&63]:'=';
    }
    return out;
}
inline bool utf8(const std::string& data) {
    for(std::size_t i=0;i<data.size();) {
        auto c=static_cast<unsigned char>(data[i++]);if(c<128)continue;
        int n=c>=0xc2&&c<=0xdf?1:c>=0xe0&&c<=0xef?2:c>=0xf0&&c<=0xf4?3:-1;
        if(n<0||i+n>data.size())return false;
        std::uint32_t value=c&((1<<(6-n))-1);
        for(int j=0;j<n;j++){auto b=static_cast<unsigned char>(data[i++]);if((b&0xc0)!=0x80)return false;value=(value<<6)|(b&63);}
        if(value<(n==1?0x80u:n==2?0x800u:0x10000u)||value>0x10ffff||(value>=0xd800&&value<=0xdfff))return false;
    }
    return true;
}
struct Frame {int opcode;bool final;std::string payload;};
inline std::optional<Frame> decode(std::string& buffer) {
    if(buffer.size()<2)return std::nullopt;
    auto byte=[&](std::size_t i){return static_cast<unsigned char>(buffer[i]);};
    const auto first=byte(0),second=byte(1);const int opcode=first&15;
    require(!(first&0x70)&&(second&0x80)&&(opcode==0||opcode==1||opcode==8||opcode==9||opcode==10),"WebSocket frame格式錯誤");
    std::uint64_t size=second&127;std::size_t at=2;
    if(size==126){if(buffer.size()<4)return std::nullopt;size=(byte(2)<<8)|byte(3);at=4;require(size>=126,"WebSocket長度錯誤");}
    else if(size==127){if(buffer.size()<10)return std::nullopt;size=0;for(int i=2;i<10;i++)size=(size<<8)|byte(i);at=10;require(size>=65536,"WebSocket長度錯誤");}
    require(size<=16384&&(opcode<8||((first&0x80)&&size<=125)),"WebSocket訊息過大或控制frame錯誤");
    if(buffer.size()<at+4+size)return std::nullopt;
    Frame frame{opcode,bool(first&0x80),std::string(static_cast<std::size_t>(size),'\0')};
    for(std::size_t i=0;i<size;i++)frame.payload[i]=buffer[at+4+i]^buffer[at+i%4];
    buffer.erase(0,at+4+static_cast<std::size_t>(size));return frame;
}
inline std::string encode(const std::string& payload,int opcode=1) {
    std::string out(1,static_cast<char>(0x80|opcode));const auto n=payload.size();
    if(n<126)out.push_back(static_cast<char>(n));
    else if(n<=65535){out.push_back(126);out.push_back(static_cast<char>(n>>8));out.push_back(static_cast<char>(n));}
    else{out.push_back(127);for(int i=7;i>=0;i--)out.push_back(static_cast<char>(std::uint64_t(n)>>(i*8)));}
    return out+payload;
}
}
