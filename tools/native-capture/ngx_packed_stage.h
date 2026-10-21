#pragma once
#include "ngx_stereo_layout.h"
namespace gears_ngx_stereo {
enum class PackedRole:unsigned {color,output,right_input,left_output};
enum class PackedState:unsigned {destination,source};
struct PackedRequest {
    Identity identity{};uint64_t ticket{},list{},generation{};
    std::array<Resource,4> resources{}; // Color, Output, private Right, private Left.
    Rect right{};Extent output{};
};
class PackedStage {
    PackedRequest request_{};unsigned phase_{};bool failed_{};
    BackupReceipt input_{},output_{};
    bool fail(){failed_=true;return false;}
public:
    uint64_t backups{},input_restores{},output_restores{};
    bool failed()const{return failed_;}
    bool right_ready(uint64_t ticket)const{return !failed_&&phase_==2&&request_.ticket==ticket;}
    BackupReceipt input_backup()const{return input_;}
    BackupReceipt output_backup()const{return output_;}
    template<class B> bool backup(B& b,PackedRequest const& r){
        if(failed_||phase_||!valid(r.identity)||!r.ticket||!r.list||!r.generation||!bounded(r.output))return fail();
        for(unsigned i=0;i<4;++i){auto const& x=r.resources[i];
            if(!x.identity||x.device!=r.identity.device||x.format!=26||x.mip||x.samples!=1||x.subresource)return fail();
            for(unsigned j=0;j<i;++j)if(x.identity==r.resources[j].identity)return fail();}
        if(!contains(r.resources[0].extent,r.right)||!contains(r.resources[1].extent,{0,0,r.output.width,r.output.height})||
            !contains(r.resources[2].extent,{0,0,r.right.width,r.right.height})||!contains(r.resources[3].extent,{0,0,r.output.width,r.output.height}))return fail();
        request_=r;phase_=1;
        if(!b.transition(PackedRole::color,PackedState::destination,PackedState::source)||
            !b.copy(PackedRole::color,r.right,PackedRole::right_input,{0,0})||
            !b.transition(PackedRole::color,PackedState::source,PackedState::destination)||
            !b.transition(PackedRole::right_input,PackedState::destination,PackedState::source)||
            !b.copy(PackedRole::output,{0,0,r.output.width,r.output.height},PackedRole::left_output,{0,0})||
            !b.transition(PackedRole::left_output,PackedState::destination,PackedState::source))return fail();
        input_={r.identity,r.ticket,r.list,r.generation,r.resources[0].identity,r.resources[2],r.right,true};
        output_={r.identity,r.ticket,r.list,r.generation,r.resources[1].identity,r.resources[3],{0,0,r.output.width,r.output.height},true};
        ++backups;return true;
    }
    template<class B> bool restore_input(B& b,uint64_t ticket){
        if(failed_||phase_!=1||request_.ticket!=ticket)return fail();auto const& r=request_;
        if(!b.copy(PackedRole::right_input,{0,0,r.right.width,r.right.height},PackedRole::color,{r.right.x,r.right.y})||
            !b.transition(PackedRole::right_input,PackedState::source,PackedState::destination))return fail();
        phase_=2;++input_restores;return true;
    }
    template<class B> bool restore_output(B& b,uint64_t ticket){
        if(failed_||phase_!=2||request_.ticket!=ticket)return fail();auto const& r=request_;
        if(!b.copy(PackedRole::left_output,{0,0,r.output.width,r.output.height},PackedRole::color,{0,0})||
            !b.transition(PackedRole::left_output,PackedState::source,PackedState::destination))return fail();
        phase_=0;++output_restores;return true;
    }
};
}
