#include "cover_measurement_core_observation.h"
#include "cover_measurement_core_fixture.h"

int main(){
    namespace observation=gears_cover_measurement_core;
    constexpr std::uint64_t limit=50'000'000;
    auto source=core(frame(1));
    const auto inspect=[&](nr::CoreSnapshot const& value){
        const auto saved=bytes(value);auto w=observation::observe(value,now,limit,true);
        CHECK(bytes(value)==saved);CHECK(w.valid==unsigned(nr::core_snapshot_valid(value,now,limit,true)));
        CHECK(w.now==now&&w.limit==limit&&w.mode==1);return w;
    };
    auto w=inspect(source);CHECK(w.valid&&w.non_age_valid&&!w.expired&&w.controllers_match==1);
    auto changed=source;changed.published_ns=now-50'000'000;changed.stereo.received_ns=now-50'000'000;
    changed.controllers.received_ns=now-60'000'000;changed.controllers.sync_ns=now-61'000'000;
    w=inspect(changed);CHECK(w.valid&&!w.expired);
    changed.published_ns--;changed.stereo.received_ns--;
    w=inspect(changed);CHECK(!w.valid&&w.non_age_valid&&w.expired==3);
    changed=source;changed.stereo.received_ns=now-50'000'001;
    w=inspect(changed);CHECK(!w.valid&&w.non_age_valid&&w.expired==2);
    changed=source;changed.controllers.received_ns=now-100'000'001;changed.controllers.sync_ns=now-100'000'002;
    w=inspect(changed);CHECK(!w.valid&&w.non_age_valid&&!w.expired&&!w.controllers_match);
    changed=source;changed.native_reference_revision|=1;
    w=inspect(changed);CHECK(!w.valid&&!w.non_age_valid&&!w.expired);
    changed=source;changed.published_ns=now+1;
    w=inspect(changed);CHECK(!w.valid&&!w.non_age_valid&&!w.expired);
    changed=source;changed.stereo.received_ns=now+1;
    w=inspect(changed);CHECK(!w.valid&&!w.non_age_valid&&!w.expired);
    changed=source;changed.source_ticket=0;
    w=inspect(changed);CHECK(!w.valid&&!w.non_age_valid&&w.ticket==0);
    changed=source;changed.flags|=0x80000000u;
    w=inspect(changed);CHECK(!w.valid&&!w.non_age_valid);
    namespace tick=gears_body_tick_source;
    tick::Input input{};input.scene_cache_revision=2;
    const auto current=now+1'000'000'000;
    input.metadata_begin_ns=current-10'000'000;input.metadata_end_ns=current-8'000'000;
    auto& t=input.observed;t.parent_scene=source;t.tick=1;t.thread=source.native.game_thread;t.parent_cookie=123;
    t.begin_ns=current-7'000'000;t.end_ns=current-3'000'000;
    t.stereo=source.stereo;t.stereo.sequence+=5;t.stereo.received_ns=current-4'000'000;t.stereo.predicted_time+=1'000'000'000;
    t.controllers=source.controllers;t.controllers.sequence=t.stereo.sequence;t.controllers.predicted_time=t.stereo.predicted_time;
    t.controllers.received_ns=current-5'000'000;t.controllers.sync_ns=current-6'000'000;
    t.continuity.input=t.controllers;t.continuity.stream_epoch=source.producer.stream;
    for(unsigned i=0;i<2;++i)t.continuity.tracking_epoch[i]=source.producer.tracking[i];
    t.reference_revision=source.native_reference_revision;t.native_reference=source.native_reference;
    t.publisher_pid=source.controller_publisher_pid;t.publisher_start=source.controller_publisher_start;
    CHECK(tick::classify(input,source,current,true)==tick::Verdict::okay);
    const auto input_before=bytes(input);
    w=observation::observe(input,current,limit,true);
    CHECK(bytes(input)==input_before);
    CHECK(w.mode==2&&!w.valid&&w.non_age_valid&&w.verdict==unsigned(tick::Verdict::okay));
    CHECK(w.metadata_begin==input.metadata_begin_ns&&w.tick_end==t.end_ns&&!w.hard_detail);
    auto late=input;late.metadata_begin_ns=current-50'000'001;
    w=observation::observe(late,current,limit,true);
    CHECK(w.verdict==unsigned(tick::Verdict::source_age_only));
    auto hard=input;hard.scene_cache_revision=0;
    w=observation::observe(hard,current,limit,true);
    CHECK(w.verdict==unsigned(tick::Verdict::hard)&&w.hard_detail);
    std::printf("cover measurement core observation %u checks, 0 failures; synthetic copies only\n",checks);
    return 0;
}
