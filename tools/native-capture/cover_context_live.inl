#ifndef GEARS_COVER_CONTEXT_LIVE_INCLUDED
#define GEARS_COVER_CONTEXT_LIVE_INCLUDED
// Read-only cover context on the existing admitted Scene GT. Include after
// weapon_hand_scene_binding. No CoverBody declaration, hook, pose, camera,
// controller reader, engine call, or mutation/restoration responsibility.
namespace cover_context_live {
namespace binding=weapon_hand_scene_binding;
struct Observation {
    binding::Binding actor{};
    gears_anchor::Identity link{};
    int slot{-1};unsigned char cover_type{},cover_action{},last_action{};
    bool transitioning{};
    uint64_t scene{};unsigned thread{};
    int64_t scene_qpc{},family_qpc{},renderer_qpc{};
};
bool same(Observation const& a,Observation const& b){return
    binding::same_key(a.actor,b.actor)&&a.actor.special_move==b.actor.special_move&&
    gears_anchor::equal_identity(a.link,b.link)&&a.slot==b.slot&&a.cover_type==b.cover_type&&
    a.cover_action==b.cover_action&&a.last_action==b.last_action&&a.transitioning==b.transitioning&&
    a.scene==b.scene&&a.thread==b.thread&&a.scene_qpc==b.scene_qpc&&
    a.family_qpc==b.family_qpc&&a.renderer_qpc==b.renderer_qpc;}
#if defined(GEARS_COVER_BODY_PROTOTYPE) && GEARS_COVER_BODY_PROTOTYPE
binding::Proof pawn_proof{},link_proof{};
std::array<binding::Property,6> fields{};
binding::Property link_world{},slots{};
unsigned transition_mask{};
uint64_t metadata_generation{};
std::array<unsigned char,sizeof(gears_anchor::Config)> metadata_bytes{};
struct Array {uintptr_t data{};int count{},capacity{};};
static_assert(sizeof(Array)==16);
struct Values {
    unsigned char type{},action{},last{};uint32_t transition{};
    uintptr_t link{},world{};int slot{-1};
    gears_anchor::Identity link_identity{};
    Array array{};std::array<unsigned char,212> selected{};
};
bool equal(Values const& a,Values const& b){return a.type==b.type&&a.action==b.action&&a.last==b.last&&
    a.transition==b.transition&&a.link==b.link&&a.world==b.world&&a.slot==b.slot&&
    gears_anchor::equal_identity(a.link_identity,b.link_identity)&&a.array.data==b.array.data&&
    a.array.count==b.array.count&&a.array.capacity==b.array.capacity&&a.selected==b.selected;}
bool pawn_layout(binding::Binding const& actor){
    if(metadata_generation!=binding::metadata_generation||metadata_bytes!=binding::metadata_bytes){
        pawn_proof={};link_proof={};fields={};link_world={};slots={};transition_mask=0;
        metadata_generation=binding::metadata_generation;metadata_bytes=binding::metadata_bytes;}
    gears_anchor::Identity cls{};if(!binding::identity(actor.pawn.cls,cls))return false;
    binding::keys(pawn_proof,cls);if(!binding::begin_prepare(pawn_proof))return false;
    if(pawn_proof.ready)return true;
    const wchar_t* names[]={L"CoverType",L"CurrentLink",L"CurrentSlotIdx",L"CoverAction",
                            L"LastCoverActionAnim",L"bDoingCoverActionAnimTransition"};
    const wchar_t* kinds[]={L"ByteProperty",L"ObjectProperty",L"IntProperty",L"ByteProperty",L"ByteProperty",L"BoolProperty"};
    const unsigned widths[]={1,8,4,1,1,4};
    for(unsigned i=0;i<fields.size();++i)
        if(!binding::property(pawn_proof,cls.address,names[i],kinds[i],widths[i],i==1?L"CoverLink":nullptr,fields[i]))return false;
    if(!binding::source_field(pawn_proof,transition_mask,fields[5].id.address,0xd0)||
       !transition_mask||(transition_mask&(transition_mask-1))||!binding::current(pawn_proof))return false;
    pawn_proof.ready=true;return true;
}
bool link_layout(gears_anchor::Identity const& link){
    gears_anchor::Identity cls{};binding::ClassPath path{};
    if(!binding::identity(link.cls,cls)||!binding::derives(link.cls,L"CoverLink",fields[1].reference.address,path))return false;
    binding::keys(link_proof,cls);if(!binding::begin_prepare(link_proof))return false;
    if(link_proof.ready)return true;
    if(!binding::property(link_proof,cls.address,L"WorldInfo",L"ObjectProperty",8,L"WorldInfo",link_world)||
       !binding::property(link_proof,cls.address,L"Slots",L"ArrayProperty",16,nullptr,slots))return false;
    uintptr_t inner{},structure{};unsigned dimension{},stride{},extent{},inner_offset{};
    gears_anchor::Identity inner_id{},slot_type{};
    if(!binding::source_field(link_proof,inner,slots.id.address,0xd0)||!binding::certified(link_proof,inner,inner_id)||
       inner_id.number||inner_id.outer!=slots.id.address||!binding::kind(link_proof,inner_id,L"StructProperty")||
       !binding::source_field(link_proof,dimension,inner,0x70)||dimension!=1||
       !binding::source_field(link_proof,stride,inner,0x74)||stride!=212||
       !binding::source_field(link_proof,inner_offset,inner,0x94)||inner_offset||
       !binding::source_field(link_proof,structure,inner,0xd0)||!binding::certified(link_proof,structure,slot_type)||
       !binding::named(slot_type,L"CoverSlot")||!binding::kind(link_proof,slot_type,L"ScriptStruct")||
       !binding::source_field(link_proof,extent,structure,0x88)||extent!=stride||!binding::current(link_proof))return false;
    link_proof.ready=true;return true;
}
bool values(binding::Binding const& actor,Values& out){
    Values v{};
    if(!binding::field(v.type,actor.pawn.address,fields[0].offset)||
       !binding::field(v.action,actor.pawn.address,fields[3].offset)||
       !binding::field(v.last,actor.pawn.address,fields[4].offset)||
       !binding::field(v.transition,actor.pawn.address,fields[5].offset))return false;
    if(v.type){
        if(!binding::field(v.link,actor.pawn.address,fields[1].offset)||
           !binding::field(v.slot,actor.pawn.address,fields[2].offset)||v.slot<0||
           !binding::identity(v.link,v.link_identity,true)||!link_layout(v.link_identity)||
           !binding::field(v.world,v.link,link_world.offset)||v.world!=actor.world.address||
           !binding::field(v.array,v.link,slots.offset)||!v.array.data||(v.array.data&3)||
           v.array.count<=0||v.array.count>256||v.array.capacity<v.array.count||v.array.capacity>256||
           v.slot>=v.array.count||v.array.data>UINTPTR_MAX-size_t(v.array.capacity)*v.selected.size()||
           !binding::read(v.selected,v.array.data+size_t(v.slot)*v.selected.size()))return false;
    }
    out=v;return true;
}
bool current_layout(){return binding::context_ready()&&binding::metadata_current()&&
    metadata_generation==binding::metadata_generation&&metadata_bytes==binding::metadata_bytes&&
    binding::current(pawn_proof);}
bool sample(gears_anchor::Receipt const* scene_receipt,Observation& out){
    try{
        if(!scene_receipt||!scene_receipt->invocation||scene_receipt->thread!=GetCurrentThreadId()||scene_receipt->scene_qpc<=0||
           scene_receipt->family_qpc<scene_receipt->scene_qpc||scene_receipt->renderer_qpc<scene_receipt->family_qpc)return false;
        std::array<unsigned char,sizeof(gears_anchor::Receipt)> receipt{};memcpy(receipt.data(),scene_receipt,receipt.size());
        binding::Binding actor{},again{};if(!binding::sample(scene_receipt,actor))return false;
        Values first{},last{};
        {
            std::lock_guard<std::mutex> lock(binding::mutex);
            if(!binding::context_ready()||!pawn_layout(actor)||!values(actor,first)||!current_layout()||
               (first.type&&!binding::current(link_proof))||!values(actor,last)||!equal(first,last))return false;
        }
        if(!binding::sample(scene_receipt,again)||!binding::same_key(actor,again)||actor.special_move!=again.special_move||
           actor.scene_invocation!=again.scene_invocation||memcmp(receipt.data(),scene_receipt,receipt.size()))return false;
        {
            std::lock_guard<std::mutex> lock(binding::mutex);
            if(!current_layout()||!values(again,last)||!equal(first,last)||
               (first.type&&!binding::current(link_proof))||!current_layout()||
               memcmp(receipt.data(),scene_receipt,receipt.size())||again.world_generation!=committed_world_generation.load())return false;
        }
        Observation result{};result.actor=again;result.link=first.link_identity;result.slot=first.slot;
        result.cover_type=first.type;result.cover_action=first.action;result.last_action=first.last;
        result.transitioning=(first.transition&transition_mask)!=0;result.scene=scene_receipt->invocation;result.thread=scene_receipt->thread;
        result.scene_qpc=scene_receipt->scene_qpc;result.family_qpc=scene_receipt->family_qpc;result.renderer_qpc=scene_receipt->renderer_qpc;
        out=result;return true;
    }catch(...){return false;}
}
#else
bool sample(gears_anchor::Receipt const*,Observation&){return false;}
#endif
} // namespace cover_context_live
#endif
