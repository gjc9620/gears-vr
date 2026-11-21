#include "legacy_recording.h"
#include <cstring>
namespace workbench {
namespace {
struct WirePose {float p[3],q[4];};
struct WireJoint {WirePose pose;float radius;std::uint64_t flags;};
struct WireHand {std::uint32_t active,reserved;WireJoint joints[26];};
struct WireFrame {std::int64_t time;std::uint64_t epoch;WirePose head,object;WireHand hands[2];};
static_assert(sizeof(WirePose)==28&&sizeof(WireJoint)==40&&sizeof(WireFrame)==2168);
WirePose pack(P p){return {{float(p.position.x),float(p.position.y),float(p.position.z)},{float(p.orientation.x),float(p.orientation.y),float(p.orientation.z),float(p.orientation.w)}};}
P unpack(WirePose const& p){P r{{p.p[0],p.p[1],p.p[2]},{p.q[0],p.q[1],p.q[2],p.q[3]}};if(!valid(r))throw std::runtime_error("Invalid recording pose");return r;}
}
void Recorder::save(std::string const& path)const{
    std::ofstream f(path+".tmp",std::ios::binary|std::ios::trunc);if(!f)throw std::runtime_error("Cannot save recording");
    f.write("QWBTAKE2",8);std::uint32_t count=std::uint32_t(frames_.size()),stride=sizeof(WireFrame)+16;f.write(reinterpret_cast<char*>(&count),4);f.write(reinterpret_cast<char*>(&stride),4);
    for(auto const& frame:frames_){WireFrame w{};w.time=frame.time_ns;w.epoch=frame.epoch;w.head=pack(frame.head);w.object=pack(frame.object);
        for(unsigned side=0;side<2;++side){w.hands[side].active=frame.hands[side].active;w.hands[side].reserved=frame.hands[side].data_source;for(unsigned j=0;j<26;++j){auto const& a=frame.hands[side].joints[j];w.hands[side].joints[j]={pack(a.pose),float(a.radius),a.flags};}}
        f.write(reinterpret_cast<char*>(&w),sizeof w);double mechanical[]={frame.starter_angle,frame.charging_distance};f.write(reinterpret_cast<char*>(mechanical),16);
    }f.close();if(!f)throw std::runtime_error("Recording write failed");if(std::rename((path+".tmp").c_str(),path.c_str()))throw std::runtime_error("Recording publish failed");
}
void Recorder::load(std::string const& path){
    std::ifstream f(path,std::ios::binary);char magic[8]{};std::uint32_t count{},stride{};f.read(magic,8);f.read(reinterpret_cast<char*>(&count),4);f.read(reinterpret_cast<char*>(&stride),4);
    bool version2=!std::memcmp(magic,"QWBTAKE2",8);if(!f||(!version2&&std::memcmp(magic,"QWBTAKE1",8))||count>capacity||stride!=sizeof(WireFrame)+(version2?16:0))throw std::runtime_error("Unsupported recording");
    std::vector<Frame> next;next.reserve(count);
    for(unsigned i=0;i<count;++i){WireFrame w{};f.read(reinterpret_cast<char*>(&w),sizeof w);if(!f)throw std::runtime_error("Truncated recording");Frame a{};a.time_ns=w.time;a.epoch=w.epoch;a.head=unpack(w.head);a.object=unpack(w.object);
        if(a.time_ns<=0||(!next.empty()&&(a.time_ns<=next.back().time_ns||a.epoch!=next.back().epoch)))throw std::runtime_error("Recording time/space discontinuity");
        for(unsigned side=0;side<2;++side){if(w.hands[side].active>1||w.hands[side].reserved>2)throw std::runtime_error("Invalid hand state");a.hands[side].active=w.hands[side].active!=0;a.hands[side].data_source=w.hands[side].reserved;for(unsigned j=0;j<26;++j){auto const& q=w.hands[side].joints[j];auto& out=a.hands[side].joints[j];out.pose=unpack(q.pose);out.radius=q.radius;out.flags=q.flags;if(!std::isfinite(out.radius)||out.radius<0||out.radius>.1)throw std::runtime_error("Invalid joint radius");}}
        if(version2){double mechanical[2]{};f.read(reinterpret_cast<char*>(mechanical),16);a.starter_angle=mechanical[0];a.charging_distance=mechanical[1];if(!f||!std::isfinite(a.starter_angle)||a.starter_angle<0||a.starter_angle>1.571||!std::isfinite(a.charging_distance)||a.charging_distance<0||a.charging_distance>.231)throw std::runtime_error("Invalid component state");}
        next.push_back(a);
    }if(f.peek()!=EOF)throw std::runtime_error("Trailing recording data");frames_.swap(next);recording_=false;
}
}
