#include "full_body_c0_first_failure.h"
#include <cstdio>
namespace d=gears_full_body_c0_first_failure;namespace b=d::body;
int main(){printf("{\"magic\":\"C0FAIL01\",\"version\":1,\"endian\":\"little\",\"record_size\":%zu,\"header_size\":%zu,\"goal_size\":%zu,\"offsets\":{",sizeof(d::Record),sizeof(d::Header),sizeof(b::Goal));
#define FIELD(T,N) printf("\"" #N "\":{\"offset\":%zu,\"size\":%zu},",offsetof(T,N),sizeof(T::N))
    FIELD(d::Record,header);FIELD(d::Record,stamp);FIELD(d::Record,metadata);FIELD(d::Record,source);FIELD(d::Record,goal);FIELD(d::Record,report);FIELD(d::Record,output);
    printf("\"desired\":{\"offset\":%zu,\"size\":%zu}},\"header_offsets\":{",offsetof(d::Record,desired),sizeof(d::Record::desired));
    FIELD(d::Header,version);FIELD(d::Header,bytes);FIELD(d::Header,header_bytes);FIELD(d::Header,frame_bytes);FIELD(d::Header,goal_bytes);FIELD(d::Header,report_bytes);FIELD(d::Header,output_bytes);FIELD(d::Header,desired_bytes);
    FIELD(d::Header,stage);FIELD(d::Header,detail);FIELD(d::Header,unit);FIELD(d::Header,valid_mask);FIELD(d::Header,thread);FIELD(d::Header,kernel_pending);FIELD(d::Header,kernel_fields);FIELD(d::Header,kernel_protocol);FIELD(d::Header,kernel_flags);
    FIELD(d::Header,session);FIELD(d::Header,tick);FIELD(d::Header,compose);FIELD(d::Header,qpc);FIELD(d::Header,ra);FIELD(d::Header,rcx);FIELD(d::Header,rdx);FIELD(d::Header,r8);FIELD(d::Header,registry_version);
    FIELD(d::Header,timing);FIELD(d::Header,core_published_ns);FIELD(d::Header,stereo_received_ns);FIELD(d::Header,controllers_received_ns);
    FIELD(d::Header,core_age_begin);FIELD(d::Header,core_age_end);FIELD(d::Header,stereo_age_begin);FIELD(d::Header,stereo_age_end);FIELD(d::Header,controllers_age_begin);
    printf("\"controllers_age_end\":{\"offset\":%zu,\"size\":%zu}},\"timing_offsets\":{",offsetof(d::Header,controllers_age_end),sizeof(d::Header::controllers_age_end));
    FIELD(d::Timing,begin);FIELD(d::Timing,gate);FIELD(d::Timing,plan);FIELD(d::Timing,prepare);FIELD(d::Timing,commit);FIELD(d::Timing,end);
    printf("\"frequency\":{\"offset\":%zu,\"size\":%zu}}}\n",offsetof(d::Timing,frequency),sizeof(d::Timing::frequency));return 0;
#undef FIELD
}
