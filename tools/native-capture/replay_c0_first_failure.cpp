#define main PriorBackendMain
#include "full_body_c0_native_backend_test.cpp"
#undef main
#include "full_body_c0_first_failure.h"
int main(int argc,char**argv){if(argc!=2)return 2;namespace failure=gears_full_body_c0_first_failure;
    auto saved=std::make_unique<failure::Record>();if(!read_file(argv[1],saved.get(),sizeof *saved))return 3;
    auto const&h=saved->header;if(std::memcmp(h.magic,"C0FAIL01",8)||h.version!=1||h.bytes!=sizeof *saved||h.header_bytes!=sizeof h||
        h.frame_bytes!=sizeof(body::Frame)||h.goal_bytes!=296||h.report_bytes!=sizeof(body::Report)||h.output_bytes!=sizeof(body::Output)||h.desired_bytes!=sizeof(tx::Desired)||(h.valid_mask&3)!=3)return 4;
    body::Model model{};if(!body::prepare_model(saved->source.reference,hash,model))return 5;
    auto work=std::make_unique<body::Workspace>();auto output=std::make_unique<body::Output>();
    const auto replay=body::plan(model,saved->source,saved->goal,*work,*output);
    const auto&old=saved->report;const bool report_same=(h.valid_mask&4)&&old.status==replay.status&&old.failed_bone==replay.failed_bone&&old.spine_iterations==replay.spine_iterations&&
        old.requested_distance==replay.requested_distance&&old.available_reach==replay.available_reach&&old.head_error==replay.head_error&&old.max_length_error==replay.max_length_error;
    printf("{\"bytes\":%u,\"goal_bytes\":%u,\"stage\":%u,\"detail\":%u,\"unit\":%u,\"tick\":%llu,\"compose\":%llu,\"plan_status\":%u,\"failed_bone\":%u,\"requested_distance\":%.17g,\"available_reach\":%.17g,\"head_error\":%.17g,\"saved_report_matches\":%s,\"Game\":false}\n",
        h.bytes,h.goal_bytes,h.stage,h.detail,h.unit,h.tick,h.compose,unsigned(replay.status),replay.failed_bone,replay.requested_distance,replay.available_reach,replay.head_error,report_same?"true":"false");
    printf("{\"kernel_pending\":%u,\"kernel_fields\":%u,\"kernel_protocol\":%u,\"kernel_flags\":%u,\"qpc_begin\":%llu,\"qpc_gate\":%llu,\"qpc_plan\":%llu,\"qpc_prepare\":%llu,\"qpc_commit\":%llu,\"qpc_end\":%llu,\"qpc_frequency\":%llu,\"core_published_ns\":%llu,\"stereo_received_ns\":%llu,\"controllers_received_ns\":%llu,\"core_age_begin_ns\":%lld,\"core_age_end_ns\":%lld,\"stereo_age_begin_ns\":%lld,\"stereo_age_end_ns\":%lld,\"controllers_age_begin_ns\":%lld,\"controllers_age_end_ns\":%lld}\n",
        h.kernel_pending,h.kernel_fields,h.kernel_protocol,h.kernel_flags,h.timing.begin,h.timing.gate,h.timing.plan,h.timing.prepare,h.timing.commit,h.timing.end,h.timing.frequency,h.core_published_ns,h.stereo_received_ns,h.controllers_received_ns,
        h.core_age_begin,h.core_age_end,h.stereo_age_begin,h.stereo_age_end,h.controllers_age_begin,h.controllers_age_end);return 0;
}
