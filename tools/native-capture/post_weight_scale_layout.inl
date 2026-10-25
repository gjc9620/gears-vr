// Mechanical MSVC ABI exporter for this independent sidecar only.
void print_post_weight_layout(){namespace s=cover_control_observer_live::post_weight_scale;
    printf("{\"schema\":1,\"compiler\":\"MSVC x64\",\"sizes\":{\"Header\":%zu,\"Record\":%zu,\"Readback\":%zu,\"CCTRL_Row\":%zu,\"NodeState\":%zu,\"CallPacket\":%zu},",
        sizeof(s::Header),sizeof(s::Record),sizeof(s::Readback),sizeof(live::Row),sizeof(live::NodeState),sizeof(p::CallPacket));
#define FIELD(T,F) printf("\"" #F "\":%zu,",offsetof(T,F))
#define LAST(T,F) printf("\"" #F "\":%zu}",offsetof(T,F))
    printf("\"Header\":{");FIELD(s::Header,magic);FIELD(s::Header,version);FIELD(s::Header,header_size);FIELD(s::Header,row_size);FIELD(s::Header,count);FIELD(s::Header,capacity);
    FIELD(s::Header,session);FIELD(s::Header,module_base);FIELD(s::Header,compose);FIELD(s::Header,pid);FIELD(s::Header,pose_episode);FIELD(s::Header,read_only);FIELD(s::Header,atomic_verified);
    FIELD(s::Header,writer);FIELD(s::Header,v248_observed);FIELD(s::Header,scale_payload_decoded);LAST(s::Header,cached_flag_bytes);
    printf(",\"Record\":{");FIELD(s::Record,compute);FIELD(s::Record,weight);FIELD(s::Record,scope_owner);FIELD(s::Record,scope_compose);FIELD(s::Record,scope_outer_tick);
    FIELD(s::Record,scope_address);FIELD(s::Record,call_address);FIELD(s::Record,scope_episode);FIELD(s::Record,scope_thread);FIELD(s::Record,scope_depth);FIELD(s::Record,call_index);
    FIELD(s::Record,frame_index);FIELD(s::Record,matrix_count);FIELD(s::Record,has_scale_address);FIELD(s::Record,scale_address);FIELD(s::Record,output_address);FIELD(s::Record,affected_address);
    FIELD(s::Record,sample_begin_qpc);FIELD(s::Record,sample_end_qpc);FIELD(s::Record,node_before);FIELD(s::Record,node_after);FIELD(s::Record,before);FIELD(s::Record,after);
    FIELD(s::Record,original_return_packet);FIELD(s::Record,reject);FIELD(s::Record,original_returned);FIELD(s::Record,scope_open);FIELD(s::Record,entry_before);FIELD(s::Record,entry_after);
    FIELD(s::Record,node_before_valid);FIELD(s::Record,node_after_valid);FIELD(s::Record,linked_source);FIELD(s::Record,packet_unchanged);FIELD(s::Record,compose_closed);FIELD(s::Record,compose_returned);
    FIELD(s::Record,nested);LAST(s::Record,concurrent);
    printf(",\"Readback\":{");FIELD(s::Readback,return_address);FIELD(s::Readback,has_scale_raw4);FIELD(s::Readback,has_scale_word);FIELD(s::Readback,scale);
    FIELD(s::Readback,output);FIELD(s::Readback,affected);FIELD(s::Readback,bones);FIELD(s::Readback,matrices);LAST(s::Readback,read_mask);
    printf(",\"NodeState\":{");FIELD(live::NodeState,node);FIELD(live::NodeState,root);FIELD(live::NodeState,tree);FIELD(live::NodeState,vtable);FIELD(live::NodeState,metadata);FIELD(live::NodeState,parameters);LAST(live::NodeState,methods);
    printf(",\"CallPacket\":{");FIELD(p::CallPacket,gpr);FIELD(p::CallPacket,flags);LAST(p::CallPacket,xmm);
    printf("}\n");
#undef FIELD
#undef LAST
}
