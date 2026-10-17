#pragma once
#include "ui_scene_root_candidate.h"
#include <charconv>
#include <map>
#include <string>
#include <string_view>
#include <vector>
#include <algorithm>
#include <cmath>

// Private experiment only. Source geometry remains owned and queued by FCanvas;
// this helper never edits a buffered vertex, stack header, or cached hash.
namespace gears_ui_scene_trial {
using Matrix=std::array<float,16>;
struct Check {std::uintptr_t address{};std::vector<unsigned char> bytes;};
struct Config {
    std::uint64_t nonce{},pid{},creation_time{},started_qpc{},world_generation{},base{},client{},scene{};
    int shift_x{},shift_y{};unsigned duration_ms{};std::vector<Check> checks;
};
template<class T> bool decimal(std::string const& text,T& out){
    if(text.empty())return false;T value{};auto result=std::from_chars(text.data(),text.data()+text.size(),value);
    if(result.ec!=std::errc{}||result.ptr!=text.data()+text.size())return false;out=value;return true;
}
inline bool address(std::string_view text,std::uintptr_t& out){
    if(text.size()<3||text.substr(0,2)!="0x")return false;std::uintptr_t value{};
    auto result=std::from_chars(text.data()+2,text.data()+text.size(),value,16);
    if(result.ec!=std::errc{}||result.ptr!=text.data()+text.size()||!value)return false;out=value;return true;
}
inline bool parse(std::string const& text,Config& out){
    if(text.empty()||text.size()>262144||text.find('\0')!=std::string::npos)return false;
    std::map<std::string,std::string> fields;bool section=false;
    for(std::size_t begin=0;begin<text.size();){auto end=text.find('\n',begin);if(end==std::string::npos)end=text.size();
        auto line=text.substr(begin,end-begin);begin=end+1;if(!line.empty()&&line.back()=='\r')line.pop_back();
        if(line.empty())continue;if(line=="[PathChoiceTrial]"){if(section)return false;section=true;continue;}
        auto split=line.find('=');if(!section||split==std::string::npos||split==0||line.size()>1024)return false;
        if(!fields.emplace(line.substr(0,split),line.substr(split+1)).second)return false;
    }
    Config c;unsigned schema{},count{};
    auto u=[&](char const* key,auto& value){auto it=fields.find(key);return it!=fields.end()&&decimal(it->second,value);};
    auto a=[&](char const* key,auto& value){auto it=fields.find(key);std::uintptr_t v{};
        if(it==fields.end()||!address(it->second,v))return false;value=v;return true;};
    if(!u("schema",schema)||schema!=1||!u("nonce",c.nonce)||!c.nonce||!u("pid",c.pid)||!c.pid||c.pid>0xffffffffull||
       !u("creation_time",c.creation_time)||!c.creation_time||!u("started_qpc",c.started_qpc)||!c.started_qpc||
       !u("world_generation",c.world_generation)||!c.world_generation||!a("base",c.base)||!a("client",c.client)||
       !a("scene",c.scene)||c.client==c.scene||!u("shift_x",c.shift_x)||!u("shift_y",c.shift_y)||
       c.shift_x< -512||c.shift_x>512||c.shift_y< -512||c.shift_y>512||!u("duration_ms",c.duration_ms)||
       c.duration_ms<100||c.duration_ms>8000||!u("check_count",count)||count<16||count>2048||fields.size()!=13+count)return false;
    std::map<std::uintptr_t,unsigned char> union_bytes;std::size_t total_bytes=0;
    for(unsigned i=0;i<count;++i){auto it=fields.find("check_"+std::to_string(i));if(it==fields.end())return false;
        const auto split=it->second.find(':');Check check;
        if(split==std::string::npos||!address(std::string_view(it->second).substr(0,split),check.address))return false;
        auto hex=it->second.substr(split+1);if(hex.empty()||hex.size()%2||hex.size()>512)return false;
        total_bytes+=hex.size()/2;if(total_bytes>32768)return false;
        if(check.address>UINTPTR_MAX-hex.size()/2)return false;
        auto nibble=[](char ch)->int{return ch>='0'&&ch<='9'?ch-'0':ch>='a'&&ch<='f'?ch-'a'+10:-1;};
        for(std::size_t j=0;j<hex.size();j+=2){auto hi=nibble(hex[j]),lo=nibble(hex[j+1]);if(hi<0||lo<0)return false;
            auto byte=static_cast<unsigned char>(hi*16+lo);check.bytes.push_back(byte);
            auto found=union_bytes.emplace(check.address+j/2,byte);if(!found.second&&found.first->second!=byte)return false;}
        if(union_bytes.size()>32768)return false;c.checks.push_back(std::move(check));
    }
    out=std::move(c);return true;
}
template<class Read> bool checks(Config const& c,Read read){
    std::array<unsigned char,256> bytes{};
    for(auto const& check:c.checks)if(check.bytes.empty()||check.bytes.size()>bytes.size()||
       !read(bytes.data(),reinterpret_cast<void const*>(check.address),check.bytes.size())||
       !std::equal(check.bytes.begin(),check.bytes.end(),bytes.begin()))return false;
    return true;
}
inline Matrix translation(int x,int y){Matrix m{};for(unsigned i=0;i<4;++i)m[i*4+i]=1;m[12]=float(x);m[13]=float(y);return m;}
inline Matrix matrix(std::array<unsigned char,80> const& item){Matrix m{};std::memcpy(m.data(),item.data(),64);return m;}
inline bool finite(Matrix const& m){return std::all_of(m.begin(),m.end(),[](float f){return std::isfinite(f);});}
inline Matrix multiply(Matrix const& left,Matrix const& right){Matrix result{};
    for(unsigned row=0;row<4;++row)for(unsigned column=0;column<4;++column){double sum=0;
        for(unsigned k=0;k<4;++k)sum+=double(left[row*4+k])*right[k*4+column];result[row*4+column]=float(sum);}return result;}
inline bool close(Matrix const& a,Matrix const& b){if(!finite(a)||!finite(b))return false;
    for(unsigned i=0;i<16;++i)if(std::abs(double(a[i])-b[i])>1e-5+std::abs(double(b[i]))*1e-4)return false;return true;}
struct Lease {
    gears_ui_scene_root::MatrixSnapshot before{},pushed{};bool owned{},restored{},matrix_matches{},failed{};
};
template<class Read> bool stack(Read read,std::uintptr_t canvas,gears_ui_scene_root::MatrixSnapshot& out){
    gears_ui_batch::detail::Reader<Read> r{read};gears_ui_scene_root::MatrixSnapshot value{};value.fcanvas=canvas;
    if(!canvas||!r.field(value.stack,canvas,0x38)||!gears_ui_batch::detail::valid(value.stack,4096,80)||!value.stack.count||
       !gears_ui_batch::detail::add(value.stack.data,std::size_t(value.stack.count-1)*80,value.top_address)||
       !r.bytes(value.top.data(),value.top_address,80))return false;
    gears_ui_batch::detail::Array after{};std::array<unsigned char,80> top{};
    if(!r.field(after,canvas,0x38)||!gears_ui_batch::detail::same(after,value.stack)||
       !r.bytes(top.data(),value.top_address,80)||top!=value.top||!r.field(after,canvas,0x38)||
       !gears_ui_batch::detail::same(after,value.stack))return false;out=value;return true;
}
template<class Read> bool parent(Read read,gears_ui_scene_root::MatrixSnapshot const& current,Lease const& lease){
    if(current.fcanvas!=lease.before.fcanvas||current.stack.count!=lease.before.stack.count+1)return false;
    std::array<unsigned char,80> value{};
    auto location=current.stack.data+std::size_t(current.stack.count-2)*80;
    return read(value.data(),reinterpret_cast<void const*>(location),value.size())&&value==lease.before.top;
}
template<class Read,class Push> bool begin(Read read,Push push,std::uintptr_t canvas,Matrix const& local,Lease& out){
    if(out.owned||!finite(local)||!gears_ui_scene_root::matrix_stack(read,canvas,out.before)||
       out.before.stack.count>=out.before.stack.capacity||!finite(matrix(out.before.top)))return false;
    // Spare capacity avoids adding an allocator call to this private probe.
    try{push(canvas,local);}catch(...){out.failed=true;return false;}
    if(!stack(read,canvas,out.pushed)||!parent(read,out.pushed,out)){out.failed=true;return false;}
    out.owned=true;out.matrix_matches=close(matrix(out.pushed.top),multiply(local,matrix(out.before.top)));
    return out.matrix_matches;
}
template<class Read,class Pop> bool finish(Read read,Pop pop,Lease& lease){
    if(!lease.owned)return false;gears_ui_scene_root::MatrixSnapshot current{};
    if(!stack(read,lease.before.fcanvas,current)||!parent(read,current,lease)||
       current.top!=lease.pushed.top){lease.failed=true;return false;}
    try{pop(lease.before.fcanvas,current.stack.count-1);}catch(...){lease.failed=true;return false;}
    gears_ui_scene_root::MatrixSnapshot after{};
    lease.restored=stack(read,lease.before.fcanvas,after)&&
        after.stack.count==lease.before.stack.count&&after.top==lease.before.top;
    lease.owned=!lease.restored;lease.failed|=!lease.restored;return lease.restored;
}
template<class Read,class Pop,class Original> std::uintptr_t run(Read read,Pop pop,Lease& lease,Original original){
    std::uintptr_t value{};
    try{value=original();}
    catch(...){finish(read,pop,lease);lease.failed=true;throw;}
    finish(read,pop,lease);return value;
}
} // namespace gears_ui_scene_trial
