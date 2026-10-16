#include "crosshair_assignment.h"
#include <cassert>
#include <cstring>
#include <map>
#include <vector>
#include <iostream>
using namespace gears_crosshair_assignment;
struct Memory {
    std::map<std::uintptr_t,std::vector<unsigned char>> blocks;
    std::uintptr_t denied{};
    void block(std::uintptr_t p,std::size_t size){blocks[p].resize(size);}
    bool read(void* out,const void* p,std::size_t size){auto a=reinterpret_cast<std::uintptr_t>(p);
        if(a==denied)return false;auto i=blocks.upper_bound(a);if(i==blocks.begin())return false;--i;
        if(a-i->first>i->second.size()||size>i->second.size()-(a-i->first))return false;
        std::memcpy(out,i->second.data()+a-i->first,size);return true;}
    template<class T> void put(std::uintptr_t a,T const& v){auto i=blocks.upper_bound(a);assert(i!=blocks.begin());--i;
        assert(a-i->first<=i->second.size()&&sizeof(v)<=i->second.size()-(a-i->first));
        std::memcpy(i->second.data()+a-i->first,&v,sizeof(v));}
};
struct Fixture {
    Memory m;gears_ui::Config identities{};Config c{};
    std::uintptr_t frame=0x90000,locals=0x91000;std::array<float,3> point{12,24,48};
    unsigned add(const wchar_t* name,unsigned cls,unsigned outer=999){auto index=identities.identity_count++;
        auto& id=identities.identities[index];id.address=0x30000+index*0x2000;id.index=index;id.name_index=index;
        id.cls=cls==999?id.address:identities.identities[cls].address;
        id.outer=outer==999?0:identities.identities[outer].address;
        assert(std::wcslen(name)<id.name.size());std::copy(name,name+std::wcslen(name)+1,id.name.begin());
        m.block(id.address,0x200);m.put(id.address+0x10,id.index);m.put(id.address+0x18,id.name_index);
        m.put(id.address+0x1c,id.number);m.put(id.address+0x20,id.cls);m.put(id.address+0x28,id.outer);
        m.put(0x10000+index*8,id.address);std::uintptr_t entry=0xc0000+index*0x200;
        m.put(0x18000+index*8,entry);m.block(entry,0x180);m.put(entry,id.name_index);m.put(entry+8,id.name);return index;
    }
    std::uintptr_t address(unsigned i) const{return identities.identities[i].address;}
    void property(unsigned i,unsigned offset,unsigned size,std::uintptr_t structure){
        m.put(address(i)+0x94,offset);m.put(address(i)+0x70,unsigned(1));m.put(address(i)+0x74,size);
        m.put(address(i)+0x78,std::uint64_t(0));m.put(address(i)+0xd0,structure);}
    Fixture(){
        c.identities=&identities;identities.base=0x140000000;identities.objects_header=0x8000;identities.names_header=0x8020;
        identities.object_internal_index=0x10;identities.object_name=0x18;identities.object_class=0x20;
        identities.object_outer=0x28;identities.field_super=0x60;identities.name_entry_index=0;
        identities.name_entry_text=8;identities.property_offset_internal=0x94;
        m.block(0x8000,0x40);m.block(0x10000,16000);m.block(0x18000,16000);
        m.put(0x8000,gears_ui::Registry{0x10000,2000,2000});m.put(0x8020,gears_ui::Registry{0x18000,2000,2000});
        auto cls=add(L"Class",999),fn=add(L"Function",cls),sp=add(L"StructProperty",cls),op=add(L"ObjectProperty",cls),ss=add(L"ScriptStruct",cls);
        c.war_pc=add(L"WarPC",cls);c.war_pawn=add(L"WarPawn",cls);c.war_weapon=add(L"WarWeapon",cls);
        c.controller=add(L"PC_0",c.war_pc);c.pawn=add(L"Pawn_0",c.war_pawn);c.weapon=add(L"Weapon_0",c.war_weapon);
        c.writer=add(L"UpdateHUDActions",fn,c.war_pc);c.lhs=add(L"LastCrosshairHitLocation",sp,c.war_pc);
        c.impact_struct=add(L"ImpactInfo",ss);c.vector_struct=add(L"Vector",ss);
        c.member=add(L"HitLocation",sp,c.impact_struct);c.impact=add(L"Impact",sp,c.writer);c.ww=add(L"WW",op,c.writer);
        c.controller_pawn={add(L"Pawn",op,c.war_pc),0x100};c.pawn_controller={add(L"Controller",op,c.war_pawn),0x100};
        c.pawn_weapon={add(L"Weapon",op,c.war_pawn),0x108};c.weapon_instigator={add(L"Instigator",op,c.war_weapon),0x100};
        // Extend the PC block for its real native cache offset.
        m.blocks[address(c.controller)].resize(0x1400);
        c.property_structure=0xd0;c.impact_offset=0x40;c.member_offset=0x10;c.impact_size=64;c.ww_offset=0x10;
        property(c.lhs,0x136c,12,address(c.vector_struct));property(c.member,c.member_offset,12,address(c.vector_struct));
        property(c.impact,c.impact_offset,c.impact_size,address(c.impact_struct));property(c.ww,c.ww_offset,8,address(c.war_weapon));
        for(auto field:{c.controller_pawn,c.pawn_controller,c.pawn_weapon,c.weapon_instigator})m.put(address(field.property)+0x94,field.offset);
        m.put(address(c.controller)+0x100,address(c.pawn));m.put(address(c.pawn)+0x100,address(c.controller));
        m.put(address(c.pawn)+0x108,address(c.weapon));m.put(address(c.weapon)+0x100,address(c.pawn));
        c.script=0x80000;c.script_size=0x16a7;m.block(c.script,c.script_size);
        m.put(address(c.writer)+0x8c,gears_ui::Registry{c.script,int(c.script_size),int(c.script_size)});
        m.put(address(c.writer)+0x114,identities.base+0x1d6ca0);
        m.put(c.script+0x359,std::uint8_t(0x0f));m.put(c.script+0x35a,std::uint8_t(1));m.put(c.script+0x35b,address(c.lhs));
        m.put(c.script+0x363,std::uint8_t(0x35));m.put(c.script+0x364,address(c.member));m.put(c.script+0x36c,address(c.impact_struct));
        m.put(c.script+0x376,std::uint8_t(0));m.put(c.script+0x377,address(c.impact));
        m.block(frame,0x40);m.block(locals,0x100);m.put(frame+0x18,address(c.writer));m.put(frame+0x20,address(c.controller));
        m.put(frame+0x28,c.script+0x35a);m.put(frame+0x30,locals);m.put(locals+c.ww_offset,address(c.weapon));
        m.put(locals+c.impact_offset+c.member_offset,point);m.put(address(c.controller)+0x136c,point);
        m.block(identities.base+0x1b86280,0x200);m.block(identities.base+0x1b9cc18,4);
        for(auto slot:{std::pair<unsigned,unsigned>{0x0f,0x1d7ad0},{0x35,0x1d6750},{0,0x1d7eb0},{1,0x1d8570}})
            m.put(identities.base+0x1b86280+slot.first*8,identities.base+slot.second);
        m.block(0xa0000,0x300);m.put(address(c.member),std::uintptr_t(0xa0000));m.put(0xa0000+0x2a8,identities.base+0x1d5ec8);
        m.block(identities.base+0x1d5ec8,0x80);m.put(identities.base+0x1d5ee5,std::array<unsigned char,3>{0x8b,0x41,0x78});
        m.put(identities.base+0x1d5ef4,std::array<unsigned char,7>{0x48,0x0f,0xba,0xe0,0x16,0x72,0x34});
        m.put(identities.base+0x1d5efb,copy_route);m.block(identities.base+0x1d7ad0,32);m.put(identities.base+0x1d7ad0,execlet_entry);
    }
    auto read(){return [this](void* out,const void* a,std::size_t n){return m.read(out,a,n);};}
    void enable_view(){
        auto class_index=[&](unsigned object){auto cls=identities.identities[object].cls;
            for(unsigned i=0;i<identities.identity_count;++i)if(address(i)==cls)return i;
            assert(false);return 0u;};
        c.view.enabled=true;c.view.rotator_struct=add(L"Rotator",class_index(c.vector_struct));
        unsigned* roles[]{&c.view.cam_loc,&c.view.cam_rot,&c.view.start_trace,&c.view.trace_dir};
        unsigned* offsets[]{&c.view.cam_loc_offset,&c.view.cam_rot_offset,&c.view.start_trace_offset,&c.view.trace_dir_offset};
        const wchar_t* names[]{L"CamLoc",L"CamRot",L"StartTrace",L"TraceDir"};
        for(unsigned i=0;i<4;++i){*roles[i]=add(names[i],class_index(c.impact),c.writer);*offsets[i]=0x90+i*0x10;
            property(*roles[i],*offsets[i],12,address(i==1?c.view.rotator_struct:c.vector_struct));}
        m.put(locals+c.view.cam_loc_offset,std::array<float,3>{1,2,3});
        m.put(locals+c.view.cam_rot_offset,std::array<std::int32_t,3>{10,20,30});
        m.put(locals+c.view.start_trace_offset,std::array<float,3>{4,5,6});
        m.put(locals+c.view.trace_dir_offset,std::array<float,3>{1,0,0});
    }
    void simulate_original(){m.put(frame+0x28,c.script+0x37f);m.put(address(c.controller)+0x136c,point);}
};
static bool same(Snapshot const& a,Snapshot const& b){return a.self==b.self&&a.frame==b.frame&&a.node==b.node&&a.locals==b.locals&&
    a.pawn==b.pawn&&a.weapon==b.weapon&&a.destination==b.destination&&a.source==b.source&&a.script==b.script&&
    a.source_bytes==b.source_bytes&&a.previous_cache==b.previous_cache&&gears_crosshair_view::equal(a.view,b.view);}
static bool same(CompletedAssignmentReceipt const& a,CompletedAssignmentReceipt const& b){return a.controller==b.controller&&
    a.pawn==b.pawn&&a.weapon==b.weapon&&a.writer==b.writer&&a.frame==b.frame&&a.locals==b.locals&&a.destination==b.destination&&
    a.cache_bytes==b.cache_bytes&&a.point==b.point&&gears_crosshair_view::equal(a.view,b.view);}
int main(){
    Fixture f;assert(validate_bindings(f.c,f.read()));Snapshot before{};assert(snapshot(f.c,f.read(),f.address(f.c.controller),f.frame,before));
    CompletedAssignmentReceipt receipt{};f.simulate_original();assert(completed(f.c,f.read(),before,receipt));assert(receipt.point==f.point);
    // The cache already contained this point before the simulated original.
    assert(receipt.cache_bytes==before.previous_cache);
    Fixture changed;changed.m.put(changed.address(changed.c.controller)+0x136c,std::array<float,3>{-1,-2,-3});
    Snapshot changed_before{};assert(snapshot(changed.c,changed.read(),changed.address(changed.c.controller),changed.frame,changed_before));
    changed.simulate_original();assert(completed(changed.c,changed.read(),changed_before,receipt));
    assert(receipt.point==changed.point&&receipt.cache_bytes!=changed_before.previous_cache);
    for(unsigned mode=0;mode<20;++mode){Fixture x;Snapshot out{};out.self=999;auto saved=out;
        switch(mode){
        case 0:x.m.put(x.c.script+0x359,std::uint8_t(0));break;
        case 1:x.m.put(x.c.script+0x35b,x.address(x.c.member));break;
        case 2:x.m.put(x.c.script+0x374,std::uint8_t(1));break;
        case 3:x.m.put(x.c.script+0x375,std::uint8_t(1));break;
        case 4:x.m.put(x.identities.base+0x1b86280+0x0f*8,std::uintptr_t(0));break;
        case 5:x.m.put(x.identities.base+0x1b86280+0x35*8,std::uintptr_t(0));break;
        case 6:x.m.put(x.frame+0x18,x.address(x.c.impact));break;
        case 7:x.m.put(x.frame+0x20,x.address(x.c.pawn));break;
        case 8:x.m.put(x.frame+0x28,x.c.script+0x359);break;
        case 9:x.m.put(x.address(x.c.lhs)+0x78,std::uint64_t(0x20));break;
        case 10:x.m.put(x.address(x.c.member)+0x78,std::uint64_t(0x400000));break;
        case 11:x.m.put(x.identities.base+0x1b9cc18,unsigned(1));break;
        case 12:x.m.put(0xa0000+0x2a8,std::uintptr_t(0));break;
        case 13:x.m.put(x.locals+x.c.ww_offset,std::uintptr_t(0));break;
        case 14:x.m.put(x.address(x.c.lhs)+0x94,unsigned(0x1368));break;
        case 15:x.m.put(x.identities.base+0x1d5efb,std::uint8_t(0));break;
        case 16:x.m.denied=x.c.script+0x359;break;
        case 17:x.m.put(x.address(x.c.member)+0x74,unsigned(16));break;
        case 18:x.m.put(0x10000+x.c.lhs*8,std::uintptr_t(0));break;
        case 19:x.m.put(x.identities.base+0x1b86280,std::uintptr_t(0));break;
        }
        assert(!snapshot(x.c,x.read(),x.address(x.c.controller),x.frame,out)&&same(out,saved));
    }
    for(unsigned mode=0;mode<9;++mode){Fixture x;Snapshot s{};assert(snapshot(x.c,x.read(),x.address(x.c.controller),x.frame,s));
        x.simulate_original();CompletedAssignmentReceipt out{};out.controller=999;auto saved=out;
        switch(mode){
        case 0:x.m.put(x.frame+0x28,x.c.script+0x37e);break;
        case 1:x.m.put(x.address(x.c.controller)+0x100,std::uintptr_t(0));break;
        case 2:x.m.put(x.frame+0x30,x.locals+8);break;
        case 3:x.m.put(x.locals+x.c.ww_offset,std::uintptr_t(0));break;
        case 4:x.m.put(x.address(x.c.controller)+0x136c,std::array<float,3>{1,2,3});break;
        case 5:x.m.put(x.frame+0x18,x.address(x.c.impact));break;
        case 6:x.m.put(x.address(x.c.weapon)+0x100,std::uintptr_t(0));break;
        case 7:x.m.put(x.identities.base+0x1b86280+0x35*8,std::uintptr_t(0));break;
        case 8:x.m.put(x.locals+x.c.impact_offset+x.c.member_offset,std::array<float,3>{1,2,3});break;
        }
        assert(!completed(x.c,x.read(),s,out)&&same(out,saved));
    }
    Fixture patched;patched.m.put(patched.identities.base+0x1d7ad0,std::uint8_t(0xe9));
    assert(!validate_bindings(patched.c,patched.read()));assert(validate_bindings(patched.c,patched.read(),false));
    Fixture unstable;Snapshot untouched{};untouched.frame=999;auto saved=untouched;unsigned token_reads=0;
    auto unstable_read=[&](void* out,const void* a,std::size_t n){auto address=reinterpret_cast<std::uintptr_t>(a);
        if(address==unstable.c.script+0x359&&++token_reads==2)unstable.m.put(unstable.frame+0x30,unstable.locals+8);
        return unstable.m.read(out,a,n);};
    assert(!snapshot(unstable.c,unstable_read,unstable.address(unstable.c.controller),unstable.frame,untouched)&&same(saved,untouched));
    Fixture nonfinite;nonfinite.point[0]=std::numeric_limits<float>::quiet_NaN();
    nonfinite.m.put(nonfinite.locals+nonfinite.c.impact_offset+nonfinite.c.member_offset,nonfinite.point);
    Snapshot nan_before{};assert(snapshot(nonfinite.c,nonfinite.read(),nonfinite.address(nonfinite.c.controller),nonfinite.frame,nan_before));
    nonfinite.simulate_original();CompletedAssignmentReceipt nan_out{};nan_out.controller=999;auto nan_saved=nan_out;
    assert(!completed(nonfinite.c,nonfinite.read(),nan_before,nan_out)&&same(nan_saved,nan_out));
    Fixture viewed;viewed.enable_view();Snapshot view_before{};
    assert(snapshot(viewed.c,viewed.read(),viewed.address(viewed.c.controller),viewed.frame,view_before));
    assert((view_before.view.valid&&view_before.view.location==std::array<float,3>({1,2,3})));
    viewed.simulate_original();CompletedAssignmentReceipt view_out{};
    assert(completed(viewed.c,viewed.read(),view_before,view_out)&&gears_crosshair_view::equal(view_out.view,view_before.view));
    viewed.m.put(viewed.locals+viewed.c.view.cam_rot_offset,std::array<std::int32_t,3>{11,20,30});
    auto view_saved=view_out;assert(!completed(viewed.c,viewed.read(),view_before,view_out)&&same(view_out,view_saved));
    for(unsigned fault=0;fault<5;++fault){Fixture x;x.enable_view();Snapshot out{};out.self=999;auto saved=out;
        if(fault==0)x.m.put(x.locals+x.c.view.cam_loc_offset,std::array<float,3>{std::numeric_limits<float>::quiet_NaN(),2,3});
        if(fault==1)x.m.put(x.address(x.c.view.cam_rot)+0xd0,x.address(x.c.vector_struct));
        if(fault==2)x.m.put(x.address(x.c.view.start_trace)+0x74,unsigned(16));
        if(fault==3){x.c.view.cam_loc_offset=x.c.impact_offset;x.m.put(x.address(x.c.view.cam_loc)+0x94,x.c.impact_offset);}
        if(fault==4)x.m.denied=x.locals+x.c.view.trace_dir_offset;
        assert(!snapshot(x.c,x.read(),x.address(x.c.controller),x.frame,out)&&same(out,saved));}
    std::cout<<"crosshair_assignment_test passed\n";
}
