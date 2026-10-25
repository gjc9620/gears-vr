// CPU HOST SEAM ONLY: synthetic originals/memory/identity/entry attestation.
// No native authority, engine timing, scale semantics, or headset acceptance.
namespace post=cover_control_observer_live::post_weight_scale;
unsigned original_calls{};char const* scenario="normal";p::CallPacket last_return{};
extern "C" void cover_body_live::GearsCoverRawInvoke(CallPacket* packet,void* original) noexcept(false){
    ++original_calls;const auto kind=unsigned(reinterpret_cast<uintptr_t>(original)-1);
    C(active.load()>0&&live::in_flight.load()>0);
    if(kind<2){const unsigned n=kind?1:3;for(unsigned i=0;i<n;++i){auto& m=output_matrices[i];m={};
            m.values[0]=m.values[5]=m.values[10]=m.values[15]=1;m.values[12]=float(10+i);}
        const p::ArrayHeader h{reinterpret_cast<uintptr_t>(output_matrices.data()),int32_t(n),int32_t(n)};
        memcpy(reinterpret_cast<void*>(packet->gpr[4]),&h,sizeof h);
    }else{
        for(unsigned x=0;x<6;++x)packet->xmm[x].fill(static_cast<unsigned char>(0xa0+x));
        const float alpha=!strcmp(scenario,"alpha")?.625f:1.0f;memcpy(packet->xmm[0].data(),&alpha,4);
        if(!strcmp(scenario,"changed"))change_matrix_on_read=true;
        if(!strcmp(scenario,"read-failed"))fail_read_at=packet->gpr[4];
    }
    packet->gpr[0]=0x123456789abcdef0ull;packet->gpr[5]=0x1122334455667788ull;packet->gpr[6]=0x8877665544332211ull;
    packet->flags=0x202;last_return=*packet;
}
void pair(unsigned kind,int end){
    game_state(kind);affected=end==11?std::array<int32_t,3>{{8,10,11}}:std::array<int32_t,3>{{32,34,35}};
    if(kind)affected[0]=end;
    alignas(16) unsigned char stack[0x120]{};const auto rsp=reinterpret_cast<uintptr_t>(stack)+8;
    p::ArrayHeader out{},scale_header{},indices{reinterpret_cast<uintptr_t>(affected.data()),int32_t(kind?1:3),int32_t(kind?1:3)};
    memcpy(reinterpret_cast<void*>(rsp+0x70),&scale_header,16);memcpy(reinterpret_cast<void*>(rsp+0x80),&out,16);memcpy(reinterpret_cast<void*>(rsp+0x90),&indices,16);
    const uintptr_t compute_ra=base+0x1290e5;memcpy(reinterpret_cast<void*>(rsp),&compute_ra,8);
    p::CallPacket packet{};packet.gpr[1]=node;packet.gpr[2]=uint32_t(end);packet.gpr[3]=mesh;packet.gpr[4]=rsp+0x80;
    live::GearsCoverControlInvokeProtected(&packet,kind,compute_ra,rsp);C(!memcmp(&packet,&last_return,sizeof packet));
    C(live::rows[live::row_count-1].after_valid);const auto original_matrices=output_matrices;const auto original_memory=memory;
    // Only the test fixture creates native-looking stack state between calls.
    if(!strcmp(scenario,"nonempty"))scale_header={0xbad00000,1,1};
    if(!strcmp(scenario,"bad-header"))scale_header={0xbad00000,-1,1};
    memcpy(reinterpret_cast<void*>(rsp+0x70),&scale_header,16);
    const uint32_t cached_flag=!strcmp(scenario,"flag")?0x80000000u:0u;memcpy(reinterpret_cast<void*>(rsp+0x48),&cached_flag,4);
    const uintptr_t weight_ra=base+0x12911f;memcpy(reinterpret_cast<void*>(rsp),&weight_ra,8);
    packet={};packet.gpr[1]=node;packet.gpr[2]=0xdead;packet.gpr[3]=0xbeef;
    packet.gpr[4]=rsp+(!strcmp(scenario,"wrong-r9")?0x80:0x70); // Stale RDX/R8 are never the source binding.
    live::GearsCoverControlInvokeProtected(&packet,2,weight_ra,rsp);C(!memcmp(&packet,&last_return,sizeof packet));
    C(memory==original_memory);if(strcmp(scenario,"changed"))C(!memcmp(output_matrices.data(),original_matrices.data(),sizeof output_matrices));
    else {auto expected=original_matrices;expected[0].values[13]+=1;C(!memcmp(output_matrices.data(),expected.data(),sizeof output_matrices));}
    C(!memcmp(reinterpret_cast<void*>(rsp+0x70),&scale_header,16));C(!memcmp(reinterpret_cast<void*>(rsp+0x48),&cached_flag,4));
}
int main(int argc,char** argv){C(argc==3);if(argc!=3)return 1;scenario=argv[1];
    if(!strcmp(scenario,"layout")){print_post_weight_layout();return 0;}
    C(swprintf_s(directory,L"%S\\post-%lu-%llu",argv[2],GetCurrentProcessId(),GetTickCount64())>0);C(CreateDirectoryW(directory,nullptr)!=FALSE);
    for(unsigned i=0;i<3;++i){code[i].address=base+gears_cover_control_certificate::hooks[i].rva;code[i].bytes=gears_cover_control_certificate::hooks[i].prefix;code[i].bytes[0]=0xe9;
        live::targets[i]=reinterpret_cast<void*>(code[i].address);live::originals[i]=reinterpret_cast<void*>(uintptr_t(i+1));}
    for(unsigned i=3;i<6;++i){code[i].address=base+0x700000+(i-3)*0x100;code[i].bytes.fill(0xcc);}
    code[4].address=base+0xc3f20;code[4].bytes[0]=0xc2;code[4].bytes[1]=code[4].bytes[2]=0;
    if(!strcmp(scenario,"normal")){wchar_t ini[MAX_PATH]{};C(swprintf_s(ini,L"%s\\observer.ini",directory)>0);
        C(live::configure(ini,nullptr)&&!live::enabled&&!post::enabled);
        C(WritePrivateProfileStringW(L"CoverPostWeightScaleObserver",L"enabled",L"invalid",ini)!=FALSE);C(!live::configure(ini,nullptr));
        C(WritePrivateProfileStringW(L"CoverPostWeightScaleObserver",L"enabled",L"1",ini)!=FALSE);C(!live::configure(ini,nullptr)&&!post::enabled);
        C(WritePrivateProfileStringW(L"CoverControlObserver",L"enabled",L"1",ini)!=FALSE);C(live::configure(ini,nullptr)&&live::enabled&&post::enabled);
    }
    live::enabled=true;post::enabled=true;auto s=scope();live::compose_begin(s);
    if(!strcmp(scenario,"normal")){pair(0,11);pair(1,11);pair(0,35);pair(1,35);
        C(post::count==4);for(unsigned i=0;i<4;++i){const auto& r=post::records[i];C(r.reject==post::Reject::okay);C(r.before.read_mask==127&&r.after.read_mask==127);
            C(r.before.has_scale_word==0&&r.before.scale.count==0&&r.linked_source&&r.original_returned&&r.scope_open);
            C(r.entry_before&&r.entry_after&&r.packet_unchanged&&!r.compose_closed);C(r.weight.out_argument==r.scale_address&&r.compute.out_argument==r.output_address);}
        pair(0,11);C(post::count==4&&post::dropped==1&&original_calls==10);
    }else{pair(0,11);C(post::count==1&&original_calls==2);const auto& r=post::records[0];
        const auto expected=!strcmp(scenario,"nonempty")?post::Reject::scale_nonempty:!strcmp(scenario,"bad-header")?post::Reject::shape:
            !strcmp(scenario,"flag")?post::Reject::scale_flag:!strcmp(scenario,"alpha")?post::Reject::alpha:
            !strcmp(scenario,"wrong-r9")?post::Reject::stack:!strcmp(scenario,"read-failed")?post::Reject::read:
            !strcmp(scenario,"compose-failed")?post::Reject::okay:post::Reject::changed;
        if(r.reject!=expected)fprintf(stderr,"reject=%u expected=%u CCTRL=%u\n",unsigned(r.reject),unsigned(expected),unsigned(r.weight.failure));C(r.reject==expected);C(r.packet_unchanged);
        if(!strcmp(scenario,"nonempty"))C(r.before.scale.count==1&&r.after.scale.count==1&&r.before.scale.data==0xbad00000);
        if(!strcmp(scenario,"flag"))C(r.before.has_scale_word==0x80000000u); // Catches an erroneous byte-only read.
        if(!strcmp(scenario,"alpha"))C(r.weight.weight_bits==0x3f200000u&&r.original_return_packet.xmm[0][4]==0xa0);
        if(!strcmp(scenario,"read-failed"))C(!(r.before.read_mask&post::scale_read)&&!(r.after.read_mask&post::scale_read));
    }
    const bool compose_returned=strcmp(scenario,"compose-failed")!=0;live::compose_end(s,compose_returned);
    for(unsigned i=0;i<post::count;++i){C(post::records[i].compose_closed&&post::records[i].compose_returned==compose_returned);
        if(!compose_returned)C(post::records[i].reject==post::Reject::compose_failed);}
    live::request_drain();C(!live::pending_count());live::print(nullptr,true);C(post::written.load()&&live::file_written.load());
    C(sizeof(live::Row)==1944&&sizeof(live::Header)==72); // Existing CCTRL-v1b unchanged.
    printf("PostWeightScaleTest %s: %u checks, %u failures; sidecar_Record=%zu Header=%zu synthetic_host_only=1 writer=0 v248_observed=0\n",scenario,checks,failures,sizeof(post::Record),sizeof(post::Header));
    return failures?1:0;
}
