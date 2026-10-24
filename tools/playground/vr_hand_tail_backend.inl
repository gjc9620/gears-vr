// Included inside handstream, only for the fixed-instance tail experiment.
bool Backend::tail_forward_allowed()const {
    return forward_write&&!trial.lease.restoring()&&!trial.lease.primary_faulted()&&
        !trial.lease.faulted()&&!hand_writes_relinquished&&forward_allowed();
}
bool Backend::tail_wanted(unsigned channels,std::array<policy::Target,4> const& targets,
                          std::array<gears_hand_tail_translation::Value,2>& wanted){
    const unsigned expected=((tail_mask&1u)?12u:0u)|((tail_mask&2u)?3u:0u);
    if(!tail_forward_allowed()||!channels||channels!=expected||targets!=trial.targets)return false;
    const unsigned selected=channels==3?1u:channels==12?2u:channels==15?3u:0u;
    if(!selected)return false;
    trial.tail_writes_at_begin=trial.tail_field_writes;
    for(unsigned hand=0;hand<2;++hand)if(selected&(1u<<hand)){
        const unsigned channel=hand*2u+1u,index=indices[channel];
        handik::Control current{};
        if(!control(channel,current)||!handtail::same_schema(current.translation,trial.lease_anchor.controls[index].translation)||
           !gears_hand_tail_translation::exact_zero(current.strength)||
           !gears_hand_tail_translation::exact(current.translation.value,trial.lease_anchor.controls[index].translation.value))return false;
        std::array<float,3> position{};std::memcpy(position.data(),targets[hand*2u].data(),12);
        for(float value:position)if(!std::isfinite(value)||std::abs(value)>1e7f)return false;
        // World/None/non-additive were checked by the original-value reader.
        // Only the live apply bit and absolute target bytes are changed.
        wanted[hand]=current.translation.value;
        wanted[hand].translation=targets[hand*2u];
        wanted[hand].flags=current.translation.apply_mask;
    }
    return tail_forward_allowed();
}
bool Backend::tail_read(unsigned hand,gears_hand_tail_translation::Value& value,
                        gears_hand_tail_translation::Strength& strength){
    if(hand>=2||(trial.tail_revoked_mask&(1u<<hand)))return false;
    handik::Control current{};
    if(!control(hand*2u+1u,current))return false;
    value=current.translation.value;strength=current.strength;return true;
}
bool Backend::tail_write_field(unsigned hand,gears_hand_tail_translation::Field field,
                               gears_hand_tail_translation::FieldValue const& expected,
                               gears_hand_tail_translation::FieldValue const& wanted){
    using Field=gears_hand_tail_translation::Field;
    using gears_hand_tail_translation::field_value;
    const unsigned field_index=unsigned(field);
    const auto admitted=[&](){return trial.lease.restoring()?forward_allowed():tail_forward_allowed();};
    if(hand>=2||field_index>=unsigned(Field::count)||hand_writes_relinquished||
       (trial.tail_revoked_mask&(1u<<hand))||!admitted())return false;
    auto const& journal=trial.lease.translation();
    const auto saved_value=journal.original(hand),owned=journal.owned(hand);
    if(!saved_value||!owned||!(journal.pending_fields(hand)&(1u<<field_index))||
       (wanted!=field_value(*saved_value,field)&&wanted!=field_value(*owned,field)))return false;
    handik::Control current{};
    if(!control(hand*2u+1u,current)||!gears_hand_tail_translation::exact_zero(current.strength)||
       !handtail::same_schema(current.translation,trial.lease_anchor.controls[indices[hand*2u+1u]].translation)||
       field_value(current.translation.value,field)!=expected)return false;
    if(field==Field::flags){
        std::uint32_t value{};std::memcpy(&value,wanted.data(),sizeof(value));
        if(value&~(current.translation.apply_mask|current.translation.add_mask))return false;
    }
    if(!admitted()||hand_writes_relinquished)return false;
    count_add(trial.tail_field_writes);
    return gears_hand_tail_fields::compare_write(current.translation.addresses,current.translation.strength_addresses,
        current.translation.apply_mask,current.translation.add_mask,field,expected,wanted);
}
