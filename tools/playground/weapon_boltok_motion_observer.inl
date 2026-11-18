// Included inside boltok_runtime. Read-only evidence for an already excessive
// consumer interval. These original poses never enter a Frame, stroke, lease,
// native command or inventory transaction.
namespace motion_observer {
namespace source=gears_hand_continuity;
struct Anchor {source::TrackedInput raw{};source::Publisher publisher{};hand::FrameKey key{};bool valid{};};
Anchor anchor{};
void capture(hand::Frame const& f,gears_rig_tracking::Frame const& rig) noexcept {
    anchor={};
    if(!(rig.flags&gears_rig_tracking::producer_continuity_available)||
       !source::same_input(rig.controllers,f.controllers)||!rig.controller_publisher_pid||
       !rig.controller_publisher_start)return;
    source::TrackedInput raw{rig.controllers,rig.producer.stream,{rig.producer.tracking[0],rig.producer.tracking[1]}};
    if(!source::tracked_input_shape_valid(raw))return;
    anchor={raw,{rig.controller_publisher_pid,rig.controller_publisher_start},f.key,true};
}
void observe_gap(hand::Frame const& f) noexcept {
    static unsigned rows{};
    if(!log_file||rows>=2||!moving||!cycle||!control.source.controllers.sync_ns||
       f.controllers.sync_ns<=control.source.controllers.sync_ns||
       f.controllers.sync_ns-control.source.controllers.sync_ns<=100000000)return;
    ++rows;const auto began=weaponhand::clock_ns(nullptr);
    source::TrackedInput current{};source::Publisher publisher{};source::MotionHistory history{};
    gears_current_tracking::Failure failure{};
    const bool matched=nativeequipment::read_input_continuity(f,current,publisher,nullptr,&failure,&history);
    const bool anchored=anchor.valid&&source::same_input(anchor.raw.input,control.source.controllers)&&
        hand::same(anchor.key,control.source.key)&&anchor.publisher.pid==publisher.pid&&anchor.publisher.start==publisher.start;
    unsigned hops{};bool chain=matched&&anchored&&source::same_input(current.input,f.controllers);
    auto previous=anchor.raw;
    if(chain){
        if(history.reserved||!history.count||history.count>source::motion_history_capacity||
           !source::same_tracking(history.frames[history.count-1],current))chain=false;
        for(unsigned i=0;chain&&i<history.count;++i){const auto& next=history.frames[i];
            if(next.input.sequence<previous.input.sequence)continue;
            if(next.input.sequence==previous.input.sequence){if(!source::same_tracking(previous,next))chain=false;continue;}
            if(!source::motion_pair(previous,next,0,100000000)||!source::motion_pair(previous,next,1,100000000)){chain=false;break;}
            ++hops;previous=next;
        }
        if(!hops||!source::same_tracking(previous,current))chain=false;
    }
    fwprintf(log_file,L"boltok_motion_observation frame=%llu sequence=%llu prior_sequence=%llu source_gap_ns=%lld matched=%u failure=%u anchored=%u both_hands_complete=%u hops=%u count=%u publisher=%llu,%llu previous_stream=%llu current_stream=%llu previous_tracking=%llu,%llu current_tracking=%llu,%llu elapsed_ns=%lld observation_only=1\n",
        f.key.frame,f.controllers.sequence,control.source.controllers.sequence,f.controllers.sync_ns-control.source.controllers.sync_ns,
        unsigned(matched),unsigned(failure),unsigned(anchored),unsigned(chain),hops,history.count,publisher.pid,publisher.start,
        anchor.raw.stream_epoch,current.stream_epoch,anchor.raw.tracking_epoch[0],anchor.raw.tracking_epoch[1],
        current.tracking_epoch[0],current.tracking_epoch[1],weaponhand::clock_ns(nullptr)-began);
    const auto point=[&](source::TrackedInput const& raw,unsigned index){const auto& l=raw.input.hands[0];const auto& r=raw.input.hands[1];
        fwprintf(log_file,L"boltok_motion_observation_point frame=%llu index=%u sequence=%llu sync_ns=%lld predicted_ns=%lld received_ns=%lld stream=%llu tracking=%llu,%llu left=%.9g,%.9g,%.9g left_q=%.9g,%.9g,%.9g,%.9g right=%.9g,%.9g,%.9g right_q=%.9g,%.9g,%.9g,%.9g press=%u release=%u squeeze=%.9g original_producer=1 motion_only=1\n",
            f.key.frame,index,raw.input.sequence,raw.input.sync_ns,raw.input.predicted_time,raw.input.received_ns,raw.stream_epoch,
            raw.tracking_epoch[0],raw.tracking_epoch[1],l.grip.position.x,l.grip.position.y,l.grip.position.z,
            l.grip.orientation.x,l.grip.orientation.y,l.grip.orientation.z,l.grip.orientation.w,
            r.grip.position.x,r.grip.position.y,r.grip.position.z,r.grip.orientation.x,r.grip.orientation.y,r.grip.orientation.z,r.grip.orientation.w,
            l.pressed[gears_vr::control_squeeze],l.released[gears_vr::control_squeeze],l.squeeze);};
    if(anchored)point(anchor.raw,0);
    for(unsigned i=0;i<history.count&&i<source::motion_history_capacity;++i)point(history.frames[i],i+1);
    fflush(log_file);
}
}
