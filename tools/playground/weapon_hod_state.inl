#include "../vr-common/weapon_hod_beam_audio.h"
// Read-only original HOD/Beam/audio state. Two independent typed discoveries
// must agree; absent/unreadable fields are unknown, never an assumed stop.
namespace hod_state {
namespace hand=gears_weapon_hand;namespace meta=gripdamage_metadata;
struct Observed {
    hand::ObjectIdentity manager{};std::uint64_t fingerprint{};
    bool manager_enabled{},input_down{},laser{},valid_emitter{},loop_playing{},
         beam_firing{},pending_beam{},target_live{},beam_audio{};
    bool aim_key{},toggle_targeting{},controller_aim{},pawn_aim{};
    unsigned owned_beams{};
    bool aim_stopped() const noexcept {return !aim_key&&!controller_aim&&!pawn_aim;}
    bool stopped() const noexcept {
        return !input_down&&!laser&&!valid_emitter&&!loop_playing&&
            !beam_firing&&!pending_beam&&!target_live&&!beam_audio;
    }
};
struct Read {
    meta::Reader& r;unified_weapon_fire::Fingerprint hash;
    bool retain(meta::Property const& p) noexcept {
        if(r.result.field_count==r.result.fields.size())return false;
        r.result.fields[r.result.field_count++]=p;return true;
    }
    template<std::size_t N>bool flag(uptr object,wchar_t const(&label)[N],bool& out) noexcept {
        weaponhand::BoolField value{};
        if(!weaponhand::boolean(r,object,label,value)||!retain(value.property))return false;
        out=(value.bits&value.mask)!=0;hash.word(object);hash.word(value.property.offset);
        hash.word(value.mask);hash.word(value.bits&value.mask);return true;
    }
    template<std::size_t N>bool pointer(uptr object,wchar_t const(&label)[N],uptr& out,bool component=false) noexcept {
        meta::PointerField value{};
        if(!weaponhand::read_pointer(r,object,label,value,component)||!retain(value.property))return false;
        out=value.value;hash.word(object);hash.word(value.property.offset);hash.word(out);return true;
    }
    bool playing(uptr audio,bool& out) noexcept {
        if(!audio){out=false;return true;}
        return weaponhand::derives(r,audio,L"AudioComponent",L"Engine")&&flag(audio,L"bWasPlaying",out);
    }
    bool particle(uptr component,bool& out) noexcept {
        if(!component){out=false;return true;}
        return weaponhand::derives(r,component,L"ParticleSystemComponent",L"Engine")&&flag(component,L"bIsActive",out);
    }
};
bool read_once(hand::Frame const& f,Observed& out) noexcept {
    out={};if(!weaponhand::current_gt(nullptr)||!hod_original::actors(f))return false;
    meta::Snapshot schema{};schema.module_base=base;meta::Reader r{schema,meta::Stage::final_check};
    if(!meta::tables(base,schema.objects,schema.names))return false;
    Read read{r};Observed value{};
    uptr gri{},manager{},laser{},valid{},sound{},particle{};
    bool targeting{},valid_flag{},laser_active{},valid_active{};
    if(!read.pointer(f.world.address,L"GRI",gri)||!gri||
       !weaponhand::derives(r,gri,L"WarGRI",L"WarfareGame")||!read.pointer(gri,L"HODBeamManager",manager))return false;
    // A typed, stable null manager is an original unavailable region, not an
    // ABI failure. Do not spawn/enable one. An old non-null owner still fails
    // its separate manager identity check and cannot stop a replacement.
    if(manager&&(!weaponhand::derives(r,manager,L"HammerOfDawnBeamManager",L"warfaregamecontentweapons")||
       !hod_original::identify(r,manager,value.manager)||!read.flag(manager,L"bEnabled",value.manager_enabled)))return false;
    if(!read.flag(f.controller.address,L"bPressingTargetingKey",value.aim_key)||
       !read.flag(f.controller.address,L"bToggleTargeting",value.toggle_targeting)||
       !read.flag(f.controller.address,L"bIsTargeting",value.controller_aim)||
       !read.flag(f.pawn.address,L"bIsTargeting",value.pawn_aim)||
       !read.flag(f.controller.address,L"bHoldingButton_R2",value.input_down)||
       !read.flag(f.weapon.address,L"bTargetingBeamFiring",targeting)||
       !read.flag(f.weapon.address,L"bValidTargetEmitterIsOn",valid_flag)||
       !read.pointer(f.weapon.address,L"TargetingBeamEmitter",laser,true)||!read.particle(laser,laser_active)||
       !read.pointer(f.weapon.address,L"ValidTargetEmitter",valid)||
       !read.pointer(f.weapon.address,L"TargetingBeamLoopSound",sound,true)||!read.playing(sound,value.loop_playing))return false;
    if(valid&&(!read.pointer(valid,L"ParticleSystemComponent",particle,true)||!read.particle(particle,valid_active)))return false;
    value.laser=targeting||laser_active;value.valid_emitter=valid_flag||valid_active;
    if(manager){
    weaponhand::BoolField array_field{};meta::Table beams{};uptr inner{};meta::Property element{};
    if(!weaponhand::field(r,manager,L"ActiveBeams",L"ArrayProperty",16,array_field)||!read.retain(array_field.property)||
       !r.at(manager,array_field.property.offset,beams)||beams.count<0||beams.count>64||
       beams.capacity<beams.count||beams.capacity>1024||(beams.count&&(!beams.data||beams.data>UINTPTR_MAX-uptr(beams.count)*8))||
       !r.at(schema.identities[array_field.property.identity].address,0xd0,inner)||!r.property(inner,element)||
       !weaponhand::core_type(r,element.type,L"ObjectProperty")||element.element_bytes!=8||element.dimension!=1||
       schema.identities[element.identity].outer!=schema.identities[array_field.property.identity].address||!read.retain(element))return false;
    read.hash.word(beams.data);read.hash.word(unsigned(beams.count));read.hash.word(unsigned(beams.capacity));
    for(int i=0;i<beams.count;++i){
        uptr beam{},pawn{},owner{};
        if(!copy(&beam,beams.data+uptr(i)*8,8))return false;read.hash.word(beam);
        if(!beam)continue;
        if(!weaponhand::derives(r,beam,L"HammerOfDawnBeam",L"warfaregamecontentweapons")||
           !read.pointer(beam,L"Instigator",pawn)||!read.pointer(beam,L"BeamManager",owner)||owner!=manager)return false;
        if(pawn!=f.pawn.address)continue;
        ++value.owned_beams;bool firing{},aborted{},dormant{},invalid{},beam_audio{};
        if(!read.flag(beam,L"bActuallyFiring",firing)||!read.flag(beam,L"bAborted",aborted)||
           !read.flag(beam,L"bDormant",dormant)||!read.flag(beam,L"bTargetIsInvalid",invalid)||
           !gears_weapon_hod_beam_audio::sample(read,beam,beam_audio))return false;
        value.beam_firing|=firing;value.pending_beam|=!aborted&&!dormant;
        value.target_live|=!invalid&&!dormant;value.beam_audio|=beam_audio;
    }
    }
    if(!unified_weapon_fire::retained_metadata(r)||!hod_original::actors(f))return false;
    unified_weapon_fire::schema_fingerprint(schema,read.hash);value.fingerprint=read.hash.value;out=value;return true;
}
bool read(hand::Frame const& source,Observed& out) noexcept {
    out={};Observed a{},b{};
    if(!read_once(source,a)||!read_once(source,b)||!a.fingerprint||a.fingerprint!=b.fingerprint||
       !hand::same(a.manager,b.manager))return false;
    out=b;return true;
}
} // namespace hod_state
