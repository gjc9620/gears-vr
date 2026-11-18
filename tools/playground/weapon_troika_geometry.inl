// Inside troika_entry_runtime. The callback only proposes observed geometry;
// no lease, C0 target, control rotation or firing publication is produced.
namespace mounted_geometry {
namespace wire=gears_troika_geometry;
std::uint64_t attempted_cookie{},attached_cookie{};
bool input_allowed(troika_mounted_context::Context const& c) noexcept {
    // Mounted gameplay gate, not ordinary observe_game with a forged Pawn.
    // IgnoreMoveInput can be original seated movement suppression; it is not
    // used as a rejection. Pause, disabled input/look and dead actors reject.
    try{namespace meta=weaponhand::meta;meta::Snapshot schema{};schema.module_base=base;
        meta::Reader reader{schema,meta::Stage::final_check};meta::PointerField pauser{};
        if(!meta::tables(base,schema.objects,schema.names)||!weaponhand::read_pointer(reader,c.world.address,L"Pauser",pauser)||pauser.value)return false;
        std::array<uptr,4> objects{c.controller.address,c.controller.address,c.driver.address,c.turret.address};
        std::array<unsigned,4> offsets{};std::array<int,4> values{};weaponhand::BoolField field{};
        for(unsigned i=0;i<4;++i){
            const bool found=i==0?weaponhand::field(reader,objects[i],L"InputIsDisabledCount",L"IntProperty",4,field):
                i==1?weaponhand::field(reader,objects[i],L"bIgnoreLookInput",L"ByteProperty",1,field):
                weaponhand::field(reader,objects[i],L"Health",L"IntProperty",4,field);
            if(!found)return false;
            offsets[i]=field.property.offset;
            if(i==1){unsigned char value{};if(!reader.at(objects[i],offsets[i],value))return false;values[i]=value;}
            else if(!reader.at(objects[i],offsets[i],values[i]))return false;
            if((i<2&&values[i]!=0)||(i>=2&&values[i]<=0))return false;
        }
        for(auto actor:{c.world.address,c.controller.address,c.driver.address,c.turret.address,c.weapon.address})
            if(!weaponhand::boolean(reader,actor,L"bDeleteMe",field)||(field.bits&field.mask))return false;
        if(!weaponhand::boolean(reader,c.controller.address,L"bCinematicMode",field)||(field.bits&field.mask)||
           !unified_weapon_fire::retained_metadata(reader))return false;
        uptr closing{};if(!reader.at(c.world.address,pauser.property.offset,closing)||closing)return false;
        for(unsigned i=0;i<4;++i){int value{};
            if(i==1){unsigned char byte{};if(!reader.at(objects[i],offsets[i],byte))return false;value=byte;}
            else if(!reader.at(objects[i],offsets[i],value))return false;
            if(value!=values[i])return false;
        }
        return !vrequip::model.switch_pending()&&!vrequip::model.animation_owns_hands();
    }catch(...){return false;}
}
std::uint32_t __cdecl prepare(void* context,wire::Request const* input,wire::Response* output) noexcept {
    if(!output)return unsigned(wire::Status::source_mismatch);*output={};
    try{
        if(context!=&weaponhand::client||!input||input->size!=sizeof(*input)||input->abi_version!=wire::version||
           !input->query_id||stop_requested.load(std::memory_order_acquire)||!owner.outstanding||
           owner.receipt.binding!=input->source.entry_binding||owner.receipt.token!=input->source.entry_token)return unsigned(wire::Status::source_mismatch);
        gears_troika_source::Frame actual{};
        if(!troika_tracking::acquire(owner,actual)||std::memcmp(&actual,&input->source.frame,sizeof actual))return unsigned(wire::Status::source_mismatch);
        troika_mounted_context::Context gameplay{};
        if(!troika_mounted_context::observe(owner,gameplay)||!input_allowed(gameplay))return unsigned(wire::Status::unavailable);
        gears_weapon::TroikaMountedMapping mapped{};troika_mounted_parts::Observation parts{};
        if(!troika_tracking::map(owner,actual,mapped)||!troika_mounted_parts::observe(owner,actual,mapped,parts))return unsigned(wire::Status::geometry_unavailable);
        wire::Response result{};result.query_id=input->query_id;result.publication_sequence=actual.publication_sequence;
        result.source_ticket=actual.source_ticket;result.entry_binding=input->source.entry_binding;result.entry_token=input->source.entry_token;
        result.component=parts.component;result.asset=parts.asset;result.tracked_mask=mapped.hands.tracked_mask;
        result.hand_handle[0]=parts.hand_handle[0];result.hand_handle[1]=parts.hand_handle[1];
        for(unsigned side=0;side<2;++side)if(result.tracked_mask&(1u<<side))for(unsigned handle=0;handle<2;++handle){
            gears_weapon::troika_handle_geometry::Contact hit{};
            if(!gears_weapon::troika_handle_geometry::contact(parts.bones[5],mapped.hands.world[side].position,handle,mapped.units_per_metre,hit))return unsigned(wire::Status::geometry_unavailable);
            // A shaft-height miss is known noncontact, encoded as finite far.
            result.distances[side][handle]=(std::min)(hit.distance_metres,10.);
            if(hit.touched)result.contacts[side]|=1u<<handle;
        }
        for(unsigned side=0;side<2;++side){
            auto const& hand=actual.controllers.hands[side];gears_vr::Pose press{};
            if(!hand.last_squeeze_press_sequence||hand.last_squeeze_press_sequence>actual.controllers.sequence||
               !gears_vr::controller_pose_usable(hand.last_squeeze_press_grip,hand.last_squeeze_press_flags,1))continue;
            if(!troika_tracking::physical_space.map_press(troika_tracking::space_owner,actual,side,weaponhand::clock_ns(nullptr),press))return unsigned(wire::Status::geometry_unavailable);
            result.press_tracked_mask|=1u<<side;
            for(unsigned handle=0;handle<2;++handle){gears_weapon::troika_handle_geometry::Contact hit{};
                if(!gears_weapon::troika_handle_geometry::contact(parts.bones[5],press.position,handle,mapped.units_per_metre,hit))return unsigned(wire::Status::geometry_unavailable);
                result.press_distances[side][handle]=(std::min)(hit.distance_metres,10.);
                if(hit.touched)result.press_contacts[side]|=1u<<handle;
            }
        }
        if(!wire::valid(result,*input)||!troika_tracking::source_current(actual)||!input_allowed(gameplay))return unsigned(wire::Status::source_mismatch);
        *output=result;return unsigned(wire::Status::ready);
    }catch(...){*output={};return unsigned(wire::Status::geometry_unavailable);}
}
namespace control=gears_troika_control;
bool assignment_current(troika_mounted_context::Context const& gameplay,wire::Response const& geometry) noexcept {
    try{namespace meta=weaponhand::meta;meta::Snapshot schema{};schema.module_base=base;
        meta::Reader reader{schema,meta::Stage::final_check};weaponhand::BoolField field{};
        if(!meta::tables(base,schema.objects,schema.names)||!troika_original::same_object(geometry.asset)||
           !troika_original::link(gameplay.turret_mesh.address,L"SkeletalMesh",geometry.asset.address))return false;
        for(unsigned side=0;side<2;++side){Name value{};meta::NameText text{};
            const bool known=side?weaponhand::field(reader,gameplay.turret.address,L"RightHandBoneHandleName",L"NameProperty",8,field):
                weaponhand::field(reader,gameplay.turret.address,L"LeftHandBoneHandleName",L"NameProperty",8,field);
            if(!known||!troika_reflection::retain(reader,field.property)||!reader.at(gameplay.turret.address,field.property.offset,value)||
               value.number||!meta::fresh_name(schema.names,value.index,text)||
               !(geometry.hand_handle[side]?meta::exact_text(text,L"b_troika_handle_R"):meta::exact_text(text,L"b_troika_handle_L")))return false;
        }
        return unified_weapon_fire::retained_metadata(reader);
    }catch(...){return false;}
}
std::uint32_t __cdecl commit(void* context,wire::Request const* request,wire::Response const* geometry,control::Decision const* decision) noexcept {
    // Acknowledgement only. Native complete must succeed after this callback;
    // PG never treats this candidate Decision as a firing or C0 permission.
    troika_mounted_context::Context gameplay{};
    if(context!=&weaponhand::client||!request||!geometry||!decision||stop_requested.load(std::memory_order_acquire)||
       !owner.outstanding||owner.receipt.binding!=request->source.entry_binding||owner.receipt.token!=request->source.entry_token||
       !wire::valid(*geometry,*request)||!control::valid(*decision,*request,weaponhand::clock_ns(nullptr))||
       !troika_tracking::source_current(request->source.frame)||!troika_mounted_context::observe(owner,gameplay)||
       !input_allowed(gameplay)||!assignment_current(gameplay,*geometry))return unsigned(control::CommitReply::rejected);
    return unsigned(control::CommitReply::accepted);
}
hand::Reply invoke(control::Attach fn,control::Registration const* r,hand::RegistrationResult* result) noexcept {
    __try{return fn(r,result);}
    __except(GetExceptionCode()==0xe06d7363ul||GetExceptionCode()==EXCEPTION_STACK_OVERFLOW?
        EXCEPTION_CONTINUE_SEARCH:EXCEPTION_EXECUTE_HANDLER){handrig::admission.quarantine();return hand::Reply::unavailable;}
}
bool attach() noexcept {
    const auto cookie=weaponhand::client.cookie();
    if(!cookie||weaponhand::client.phase()!=hand::ClientPhase::active||!weaponhand::current_gt(nullptr))return false;
    if(attached_cookie==cookie)return true;if(attempted_cookie==cookie)return false;
    if(handrig::reading||!weaponhand::native_module)return false;
    handrig::lifetime::Admission::Entry entry{};if(!handrig::admission.acquire(entry))return false;
    struct Local {handrig::lifetime::Admission::Entry& entry;~Local(){handrig::reading=false;handrig::admission.release(entry);}} local{entry};
    handrig::reading=true;const auto* lifetime=handrig::admission.view(entry);
    if(!lifetime||lifetime->context!=&weaponhand::client||lifetime->cookie!=cookie||
       lifetime->native_started_qpc!=weaponhand::client.native_started_qpc())return false;
    attempted_cookie=cookie;
    const auto address=GetProcAddress(weaponhand::native_module,control::attach_export);
    if(!weaponhand::executable(address,weaponhand::native_module))return false;
    control::Registration r{};r.geometry.context=&weaponhand::client;r.geometry.hand_cookie=cookie;r.geometry.native_started_qpc=lifetime->native_started_qpc;r.geometry.prepare=prepare;r.commit=commit;
    hand::RegistrationResult result{};const auto reply=invoke(reinterpret_cast<control::Attach>(address),&r,&result);
    if(reply==hand::Reply::accepted&&result.cookie==cookie&&result.native_started_qpc==r.geometry.native_started_qpc&&
       weaponhand::client.phase()==hand::ClientPhase::active&&weaponhand::client.cookie()==cookie)attached_cookie=cookie;
    return attached_cookie==cookie;
}
} // namespace mounted_geometry
