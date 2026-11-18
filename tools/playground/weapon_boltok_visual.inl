// Rigid cylinder/display rounds share one mesh. Held ammunition remains separate.
// Geometry and presentation masks never grant ammunition or hand permissions.
#include "../vr-common/weapon_boltok_compound_geometry.h"
#include "../vr-common/weapon_boltok_cylinder_grasp.h"
#include "../vr-common/weapon_boltok_object02_input.h"
namespace boltok_visual {
namespace hand=gears_weapon_hand;namespace asset=weapon_part_assets;
namespace geom=gears_weapon::part_geometry;namespace inv=lancerreload_inventory;
namespace compound=gears_boltok_compound_geometry;
namespace original=gears_weapon::original_part_input;
std::array<asset::Handle,3> handles{}; // Active, pending/retired, held item.
std::uint64_t generation{},next_generation{1},last_work_frame{};
inv::Identity identity{};gears_weapon::BoltokVisualGeometry geometry{};
geom::Prepared cylinder_source{},round_source{};
bool ready{},source_loaded{},active_warm{},pending_warm{},retired{},restoring{};
unsigned failure{},active_mask{63},wanted_mask{63},pending_mask{63};
bool withdraw() noexcept {
    bool okay=true;for(auto h:handles)if(h.generation){const auto r=asset::find(h);
        if(r&&(r->stage==asset::Stage::ready||r->stage==asset::Stage::visible))
            okay=asset::set_visible(h,generation,false)&&okay;}return okay;
}
bool restore() noexcept {
    ready=false;restoring=true;bool okay=withdraw();
    for(auto& h:handles)if(h.generation){
        if(asset::find(h)&&!asset::destroy(h,generation)){okay=false;continue;}h={};
    }
    if(!okay)return false;
    generation=0;identity={};geometry={};cylinder_source={};round_source={};
    source_loaded=active_warm=pending_warm=retired=restoring=false;
    active_mask=wanted_mask=pending_mask=63;last_work_frame=0;return true;
}
bool lighting(hand::Frame const& f,asset::Handle h) noexcept {
    namespace meta=gripdamage_metadata;meta::Snapshot schema{};schema.module_base=base;
    meta::Reader r{schema,meta::Stage::final_check};meta::PointerField p{};hand::ObjectIdentity light{};
    const auto component=nativelancerfollow::bound_key.component;
    if(!meta::tables(base,schema.objects,schema.names)||!weaponhand::object(r,component)||
       !weaponhand::read_pointer(r,component.address,L"LightEnvironment",p,true)||
       (p.value&&(!lancermain_asset::identify(r,p.value,light)||!playground::instance(p.value,L"LightEnvironmentComponent")))||
       !unified_weapon_fire::retained_metadata(r)||!nativeequipment::current(nullptr,f))return false;
    return asset::bind_light_environment(f,h,generation,light,!p.value);
}
// Current original part geometry only; no input, transform or action witness is cached.
struct ShowCost {
    hand::Frame const& frame;asset::SourceCost before{asset::source_cost};
    std::int64_t began{weaponhand::clock_ns(nullptr)},ended{},withdraw_ns{},flush_ns{};
    std::array<std::int64_t,7> stage_ns{},ack_ns{};std::int64_t ack_batch_ns{};unsigned ack_read_only{},phase{},index{},asset_gate{};bool passed{};
    unsigned selected{},hidden_rounds{};
    unsigned validation_calls{},validation_fallbacks{},validation_ranges{},validation_bytes{},validation_sealed{},validation_disabled{};
    unsigned validation_failure{},validation_reads{};std::uintptr_t validation_failure_address{};std::size_t validation_failure_bytes{};
    unsigned validation_batched{},validation_batches{},validation_padding{},validation_span{};
    unsigned validation_calls_before{};
    explicit ShowCost(hand::Frame const& f) noexcept:frame(f){
        if(auto const* scope=weaponhand::source_validation::active)if(scope->matches(f))validation_calls_before=scope->validations;
    }
    void finish(bool okay,unsigned where,unsigned part) noexcept {
        ended=weaponhand::clock_ns(nullptr);passed=okay;phase=where;index=part;asset_gate=asset::failure;
        if(auto const* scope=weaponhand::source_validation::active)if(scope->matches(frame)){
            validation_calls=scope->validations;validation_fallbacks=scope->fallbacks;
            validation_sealed=unsigned(scope->sealed);validation_disabled=unsigned(scope->disabled);
            if(scope->proof){validation_ranges=scope->proof->merged;validation_bytes=scope->proof->used;
                validation_failure=unsigned(scope->proof->failure);validation_reads=scope->proof->count;
                validation_failure_address=scope->proof->failure_address;validation_failure_bytes=scope->proof->failure_bytes;
                validation_batched=unsigned(scope->proof->batched);validation_batches=scope->proof->batch_count;
                validation_padding=scope->proof->padding_bytes;validation_span=scope->proof->planned_bytes;}
        }
    }
    ~ShowCost() noexcept {
        if(!ended)ended=weaponhand::clock_ns(nullptr);
        if(!log_file)return;
        static unsigned slow_rows{},failed_rows{},late_rows{};
        const bool late=frame.controllers.sync_ns&&ended-frame.controllers.sync_ns>=80000000;
        bool log_row{};
        if(passed){if(slow_rows++<8)log_row=true;}
        else if(failed_rows++<8)log_row=true;
        if(!log_row&&late&&late_rows++<3)log_row=true;
        if(!log_row)return;
        fwprintf(log_file,L"boltok_show_cost frame=%llu sequence=%llu sync_ns=%lld began_ns=%lld ended_ns=%lld passed=%u phase=%u index=%u asset=%u selected=%u hidden_rounds=%u begin_age_ns=%lld end_age_ns=%lld elapsed_ns=%lld withdraw_ns=%lld source_calls=%llu source_ns=%lld stage_ns=%lld,%lld,%lld,%lld,%lld,%lld,%lld flush_ns=%lld ack_batch_ns=%lld ack_read_only=%u ack_ns=%lld,%lld,%lld,%lld,%lld,%lld,%lld\n",
            frame.key.frame,frame.controllers.sequence,frame.controllers.sync_ns,began,ended,unsigned(passed),phase,index,asset_gate,
            selected,hidden_rounds,
            began-frame.controllers.sync_ns,ended-frame.controllers.sync_ns,ended-began,withdraw_ns,
            asset::source_cost.calls-before.calls,asset::source_cost.ns-before.ns,
            stage_ns[0],stage_ns[1],stage_ns[2],stage_ns[3],stage_ns[4],stage_ns[5],stage_ns[6],flush_ns,ack_batch_ns,ack_read_only,
            ack_ns[0],ack_ns[1],ack_ns[2],ack_ns[3],ack_ns[4],ack_ns[5],ack_ns[6]);fflush(log_file);
        fwprintf(log_file,L"boltok_source_validation frame=%llu sequence=%llu calls=%u calls_total=%u fallbacks=%u ranges=%u bytes=%u sealed=%u disabled=%u failure=%u fail_address=%llx fail_bytes=%llu reads=%u batched=%u batches=%u padding=%u span=%u\n",
            frame.key.frame,frame.controllers.sequence,validation_calls-validation_calls_before,validation_calls,validation_fallbacks,validation_ranges,validation_bytes,validation_sealed,validation_disabled,
            validation_failure,static_cast<unsigned long long>(validation_failure_address),static_cast<unsigned long long>(validation_failure_bytes),validation_reads,
            validation_batched,validation_batches,validation_padding,validation_span);fflush(log_file);
    }
};
bool current_face(boltok_parts::Mechanical const& parts,gears_vr::Pose& face) noexcept {
    const auto& root=parts.root_metres;const auto& local=parts.in_root[1];
    const gears_vr::Pose bone{
        gears_weapon::scaled(gears_weapon::sum(root.position,gears_weapon::rotate(root.orientation,local.position)),parts.units),
        gears_vr::normalized(gears_vr::multiply(root.orientation,local.orientation))};
    return geometry.cylinder(bone,face);

}
bool draw(hand::Frame const& f,asset::Handle h,gears_vr::Pose const& face,
    bool visible,bool preparation,ShowCost* cost=nullptr) noexcept {
    std::array<asset::WeaponPoseRequest,1> requests{};
    auto began=weaponhand::clock_ns(nullptr);
    const bool staged=asset::stage_weapon_pose(f,h,generation,
        {face.position,gears_vr::ue_angles(gears_vr::rotation_matrix(face.orientation))},
        {1,1,1},visible,requests[0],preparation);
    if(cost)cost->stage_ns[0]=weaponhand::clock_ns(nullptr)-began;
    if(!staged){if(cost)cost->finish(false,3,0);return false;}
    began=weaponhand::clock_ns(nullptr);
    const bool flushed=asset::flush_weapon_poses(f,requests);
    if(cost)cost->flush_ns=weaponhand::clock_ns(nullptr)-began;
    if(!flushed){if(cost)cost->finish(false,4,0);return false;}
    began=weaponhand::clock_ns(nullptr);
    const bool ack=asset::acknowledge_weapon_pose(f,requests[0]);
    if(cost)cost->ack_ns[0]=weaponhand::clock_ns(nullptr)-began;
    if(!ack&&cost)cost->finish(false,5,0);
    return ack;
}
bool complete(asset::Handle h) noexcept {
    const auto r=asset::find(h);return r&&r->lighting_bound&&
        (r->stage==asset::Stage::ready||r->stage==asset::Stage::visible);
}
bool load_original_object02(geom::Conversion const& conversion,geom::Prepared& out) noexcept {
    const auto& definition=original::boltok_object02;
    std::wstring folder{};std::vector<std::uint8_t> vertices{},indices{};
    if(!weapon_part_inputs::folder(folder,original::boltok_object02_cache)||
       !weapon_part_inputs::read(folder+L"/"+definition.vertex_file,
           definition.vertices*sizeof(geom::Vertex),vertices)||
       !weapon_part_inputs::read(folder+L"/"+definition.index_file,
           definition.indices*sizeof(std::uint16_t),indices)||
       !weapon_part_inputs::hash(vertices,definition.vertex_hash)||
       !weapon_part_inputs::hash(indices,definition.index_hash))return false;
    const geom::Input input{vertices.data(),vertices.size(),indices.data(),indices.size(),
        definition.sections,definition.section_count};
    return geom::prepare(input,conversion,out)==geom::Result::ready;
}
bool load_sources(hand::Frame const& f) noexcept {
    if(source_loaded)return true;
    const geom::Conversion authored_conversion{{{{1,0,0},{0,0,1},{0,-1,0}}},
        gears_weapon::boltok_authoring_raw_per_metre*geometry.scale,f.reference_generation,true};
    const geom::Conversion original_conversion{{{{1,0,0},{0,1,0},{0,0,1}}},
        gears_weapon::boltok_authoring_raw_per_metre*geometry.scale,f.reference_generation,true};
    if(!load_original_object02(original_conversion,cylinder_source)||
       weapon_part_inputs::load("boltok-round-lod0",authored_conversion,round_source)!=weapon_part_inputs::Result::ready)return false;
    source_loaded=true;return true;
}
// One construction/advance/light/warm boundary per fresh GT frame. Cached
// geometry is immutable CPU data; every Engine operation authenticates f anew.
bool service(hand::Frame const& f,unsigned index,unsigned mask) {
    if(last_work_frame==f.key.frame)return true;
    auto& h=handles[index];const auto r=asset::find(h);
    if(h.generation&&!r){h={};if(index==0)active_warm=false;else if(index==1)pending_warm=false;return true;}
    if(complete(h))return true;
    last_work_frame=f.key.frame;
    if(!h.generation){
        failure=2;if(!load_sources(f))return false;
        geom::Prepared data{};
        if(index!=2&&compound::build(cylinder_source,round_source,geometry,mask,data)!=geom::Result::ready)return false;
        hand::ObjectIdentity material{};failure=3;if(!asset::held_weapon_material(f,material))return false;
        const std::vector<hand::ObjectIdentity> materials(index==2?1:3,material);failure=4;
        const auto c=index==2?asset::create(f,generation,cylinder_source,materials,h):
            asset::create_weapon_part(f,generation,data,materials,h);
        return c!=asset::Creation::rejected&&nativeequipment::current(nullptr,f);
    }
    failure=6;
    if(r->stage!=asset::Stage::ready&&r->stage!=asset::Stage::visible)
        return asset::advance(f,h,generation)!=asset::Creation::rejected&&nativeequipment::current(nullptr,f);
    failure=7;return lighting(f,h)&&nativeequipment::current(nullptr,f);
}
bool prepare(hand::Frame const& f,boltok_parts::Mechanical const& parts) noexcept try {
    failure=1;
    if(restoring)return restore();
    if(generation&&(!(identity==inv::identity(f))||geometry.calibration!=f.reference_generation||
       std::abs(geometry.scale-parts.component_scale)>1e-5||geometry.units!=parts.units))return restore();
    if(!generation){
        if(!next_generation||next_generation==UINT64_MAX)return false;
        generation=next_generation++;identity=inv::identity(f);
        geometry={parts.component_scale,parts.units,f.reference_generation};
        if(!geometry.valid())return false;
    }
    ready=false;
    if(!complete(handles[0])){if(!service(f,0,active_mask))return false;return false;}
    if(!complete(handles[2])){if(!service(f,2,0))return false;return false;}
    if(!active_warm){
        if(last_work_frame==f.key.frame)return false;
        last_work_frame=f.key.frame;gears_vr::Pose face{};failure=9;
        if(!current_face(parts,face)||!draw(f,handles[0],face,false,true))return false;
        active_warm=true;return false;
    }
    ready=true;failure=0;return true;
}catch(...){failure=8;ready=false;return false;}
bool show(hand::Frame const& f,boltok_parts::Mechanical const& parts,unsigned live_mask,
    bool original_hidden,bool inserted_ready=false) noexcept try {
    if(!ready||live_mask>63||!original_hidden||!(identity==inv::identity(f))||
       geometry.calibration!=f.reference_generation)return withdraw();
    ShowCost cost(f);cost.selected=1;
    for(unsigned bits=live_mask;bits;bits>>=1)cost.hidden_rounds+=bits&1u;
    cost.hidden_rounds=6-cost.hidden_rounds;
    gears_vr::Pose face{};if(!current_face(parts,face)){cost.finish(false,1,0);return false;}
    // Old compound retires at the open endpoint. Only the committed insertion
    // promotes the whole replacement cylinder.
    if(!inserted_ready){
        bool hidden=asset::set_visible(handles[0],generation,false);
        if(handles[1].generation)hidden=asset::set_visible(handles[1],generation,false)&&hidden;
        cost.finish(hidden,2,0);return hidden;
    }
    if(inserted_ready&&!draw(f,handles[0],face,true,false,&cost)){withdraw();return false;}
    wanted_mask=live_mask;
    // A retired buffer is still owned until destroy succeeds or find confirms
    // its exact generation has already been emptied by the normal drain.
    if(handles[1].generation&&(retired||pending_mask!=wanted_mask)){
        if(last_work_frame!=f.key.frame){
            last_work_frame=f.key.frame;
            if(asset::find(handles[1])){
                if(!asset::set_visible(handles[1],generation,false)){cost.finish(false,6,1);return false;}
                if(!asset::destroy(handles[1],generation)){
                    retired=true;
                    if(!nativeequipment::current(nullptr,f)){cost.finish(false,6,1);return false;}
                    cost.finish(true,0,0);return true;
                }
            }
            handles[1]={};retired=pending_warm=false;
        }
    }
    if(!retired&&wanted_mask!=active_mask&&(!handles[1].generation||pending_mask==wanted_mask)){
        if(!handles[1].generation){pending_mask=wanted_mask;pending_warm=false;}
        if(!complete(handles[1])){
            if(!service(f,1,pending_mask)){cost.finish(false,7,1);return false;}
        }else if(last_work_frame!=f.key.frame){
            last_work_frame=f.key.frame;
            if(!pending_warm){
                if(!draw(f,handles[1],face,false,true)){cost.finish(false,8,1);return false;}
                pending_warm=true;
            }else{
                if(!draw(f,handles[1],face,true,false)||!asset::set_visible(handles[0],generation,false)){
                    cost.finish(false,9,1);return false;
                }
                std::swap(handles[0],handles[1]);active_mask=pending_mask;retired=true;pending_warm=false;
                if(log_file){fwprintf(log_file,L"boltok_compound_promoted frame=%llu sequence=%llu applied_mask=%u wanted_mask=%u cleanup_debt=1\n",
                    f.key.frame,f.controllers.sequence,active_mask,wanted_mask);fflush(log_file);}
            }
        }
    }
    if(!nativeequipment::current(nullptr,f)){cost.finish(false,10,0);return false;}
    cost.finish(true,0,0);return true;
}catch(...){failure=8;withdraw();return false;}
bool held(hand::Frame const& f,hand::Decision const& d,gears_vr::Pose const& pose) noexcept {
    if(!ready||!(identity==inv::identity(f)))return false;
    gears_vr::Pose mount{};
    if(!gears_boltok_cylinder_grasp::item_in_grip(geometry.physical_scale(),mount))return false;
    mount.position=gears_weapon::scaled(mount.position,geometry.units);
    const auto item=gears_boltok_cylinder_grasp::compose(pose,mount);
    return asset::set_pose(f,handles[2],generation,d,
        {item.position,gears_vr::ue_angles(gears_vr::rotation_matrix(item.orientation))},true);
}
bool whole_socket(gears_weapon::FeedPortMount& out) noexcept {
    if(!ready||!geometry.valid())return false;
    gears_weapon::FeedPortMount candidate{};
    if(!gears_boltok_cylinder_grasp::item_in_grip(geometry.physical_scale(),candidate.item_in_left_grip))return false;
    candidate.outward_axis_in_weapon={0,-1,0};
    // Captured Volume/Snap remain acquisition/visual targets. Final physical
    // seating uses the existing bounded insertion contract, never six holes.
    if(!candidate.valid())return false;
    out=candidate;return true;
}
bool hide_held() noexcept {return !handles[2].generation||!asset::find(handles[2])||
    asset::set_visible(handles[2],generation,false);}
}
