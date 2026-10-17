// One-copy causal diagnostic only. Actual wrappers and fake GPU; no game.
#define GEARS_NGX_OUTPUT_PRESERVE_PAIR_INPUT 1
#define GEARS_NGX_OUTPUT_PRESERVE_TEST_INCLUDED 1
#include "ngx_output_delayed_test.cpp"
namespace {
unsigned game_copy_calls{},readback_copy_calls{};bool throw_game_copy{},throw_readback{};
void STDMETHODCALLTYPE counted_copy(ID3D12GraphicsCommandList* list,const D3D12_TEXTURE_COPY_LOCATION* destination,UINT,UINT,UINT,const D3D12_TEXTURE_COPY_LOCATION*,const D3D12_BOX*){
    assert(list==pixel_list);
    if(destination->Type==D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT&&!destination->pResource){++readback_copy_calls;if(throw_readback)throw std::runtime_error("readback");}
    else{++game_copy_calls;if(throw_game_copy)throw std::runtime_error("game copy");}
}
void preserve_reset(unsigned output_width=2560,unsigned output_height=1080){
    pixel_reset();create_mutation=0;ngx_route_observer::original_create=stable_create;ngx_output_pixels::delayed={};ngx_route_observer::recorder.create_count=4;
    const std::array<uint32_t,6> create_fields{1706,720,output_width,output_height,2,67};
    for(unsigned i=0;i<create_fields.size();++i)put(creation,i*4,create_fields[i]);put(creation,0x18,uint8_t(0));
    assert(invoke_create()==create_result&&ngx_output_pixels::delayed.epoch_valid);ngx_route_observer::recorder.evaluate_count=8;
    ngx_output_pixels::suppression={};game_copy_calls=readback_copy_calls=0;throw_game_copy=throw_readback=false;
}
void selected_first_eval(){preserve_reset();arm();warm();ticket(132);pixel_mode=0;assert(invoke_evaluate()==evaluate_result);}
void probe_copy(unsigned variant=0){D3D12_TEXTURE_COPY_LOCATION source{},destination{};D3D12_BOX box{};UINT x=0;const D3D12_BOX* region=nullptr;uintptr_t caller=0x1c6093;
    std::memcpy(&source.pResource,evaluation.data()+8,8);std::memcpy(&destination.pResource,evaluation.data(),8);
    source.Type=destination.Type=D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
    if(variant==1){source.pResource=reinterpret_cast<ID3D12Resource*>(uintptr_t(0x900));destination.pResource=reinterpret_cast<ID3D12Resource*>(uintptr_t(0x901));}
    else if(variant==2)caller=0x123;
    else if(variant==3)source.SubresourceIndex=1;
    else if(variant==4)destination.Type=D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
    else if(variant==5)x=1;
    else if(variant==6)region=&box;
    else if(variant==7)destination.SubresourceIndex=1;
    ngx_output_pixels::copy_passthrough(pixel_list,&destination,x,0,0,&source,region,caller,counted_copy);
}
void second_eval_copy(){put(evaluation,0x60,uint32_t(1280));assert(invoke_evaluate()==evaluate_result);probe_copy();}
void good_pair(){using namespace ngx_output_pixels;
    selected_first_eval();throw_game_copy=true;probe_copy();throw_game_copy=false;
    assert(suppression.valid&&suppression.suppressed&&!suppression.original_forwarded&&suppression.readback_started&&suppression.readback_returned);
    assert(game_copy_calls==0&&readback_copy_calls==1&&fake_backend.prepared==1&&fake_backend.appended==1&&!failure);
    assert(suppression.epoch==1&&suppression.world==9&&suppression.ticket==132&&suppression.ordinal==1&&suppression.eval_sequence==1&&suppression.copy_sequence==1&&
           suppression.caller==0x1c6093&&suppression.thread==GetCurrentThreadId()&&suppression.list_raw==uintptr_t(pixel_list)&&suppression.list_canonical==0x700&&suppression.generation==4&&
           suppression.source_canonical==0x101&&suppression.destination_canonical==0x102&&suppression.format==26&&suppression.width==2560&&suppression.height==1080&&
           suppression.source_type==unsigned(D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX)&&suppression.destination_type==unsigned(D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX)&&
           !suppression.source_subresource&&!suppression.destination_subresource&&!suppression.x&&!suppression.y&&!suppression.z&&suppression.box_null);
    second_eval_copy();assert(game_copy_calls==1&&readback_copy_calls==2&&fake_backend.appended==2&&!failure);
    assert(slots[0].original_suppressed&&!slots[0].original_forwarded&&!slots[0].original_returned&&slots[0].readback_returned&&
           !slots[1].original_suppressed&&slots[1].original_forwarded&&slots[1].original_returned&&slots[1].readback_returned);
    end_ticket(132,true,{true,true,false,0,0xface});assert(published&&delayed.closed);
    execute();poll();assert(!fake_backend.mapped);retire();poll();assert(fake_backend.mapped==2&&fake_backend.released==2);
    probe_copy();assert(game_copy_calls==2&&readback_copy_calls==2&&suppression.ordinal==1); // Every later copy forwards once.
    FILE* report{};assert(!tmpfile_s(&report));print(report);assert(contains(report,"omission_count=1")&&contains(report,"preserve_original_forwarded=0")&&contains(report,"original_suppressed=1")&&contains(report,"causal_pixels_verified=0"));fclose(report);
}
void rejected_cases(){using namespace ngx_output_pixels;
    preserve_reset();probe_copy();assert(game_copy_calls==1&&!readback_copy_calls&&!suppression.valid&&!count); // No Resume/selected scope.
    selected_first_eval();probe_copy(1);assert(game_copy_calls==1&&!readback_copy_calls&&!suppression.valid&&!failure);probe_copy();assert(game_copy_calls==1&&readback_copy_calls==1&&suppression.valid); // Unrelated copy never suppressed.
    for(unsigned variant:{2u,3u,4u,5u,6u,7u}){selected_first_eval();probe_copy(variant);assert(game_copy_calls==1&&!readback_copy_calls&&!suppression.valid&&failure==typed_mismatch);}
    for(unsigned unknown:{0u,1u,2u,3u,4u,5u,6u}){selected_first_eval();
        if(unknown==0)fake_backend.same_device=false;
        else if(unknown==1)fake_backend.width=1280;
        else if(unknown==2)fake_backend.format=24;
        else if(unknown==3)fake_backend.unknown_identity=true;
        else if(unknown==4)put(evaluation,0x18,uintptr_t(0x999));
        else if(unknown==5)cells_available=false;
        else fake_backend.typed=false; // Includes the native prepare DIRECT-list/resource certification contract.
        probe_copy();assert(game_copy_calls==1&&!readback_copy_calls&&!suppression.valid&&failure&&fake_backend.released==1);
        cells_available=true;probe_copy();assert(game_copy_calls==2&&!readback_copy_calls&&!suppression.valid); // Quarantine never retries omission.
    }
    selected_first_eval();admitted=false;assert(invoke_evaluate()==evaluate_result);probe_copy();assert(!suppression.valid&&game_copy_calls==1&&!readback_copy_calls&&failure);
    preserve_reset();arm();warm();ticket(132);pixel_mode=1;assert(invoke_evaluate()==evaluate_result);
    assert(!suppression.valid&&original_copies==1&&!extra_copies&&failure==preserve_pair_unproved); // Cannot know in-Eval normal return.
}
void terminal_cases(){using namespace ngx_output_pixels;
    selected_first_eval();throw_readback=true;try{probe_copy();assert(false);}catch(std::runtime_error const&){}
    assert(suppression.valid&&suppression.readback_started&&!suppression.readback_returned&&failure==append_exception&&slots[0].life.appended&&!fake_backend.released&&
           game_copy_calls==0&&readback_copy_calls==1);throw_readback=false;probe_copy();assert(game_copy_calls==1&&readback_copy_calls==1);
    selected_first_eval();probe_copy();put(evaluation,0x60,uint32_t(1280));assert(invoke_evaluate()==evaluate_result);throw_game_copy=true;
    try{probe_copy();assert(false);}catch(std::runtime_error const&){}throw_game_copy=false;
    assert(suppression.valid&&failure==copy_exception&&game_copy_calls==1&&readback_copy_calls==1&&fake_backend.released==1&&slots[0].life.appended);
    for(unsigned terminal:{0u,1u,2u,3u}){selected_first_eval();probe_copy();const auto receipt_qpc=suppression.qpc;
        if(terminal==0)stop();
        else if(terminal==1)assert(invoke_create()==create_result);
        else if(terminal==2){put(evaluation,0x60,uint32_t(1280));assert(invoke_evaluate()==evaluate_result);assert(invoke_evaluate()==evaluate_result);}
        else{recording=false;assert(invoke_evaluate()==evaluate_result);}
        probe_copy();assert(suppression.valid&&suppression.qpc==receipt_qpc&&game_copy_calls==1&&readback_copy_calls==1&&!fake_backend.released&&slots[0].life.appended);
        auto request=identity();request.nonce=2;assert(!delayed.arm(request,request,uint64_t(now()),300000)); // Selected intervention cannot rearm.
    }
}
void pinned_state_cases(){using namespace ngx_output_pixels;
    for(unsigned wrong_format:{0u,1u,2u}){selected_first_eval();
        if(wrong_format!=1)fake_backend.format=10;if(wrong_format!=0)fake_backend.destination_format=10;
        probe_copy();assert(game_copy_calls==1&&!readback_copy_calls&&!suppression.valid&&failure&&fake_backend.released==1&&!slots[0].life.appended);
    }
    selected_first_eval();fake_backend.width=fake_backend.destination_width=3840;fake_backend.height=fake_backend.destination_height=2160;
    probe_copy();assert(textures_supported(slots[0].source_desc,slots[0].destination_desc)&&failure==preserve_pair_unproved&&
        game_copy_calls==1&&!readback_copy_calls&&!suppression.valid&&fake_backend.prepared==1&&fake_backend.released==1&&!slots[0].life.appended);
    for(unsigned field:{2u,3u}){preserve_reset(field==2?3840u:2560u,field==3?2160u:1080u);arm();warm();ticket(132);pixel_mode=0;assert(invoke_evaluate()==evaluate_result);
        probe_copy();assert(failure==preserve_pair_unproved&&game_copy_calls==1&&!readback_copy_calls&&!suppression.valid&&fake_backend.released==1&&!slots[0].life.appended);
    }
}
}
int main(){good_pair();rejected_cases();terminal_cases();pinned_state_cases();std::puts("NgxOutputPreservePairTest passed (one original omission, unfiltered readback forwarding; fake GPU, no game)");return 0;}
