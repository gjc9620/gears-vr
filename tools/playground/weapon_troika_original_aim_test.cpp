#define GEARS_TROIKA_LIMITS_REUSE_FIXTURE 1
#define GEARS_TROIKA_AIM_FIXTURE_ONLY 1
#include "weapon_troika_original_limits_test.cpp"
namespace {
namespace troika_mounted_parts {
#include "weapon_troika_original_aim.inl"
}
namespace action=troika_mounted_parts::original_aim;
struct AimMemory:LimitMemory {
    uptr setter{},face{},input_set{},answer{},input_face{},face_time{};
    bool accept_set{true},unknown{},face_updates{true};std::array<int,3> controller{},aim{};
    unsigned calls{},before_calls{};
    AimMemory(){
        const auto pc_base=cls(L"PlayerController",engine,actor_class);put(pc_class,0x60,pc_base);
        const auto function_type=cls(L"Function",core);
        setter=node(L"SetRotation",function_type,actor_class);face=node(L"FaceRotation",function_type,turret_class);
        const auto rotator=get<uptr>(view_field,0xd0);
        input_set=property(setter,L"NewRotation",struct_class,4,12);put(input_set,0xd0,rotator);put(input_set,0x78,std::uint64_t(0x80));
        answer=property(setter,L"ReturnValue",bool_type,0,4,4);put(answer,0x78,std::uint64_t(0x480));
        input_face=property(face,L"NewRotation",struct_class,4,12);put(input_face,0xd0,rotator);put(input_face,0x78,std::uint64_t(0x80));
        face_time=property(face,L"DeltaTime",float_class,0,4);put(face_time,0x78,std::uint64_t(0x80));
        for(auto fn:{setter,face}){put(fn,0x88,16u);put(fn,0x106,std::uint16_t(16));}
        put(setter,0x108,std::uint16_t(0));put(face,0x108,std::uint16_t(0xffff));tables();
    }
    struct Call {
        AimMemory* memory{};action::Mode mode{};bool valid{true};struct Request{uptr function{};unsigned size{16};unsigned char params[64]{};}r;
        action::Dispatch run(){auto& m=*memory;++m.calls;std::array<int,3> requested{};std::memcpy(requested.data(),r.params+4,12);
            if(mode==action::Mode::set){if(m.accept_set)m.controller=requested;const unsigned result=m.accept_set?4u:0u;std::memcpy(r.params,&result,4);}
            else if(m.face_updates)m.aim=m.controller;
            if(m.after_call)m.after_call();return m.unknown?action::Dispatch::uncertain:action::Dispatch::returned;
        }
    };
    auto factory(action::Mode mode){return [this,mode](uptr object){Call c{};c.memory=this;c.mode=mode;c.r.function=mode==action::Mode::set?setter:face;
        c.valid=object==(mode==action::Mode::set?pc:turret);std::memset(c.r.params,0xcd,sizeof c.r.params);return c;};}
    template<class Current,class Before>action::Result execute(action::Mode mode,Current&& current,Before&& before){
        return action::execute(identity(mode==action::Mode::set?pc:turret),{100,200,300},mode,factory(mode),
            [](Call& c){return c.run();},current,before);
    }
};
}
int main(){
    {
        action::AttemptGate gate;std::array<std::uint64_t,7> scope{1,2,3,4,5,6,7};
        CHECK(gate.begin(scope,10));CHECK(gate.active());CHECK(!gate.begin(scope,11));gate.finish();
        CHECK(!gate.active());CHECK(!gate.begin(scope,10));CHECK(!gate.begin(scope,9));
        CHECK(gate.begin(scope,11));gate.finish();auto next=scope;++next[4];CHECK(gate.begin(next,1));gate.finish();
        CHECK(!gate.begin(next,0));next[0]=0;CHECK(!gate.begin(next,2));
    }
    {
        auto m=std::make_unique<AimMemory>();action::ActorReceipt receipt{};receipt.requested={100,200,300};
        receipt.setter=m->execute(action::Mode::set,[](){return true;},[&](){++m->before_calls;return true;});
        CHECK(receipt.setter.accepted_set()&&m->calls==1&&m->before_calls==1);
        receipt.controller=m->controller;receipt.controller_read=true;CHECK(!receipt.verified_actor());
        receipt.face=m->execute(action::Mode::face,[](){return true;},[&](){++m->before_calls;return true;});
        receipt.aim=m->aim;receipt.aim_read=true;CHECK(receipt.verified_actor()&&m->calls==2&&m->before_calls==2);
        receipt.aim[1]++;CHECK(receipt.verified_actor());receipt.aim[1]++;CHECK(!receipt.verified_actor());
        CHECK((action::rotation_matches({-1,0,65536},{65535,65536,0})));
        CHECK((!action::rotation_matches({0,2,0},{0,0,0})));
    }
    for(unsigned fault=0;fault<13;++fault){auto m=std::make_unique<AimMemory>();
        if(fault==0)m->put(m->setter,0x40,m->pc_class);
        if(fault==1)m->put(m->input_set,0x94,0u);
        if(fault==2)m->put(m->answer,0xd0,3u);
        if(fault==3)m->put(m->setter,0x108,std::uint16_t(4));
        if(fault==4)m->put(m->input_set,0xd0,m->vector_type);
        if(fault==5)m->put(m->input_set,0x78,std::uint64_t(0x180));
        if(fault==6)m->put(m->setter,0x106,std::uint16_t(15));
        if(fault==7)m->put(m->answer,0x68,m->input_set);
        if(fault==8)m->accept_set=false;
        if(fault==9)m->unknown=true;
        if(fault==10)m->after_call=[&](){m->put(m->answer,0xd0,8u);};
        if(fault==11)m->after_call=[&](){m->put(m->input_set,0x94,8u);};
        if(fault==12)m->put(m->pc_class,0x60,m->actor_class);
        const auto result=m->execute(action::Mode::set,[](){return true;},[&](){++m->before_calls;return true;});
        CHECK(!result.accepted_set());
        CHECK(m->before_calls==m->calls); // No preflight failure dispatches or marks an original attempt.
        if(fault==8)CHECK(result.dispatch==action::Dispatch::returned&&result.original_bool_known&&!result.original_bool);
        if(fault==9)CHECK(result.dispatch==action::Dispatch::uncertain&&!result.original_bool_known&&m->controller[0]==100);
        if(fault==10||fault==11)CHECK(result.dispatch==action::Dispatch::returned&&!result.metadata_closed);
    }
    {
        auto m=std::make_unique<AimMemory>();unsigned before{};bool live=true;
        const auto denied=m->execute(action::Mode::set,[](){return true;},[&](){++before;return false;});
        CHECK(denied.dispatch==action::Dispatch::not_dispatched&&m->calls==0&&before==1);
        const auto absent=m->execute(action::Mode::set,[](){return false;},[&](){++before;return true;});
        CHECK(absent.dispatch==action::Dispatch::not_dispatched&&m->calls==0&&before==1);
        m->after_call=[&](){live=false;};const auto lost=m->execute(action::Mode::set,[&](){return live;},[&](){++before;return true;});
        CHECK(lost.dispatch==action::Dispatch::returned&&lost.original_bool_known&&lost.original_bool&&!lost.source_closed&&!lost.accepted_set());
    }
    for(unsigned fault=0;fault<4;++fault){auto m=std::make_unique<AimMemory>();
        if(fault==0)m->put(m->face_time,0x50,m->object_type);
        if(fault==1)m->put(m->face,0x108,std::uint16_t(0));
        if(fault==2)m->put(m->input_face,0x94,0u);
        if(fault==3)m->unknown=true;
        const auto result=m->execute(action::Mode::face,[](){return true;},[](){return true;});CHECK(!result.completed_face());
    }
    std::printf("Troika original SetRotation/FaceRotation: %u checks passed; actual Reader and emulated dispatch/state, no original game/control consumption proof.\n",checks);
}
