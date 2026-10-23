#include "crosshair_assignment_test_fixture.h"
static bool same(Snapshot const& a,Snapshot const& b){return a.self==b.self&&a.frame==b.frame&&a.node==b.node&&a.locals==b.locals&&
    a.pawn==b.pawn&&a.weapon==b.weapon&&a.destination==b.destination&&a.source==b.source&&a.script==b.script&&
    a.source_bytes==b.source_bytes&&a.previous_cache==b.previous_cache&&gears_crosshair_view::equal(a.view,b.view);}
static bool same(CompletedAssignmentReceipt const& a,CompletedAssignmentReceipt const& b){return a.controller==b.controller&&
    a.pawn==b.pawn&&a.weapon==b.weapon&&a.writer==b.writer&&a.frame==b.frame&&a.locals==b.locals&&a.destination==b.destination&&
    a.cache_bytes==b.cache_bytes&&a.point==b.point&&gears_crosshair_view::equal(a.view,b.view);}
int main(){
    // Activation diagnostics retain the original short-circuit validation order
    // and report the first failed proof, including read failures at each stage.
    for(unsigned fault=0;fault<7;++fault){Fixture x;ValidationFailure failure=ValidationFailure::bindings;
        if(fault==1)x.m.put(0x10000+x.c.lhs*8,std::uintptr_t(0));
        if(fault==2)x.m.denied=x.identities.names_header;
        if(fault==3)x.m.put(x.address(x.c.lhs)+0x94,unsigned(0x1368));
        if(fault==4)x.m.denied=x.address(x.c.writer)+0x114;
        if(fault==5)x.m.put(x.identities.base+0x1b86280,std::uintptr_t(0));
        if(fault==6)x.m.denied=x.identities.base+0x1d7ad0;
        const auto expected=fault==0?ValidationFailure::none:fault<=2?ValidationFailure::certificate:
            fault<=4?ValidationFailure::schema:ValidationFailure::bindings;
        assert(validate_bindings(x.c,x.read(),true,&failure)==(fault==0));assert(failure==expected);
        assert(validate_bindings(x.c,x.read())==(fault==0));
        assert(std::strcmp(validation_failure_name(failure),fault==0?"none":fault<=2?"certificate":fault<=4?"schema":"bindings")==0);
    }
    Fixture precedence;precedence.m.denied=precedence.identities.names_header;
    precedence.m.put(precedence.address(precedence.c.lhs)+0x94,unsigned(0));
    ValidationFailure first{};assert(!validate_bindings(precedence.c,precedence.read(),true,&first)&&first==ValidationFailure::certificate);
    Fixture detoured;detoured.m.put(detoured.identities.base+0x1d7ad0,std::uint8_t(0xe9));
    assert(!validate_bindings(detoured.c,detoured.read(),true,&first)&&first==ValidationFailure::bindings);
    assert(validate_bindings(detoured.c,detoured.read(),false,&first)&&first==ValidationFailure::none);
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
    Fixture ads;ads.enable_aim();Snapshot ads_before{};
    assert(snapshot(ads.c,ads.read(),ads.address(ads.c.controller),ads.frame,ads_before));
    assert(ads_before.view.aiming_known&&ads_before.view.aiming);
    ads.simulate_original();CompletedAssignmentReceipt ads_out{};
    assert(completed(ads.c,ads.read(),ads_before,ads_out)&&ads_out.view.aiming);
    ads.m.put(ads.address(ads.c.controller)+ads.c.aim.offset,unsigned(0));
    assert(!completed(ads.c,ads.read(),ads_before,ads_out)); // Changed state cannot borrow the prior receipt.
    for(unsigned fault=0;fault<4;++fault){Fixture a;a.enable_aim();
        if(fault==0)a.m.put(a.address(a.c.aim.property)+0xd0,unsigned(3));
        if(fault==1)a.m.put(a.address(a.c.aim.property)+0x74,unsigned(1));
        if(fault==2)a.m.put(0xe02a0,a.identities.base+0x1daba1);
        if(fault==3)a.m.denied=a.address(a.c.controller)+a.c.aim.offset;
        Snapshot unused{};assert(!snapshot(a.c,a.read(),a.address(a.c.controller),a.frame,unused));}
    // Scene targeting remains readable when a HUD action skipped the query or
    // the query's frame/locals are unavailable. Cover and ordinary ADS share
    // the game-owned targeting bit; releasing it must apply immediately.
    Fixture cover;cover.enable_aim();cover.m.denied=cover.frame+0x30;bool raised=false;
    assert(scene_aiming(cover.c,cover.read(),cover.address(cover.c.controller),cover.address(cover.c.pawn),raised)&&raised);
    cover.m.put(cover.address(cover.c.controller)+cover.c.aim.offset,unsigned(0));
    assert(scene_aiming(cover.c,cover.read(),cover.address(cover.c.controller),cover.address(cover.c.pawn),raised)&&!raised);
    Fixture swapped;swapped.enable_aim();swapped.m.denied=swapped.address(swapped.c.weapon);
    swapped.m.put(swapped.address(swapped.c.pawn)+swapped.c.pawn_weapon.offset,std::uintptr_t(0xbad000));
    assert(scene_aiming(swapped.c,swapped.read(),swapped.address(swapped.c.controller),swapped.address(swapped.c.pawn),raised)&&raised);
    for(unsigned fault=0;fault<8;++fault){Fixture a;a.enable_aim();bool unchanged=true;unsigned reads=0;
        if(fault==0)a.c.aim.enabled=false;
        if(fault==1)a.m.denied=a.address(a.c.controller)+a.c.aim.offset;
        if(fault==2)a.m.put(a.address(a.c.aim.property)+0xd0,unsigned(3));
        if(fault==3)a.m.put(a.address(a.c.controller)+a.c.controller_pawn.offset,std::uintptr_t(0));
        if(fault==4)a.m.put(0x10000+a.c.pawn*8,std::uintptr_t(0));
        auto changing=[&](void* dest,void const* source,std::size_t size){
            if(fault==7&&reinterpret_cast<std::uintptr_t>(source)==a.address(a.c.controller)+a.c.aim.offset&&++reads==2)
                a.m.put(a.address(a.c.controller)+a.c.aim.offset,unsigned(0));
            return a.m.read(dest,source,size);};
        assert(!scene_aiming(a.c,changing,a.address(a.c.controller)+(fault==5?8:0),
            a.address(a.c.pawn)+(fault==6?8:0),unchanged)&&unchanged);
    }
    std::cout<<"crosshair_assignment_test passed\n";
}
