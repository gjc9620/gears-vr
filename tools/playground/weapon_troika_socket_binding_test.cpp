#define GEARS_TROIKA_BINDING_REUSE_FIXTURE 1
#include "weapon_troika_control_binding_test.cpp"
namespace {
namespace troika_mounted_parts {
#include "weapon_troika_socket_binding.inl"
}
namespace socket_read=troika_mounted_parts::socket_binding;
struct SocketMemory:BindingMemory {
    uptr socket_class{},selected{},other{},socket_field{},socket_inner{},socket_name{},bone_name{},location{},scale{},rotation{},rotator{};
    std::array<uptr,2> sockets{};meta::RawName chosen{};
    SocketMemory(){
        socket_class=cls(L"SkeletalMeshSocket",engine,object_class);
        selected=node(L"ActualMuzzleSocket",socket_class,asset);other=node(L"OtherMuzzleSocket",socket_class,asset);
        socket_field=property(skeletal_class,L"Sockets",array_type,0x160,16);
        socket_inner=property(socket_field,L"Sockets",object_type,0,8);put(socket_field,0xd0,socket_inner);put(socket_inner,0xd0,socket_class);
        socket_name=property(socket_class,L"SocketName",name_class,0x100,8);bone_name=property(socket_class,L"BoneName",name_class,0x108,8);
        location=property(socket_class,L"RelativeLocation",struct_class,0x110,12);put(location,0xd0,vector_type);
        scale=property(socket_class,L"RelativeScale",struct_class,0x130,12);put(scale,0xd0,vector_type);
        rotator=node(L"Rotator",cls(L"ScriptStruct",core),object_class);put(rotator,0x88,12u);
        const auto ints=cls(L"IntProperty",core);property(rotator,L"Pitch",ints,0,4);property(rotator,L"Yaw",ints,4,4);property(rotator,L"Roll",ints,8,4);
        rotation=property(socket_class,L"RelativeRotation",struct_class,0x120,12);put(rotation,0xd0,rotator);
        chosen=text(L"ActualRightSocket");const auto left=text(L"ActualLeftSocket");
        put(selected,0x100,chosen);put(other,0x100,left);
        meta::RawName r{},l{};std::memcpy(&r,references[7].data(),8);std::memcpy(&l,references[8].data(),8);
        put(selected,0x108,r);put(other,0x108,l);
        for(auto p:{selected,other}){put(p,0x110,std::array<float,3>{2,3,4});put(p,0x120,std::array<std::int32_t,3>{100,200,300});put(p,0x130,std::array<float,3>{1,1,1});}
        sockets={other,selected};put(asset,0x160,meta::Table{reinterpret_cast<uptr>(sockets.data()),2,2});tables();
    }
    bool read(socket_read::Snapshot& out,std::function<bool()> current=[](){return true;}){
        binding::Snapshot bound{};bound.asset=identity(asset);bound.reference_bytes=references;
        return socket_read::read(identity(asset),chosen,bound,current,out);
    }
};
}
int main(){
    socket_read::Snapshot sentinel{};std::memset(&sentinel,0x51,sizeof sentinel);
    {auto m=std::make_unique<SocketMemory>();auto out=sentinel;CHECK(m->read(out));CHECK(out.bone==7&&out.object.address==m->selected);
        CHECK((out.location==std::array<float,3>{2,3,4}));const auto first=out;CHECK(m->read(out)&&socket_read::same(first,out));
        m->chosen=m->text(L"ActualLeftSocket");CHECK(m->read(out)&&out.bone==8&&out.object.address==m->other);}
    for(unsigned fault=0;fault<15;++fault){auto m=std::make_unique<SocketMemory>();auto out=sentinel;
        if(fault==0)++m->chosen.number;
        if(fault==1)m->chosen=m->text(L"None");
        if(fault==2)m->put(m->other,0x100,m->chosen);
        if(fault==3)m->sockets[0]=m->selected;
        if(fault==4)m->sockets[0]=0;
        if(fault==5)m->put(m->selected,0x40,m->world);
        if(fault==6)m->put(m->selected,0x108,m->text(L"b_troika_handleroot"));
        if(fault==7)m->put(m->selected,0x130,std::array<float,3>{1,2,1});
        if(fault==8)m->put(m->selected,0x130,std::array<float,3>{0,0,0});
        if(fault==9)m->put(m->selected,0x110,std::array<float,3>{NAN,0,0});
        if(fault==10)m->put(m->socket_inner,0x74,4u);
        if(fault==11)m->put(m->socket_inner,0xd0,m->skeletal_class);
        if(fault==12)m->put(m->asset,0x160,meta::Table{reinterpret_cast<uptr>(m->sockets.data()),2,1});
        if(fault==13)m->put(m->rotation,0xd0,m->vector_type);
        if(fault==14)m->put(m->selected,0x50,m->mesh_class);
        const bool accepted=m->read(out);if(accepted)std::printf("unexpected accepted socket fault=%u\n",fault);
        CHECK(!accepted);CHECK(!std::memcmp(&out,&sentinel,sizeof out));
    }
    for(unsigned fault=0;fault<4;++fault){auto m=std::make_unique<SocketMemory>();auto out=sentinel;unsigned calls{};
        CHECK(!m->read(out,[&](){if(++calls==2){
            if(fault==0)m->put(m->selected,0x108,m->text(L"b_troika_handle_L"));
            if(fault==1)m->sockets[1]=m->other;
            if(fault==2)m->put(m->selected,0x110,std::array<float,3>{2,3,5});
            if(fault==3)return false;
        }return true;}));CHECK(calls>=2);CHECK(!std::memcmp(&out,&sentinel,sizeof out));}
    std::printf("Troika original selected socket-to-muzzle binding: %u checks passed; actual Reader/owned metadata only, no original game getter or gun consumption proof.\n",checks);
}
