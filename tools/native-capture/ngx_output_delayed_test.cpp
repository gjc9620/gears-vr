// Actual unchanged forwarding wrappers with a fake GPU, plus pure request/gate
// checks. No native hooks, game, GPU, input, or runtime publication.
#define GEARS_NGX_OUTPUT_DELAYED_CAPTURE 1
#define GEARS_NGX_OUTPUT_DELAYED_TEST_INCLUDED 1
#include "ngx_output_pixels_test.cpp"
#include <string>
namespace {
using gears_ngx_delayed::Request;
using gears_ngx_delayed::Reason;
Request identity(){return {7,8,9,9,ngx_output_pixels::delayed.epoch,1};}
unsigned create_mutation{};
uint64_t __fastcall stable_create(uintptr_t a,uintptr_t b,uintptr_t c,uintptr_t d,uintptr_t e,const void* input){
    const auto entry_parameters=parameters;const auto entry_creation=creation;
    const auto result=original_create(a,b,c,d,e,input); // The original mock is forwarded exactly once.
    parameters=entry_parameters;creation=entry_creation; // Default fixture changes only the handle.
    if(create_mutation==1)cells_available=true;
    else if(create_mutation==2)unreadable=0;
    else if(create_mutation==3)put(creation,0,uint32_t(777));
    else if(create_mutation==4)++parameters;
    else if(create_mutation==5)put(context,0x14c0,reinterpret_cast<uintptr_t>(views.data())+0xd30);
    return result;
}
void delayed_reset(){pixel_reset();create_mutation=0;ngx_route_observer::original_create=stable_create;ngx_output_pixels::delayed={};ngx_route_observer::recorder.create_count=4;assert(invoke_create()==create_result);
    assert(ngx_output_pixels::delayed.epoch_valid&&ngx_route_observer::recorder.create_count==4&&create_calls==1);
    ngx_route_observer::recorder.evaluate_count=8;
}
void arm(){const auto r=identity();assert(ngx_output_pixels::delayed.arm(r,r,uint64_t(now()),300000));}
void ticket(uint64_t id){consuming_frame_ticket=id;consuming_frozen_frame.id=id;put(evaluation,0x60,uint32_t(0));}
void frame(uint64_t id,bool copies=false){ticket(id);pixel_mode=copies?1:0;for(unsigned i=0;i<2;++i){
    put(evaluation,0x60,uint32_t(i*1280));put(evaluation,0x28,uint32_t(id*7+i));assert(invoke_evaluate()==evaluate_result);}
    ngx_output_pixels::end_ticket(id,true);
}
void warm(){for(uint64_t id=100;id<132;++id)frame(id);assert(ngx_output_pixels::delayed.stable==32&&!ngx_output_pixels::count);}
std::atomic<bool> entered{},resume_original{};std::atomic<unsigned> concurrent_calls{};
void attach_thread(){consuming_renderer=renderer.data();consuming_frozen_frame.id=consuming_frame_ticket=32;consuming_frozen_frame.world_generation=9;}
uint64_t __fastcall held_create(uintptr_t a,uintptr_t b,uintptr_t c,uintptr_t d,uintptr_t e,const void* input){
    unlocked();++concurrent_calls;entered=true;while(!resume_original.load())std::this_thread::yield();return original_create(a,b,c,d,e,input);}
uint64_t __fastcall held_evaluate(uintptr_t,uintptr_t,uintptr_t,const void*){
    unlocked();if(++concurrent_calls==1){entered=true;while(!resume_original.load())std::this_thread::yield();}return evaluate_result;}
void concurrent_cases(){using namespace ngx_output_pixels;
    delayed_reset();entered=resume_original=false;concurrent_calls=0;ngx_route_observer::original_create=held_create;
    std::thread creator([]{attach_thread();assert(invoke_create()==create_result);});while(!entered.load())std::this_thread::yield();
    assert(invoke_evaluate()==evaluate_result);resume_original=true;creator.join();assert(!delayed.epoch_valid&&!delayed.active&&concurrent_calls==1&&create_calls==2);
    delayed_reset();arm();entered=resume_original=false;concurrent_calls=0;ngx_route_observer::original_evaluate=held_evaluate;
    std::thread evaluator([]{attach_thread();assert(invoke_evaluate()==evaluate_result);});while(!entered.load())std::this_thread::yield();
    assert(invoke_evaluate()==evaluate_result);resume_original=true;evaluator.join();assert(!delayed.active&&!delayed.stable&&!count&&concurrent_calls==2);
}
void create_attestation_cases(){using namespace ngx_output_pixels;
    for(unsigned mutation=1;mutation<=5;++mutation){delayed_reset();create_mutation=mutation;
        if(mutation==1)cells_available=false; // Entry-only missing cells recover inside the normal original.
        else if(mutation==2)unreadable=reinterpret_cast<uintptr_t>(creation.data())+0x18; // Entry-only missing field.
        assert(invoke_create()==create_result&&create_calls==2&&create_probes==2);
        assert(cells_available&&!unreadable&&!delayed.epoch_valid&&delayed.reason==Reason::invalid_create&&!delayed.active);
        gears_ngx_route::Row row;row.structure=reinterpret_cast<uintptr_t>(creation.data());const auto after=ngx_route_observer::sample(row,true);
        assert(after.slots_mask==3&&after.fields_mask==127&&after.ownership.associated); // Exit-only completeness cannot rescue entry drift.
    }
    delayed_reset();handle=0;assert(invoke_create()==create_result&&create_calls==2&&create_probes==2);
    assert(delayed.epoch_valid&&delayed.handle==handle&&handle!=0); // A newly returned handle is explicitly allowed.
}
void pure_requests(){Request out;const std::string valid="schema=1\npid=7\ncreation_time=8\nstarted_qpc=9\nworld=9\nepoch=1\nnonce=1\naction=resume\n";
    assert(gears_ngx_delayed::parse(valid,out)&&out.pid==7&&out.nonce==1);
    for(size_t n=0;n<valid.size();++n)assert(!gears_ngx_delayed::parse(std::string_view(valid.data(),n),out));
    for(auto const& bad:{valid+"x",std::string(" ")+valid,std::string("schema=1\npid=18446744073709551616\n")+valid,valid+valid})assert(!gears_ngx_delayed::parse(bad,out));
    auto crlf=valid;crlf.insert(crlf.find('\n'),"\r");assert(!gears_ngx_delayed::parse(crlf,out));
    auto zero=valid;zero.replace(zero.find("nonce=1"),7,"nonce=0");assert(!gears_ngx_delayed::parse(zero,out));
    delayed_reset();auto r=identity(),own=r;
    for(unsigned field=0;field<6;++field){auto bad=r;switch(field){case 0:++bad.pid;break;case 1:++bad.creation;break;case 2:++bad.started;break;case 3:++bad.world;break;case 4:++bad.epoch;break;default:bad.nonce=0;}
        assert(!ngx_output_pixels::delayed.arm(bad,own,100,300));}
    assert(ngx_output_pixels::delayed.arm(r,own,100,300));assert(!ngx_output_pixels::delayed.arm(r,own,101,300));
    ngx_output_pixels::delayed.tick(400);assert(!ngx_output_pixels::delayed.armed&&ngx_output_pixels::delayed.reason==Reason::timeout);
    ++r.nonce;assert(ngx_output_pixels::delayed.arm(r,own,500,300));assert(invoke_create()==create_result);assert(!ngx_output_pixels::delayed.armed);
    assert(!ngx_output_pixels::delayed.arm(r,own,501,300));++r.epoch;++r.nonce;assert(ngx_output_pixels::delayed.arm(r,own,502,300));
}
void worker_requests(){using namespace ngx_output_pixels;delayed_reset();
    const wchar_t* path=L"analysis\\ngx-delayed-gate-tests\\ngx-output-arm-test.ini";
    auto write=[&](std::string const& value){FILE* file{};assert(!_wfopen_s(&file,path,L"wb"));assert(fwrite(value.data(),1,value.size(),file)==value.size());assert(!fclose(file));};
    write("schema=1\npid=7\n");worker_request(path,7,8,9,10000);assert(delayed.rejected==1&&!delayed.armed);
    worker_request(path,7,8,9,10000);assert(delayed.rejected==1); // Identical incomplete file is bounded.
    write("schema=1\npid=7\ncreation_time=8\nstarted_qpc=9\nworld=9\nepoch=1\nnonce=1\naction=resume\n");
    worker_request(path,7,8,9,10000);assert(delayed.accepted==1&&delayed.armed);const auto fixed_deadline=delayed.deadline;
    worker_request(path,7,8,9,10000);assert(delayed.accepted==1&&delayed.deadline==fixed_deadline&&delayed.rejected==1);
    assert(DeleteFileW(path));
}
void wrapper_cases(){using namespace ngx_output_pixels;
    delayed_reset();for(uint64_t id=1;id<40;++id)frame(id);assert(!delayed.accepted&&!count&&!extra_copies); // Resume required.
    arm();warm();frame(132,true);assert(delayed.closed&&count==2&&!failure&&extra_copies==2&&published&&fake_backend.publications==1&&!delayed.first_invalid.valid);
    assert(ngx_route_observer::recorder.evaluate_count==8);execute();poll();assert(!fake_backend.mapped);retire();poll();assert(fake_backend.mapped==2&&fake_backend.released==2);
    ticks=static_cast<long long>(delayed.deadline);poll();assert(delayed.finished&&!failure&&delayed.reason==Reason::complete); // Completed GPU pair is terminal.
    FILE* report{};assert(!tmpfile_s(&report));print(report);assert(contains(report,"stable_pairs=32")&&contains(report,"stage=selected_before")&&contains(report,"stage=selected_after"));fclose(report);
    // A resource/dimension/scale/base/Reset drift restarts waiting without
    // extending the single acceptance deadline. Jitter/owner rotation do not.
    for(unsigned change:{0u,1u,2u,3u,4u}){delayed_reset();arm();for(uint64_t id=100;id<103;++id)frame(id);const auto deadline=delayed.deadline;
        ticket(103);if(change==0)put(evaluation,0x30,uint32_t(777));else if(change==1)put(evaluation,0x38,uint32_t(1));
        else if(change==2)put(evaluation,0x3c,uint32_t(123));else if(change==3)put(evaluation,0x68,uint32_t(999));else put(evaluation,0x18,uintptr_t(999));
        assert(invoke_evaluate()==evaluate_result);end_ticket(103,true);assert(!delayed.stable&&!count&&delayed.deadline==deadline&&delayed.armed);}
    delayed_reset();arm();frame(100);put(context,0x14c0,reinterpret_cast<uintptr_t>(views.data())+0xd30);frame(101);assert(delayed.stable==2); // Rotating view between frames.
    delayed_reset();arm();frame(100);ticket(101);assert(invoke_evaluate()==evaluate_result);put(context,0x14c0,reinterpret_cast<uintptr_t>(views.data())+0xd30);
    assert(invoke_evaluate()==evaluate_result);end_ticket(101,true);assert(!delayed.stable); // Within-pair owner is strict.
    delayed_reset();arm();frame(100);ticket(101);assert(invoke_evaluate()==evaluate_result);end_ticket(101,true);assert(!delayed.stable); // Incomplete.
    delayed_reset();arm();frame(100);ticket(101);for(unsigned i=0;i<5;++i)assert(invoke_evaluate()==evaluate_result);end_ticket(101,true);assert(!delayed.stable); // Extra calls cannot form a replacement pair.
    delayed_reset();arm();frame(100);frame(100);assert(!delayed.stable); // Duplicate ticket cannot advance stability.
    delayed_reset();arm();frame(100);ticket(101);assert(invoke_evaluate()==evaluate_result);std::thread foreign([]{assert(invoke_evaluate()==evaluate_result);});foreign.join();
    assert(invoke_evaluate()==evaluate_result);end_ticket(101,true);assert(!delayed.stable&&!count); // Unknown foreign call blocks whole interval.
    delayed_reset();arm();warm();ticket(132);pixel_mode=1;assert(invoke_evaluate()==evaluate_result);assert(extra_copies==1&&!published); // Provisional only.
    assert(invoke_create()==create_result);assert(failure&&slots[0].life.appended&&!fake_backend.released);poll();assert(!fake_backend.mapped);
    delayed_reset();arm();warm();ticket(132);pixel_mode=1;for(unsigned i=0;i<3;++i){put(evaluation,0x60,uint32_t(i%2*1280));assert(invoke_evaluate()==evaluate_result);}
    end_ticket(132,true);assert(failure&&!published&&extra_copies==2&&!fake_backend.released); // Third Eval quarantines.
    delayed_reset();arm();warm();ticket(132);pixel_mode=3;try{invoke_evaluate();assert(false);}catch(std::runtime_error const&){}
    assert(failure&&pixel_eval_calls==65&&slots[0].life.appended&&!fake_backend.released&&active_eval==-1);
    delayed_reset();mode=3;try{invoke_create();assert(false);}catch(std::runtime_error const&){}assert(!delayed.epoch_valid&&create_calls==2);
    delayed_reset();cells_available=false;assert(invoke_create()==create_result);assert(!delayed.epoch_valid); // Missing cells never establish epoch.
    delayed_reset();ngx_route_observer::recorder={};mode=1;assert(invoke_create()==create_result);assert(!delayed.epoch_valid); // Nested Eval rejects Create epoch.
    delayed_reset();arm();pixel_mode=8;assert(invoke_evaluate()==evaluate_result);assert(!delayed.stable&&!count&&!delayed.active); // Nested, original each once.
    delayed_reset();arm();warm();ticket(132);pixel_mode=1;assert(invoke_evaluate()==evaluate_result);const auto deadline=delayed.deadline;
    ticks=static_cast<long long>(deadline);poll();assert(failure&&slots[0].life.appended&&!fake_backend.released&&!fake_backend.mapped); // Inflight timeout retains.
    delayed_reset();arm();const auto waiting_deadline=delayed.deadline;ticks=static_cast<long long>(waiting_deadline);poll();
    assert(!delayed.armed&&!count&&!failure&&!fake_backend.released&&delayed.reason==Reason::timeout);
}
void selected_open_pair(){delayed_reset();arm();warm();ticket(132);pixel_mode=1;
    for(unsigned i=0;i<2;++i){put(evaluation,0x60,uint32_t(i*1280));assert(invoke_evaluate()==evaluate_result);}
    assert(ngx_output_pixels::count==2&&!ngx_output_pixels::published&&!ngx_output_pixels::failure&&!ngx_output_pixels::delayed.first_invalid.valid);
}
void invalidation_receipt_cases(){using namespace ngx_output_pixels;using gears_ngx_delayed::Site;
    selected_open_pair();end_ticket(132,true,{true,true,false,0,0xface});
    assert(delayed.closed&&published&&!failure&&!delayed.first_invalid.valid&&fake_backend.publications==1);
    execute();retire();poll();assert(delayed.finished&&fake_backend.mapped==2&&fake_backend.released==2&&!delayed.first_invalid.valid);
    selected_open_pair();const auto before_qpc=ticks;
    end_ticket(0,false,{true,false,false,1,0xface});
    const auto first=delayed.first_invalid;
    assert(first.valid&&first.original_reason==Reason::incomplete&&first.trigger.site==Site::end_ticket&&first.trigger.qpc>uint64_t(before_qpc));
    assert(first.active==0&&first.rows==2&&first.stable==32&&first.last_ticket==132&&first.completed_ticket==131&&first.selected_ticket==132&&first.selected_world==9);
    assert(first.trigger.at_present&&!first.trigger.eligible&&first.trigger.present_ticket==0&&first.trigger.present.known&&!first.trigger.present.linked&&
           !first.trigger.present.contaminated&&first.trigger.present.rejection==1&&first.trigger.present.buffer==0xface&&!first.trigger.incoming_known);
    assert(failure==association_ambiguous&&delayed.reason==Reason::selected_invalid&&!delayed.armed&&!fake_backend.released);
    // Later Create, Eval, a second explicit Present fault, and worker stop must
    // not overwrite the original receipt or reopen/release the selected slots.
    assert(invoke_create()==create_result);assert(invoke_evaluate()==evaluate_result);end_ticket(999,false,{true,true,true,128,0xbeef});stop();
    const auto& retained=delayed.first_invalid;
    assert(retained.trigger.site==first.trigger.site&&retained.original_reason==first.original_reason&&retained.trigger.qpc==first.trigger.qpc&&
           retained.rows==first.rows&&retained.stable==first.stable&&retained.epoch==first.epoch&&retained.last_ticket==first.last_ticket&&
           retained.selected_ticket==first.selected_ticket&&retained.trigger.present.rejection==1&&retained.trigger.present.buffer==0xface&&!fake_backend.released);
    FILE* file{};assert(!tmpfile_s(&file));print_control(file);
    assert(contains(file,"invalid_site=end_ticket")&&contains(file,"invalid_original_reason=10")&&contains(file,"invalid_present_rejection=1")&&contains(file,"invalid_present_buffer=face"));fclose(file);
    selected_open_pair();end_ticket(132,false,{true,true,true,128,0xbeef});
    assert(delayed.first_invalid.trigger.present.linked&&delayed.first_invalid.trigger.present.contaminated&&!delayed.first_invalid.trigger.eligible);
    selected_open_pair();end_ticket(0,false);assert(delayed.first_invalid.valid&&delayed.first_invalid.trigger.at_present&&!delayed.first_invalid.trigger.present.known);
    selected_open_pair();ticket(133);assert(invoke_evaluate()==evaluate_result);
    assert(delayed.first_invalid.trigger.site==Site::eval_ticket_change&&delayed.first_invalid.trigger.incoming_known&&delayed.first_invalid.trigger.incoming.ticket==133&&
           delayed.first_invalid.trigger.incoming.world==9&&delayed.first_invalid.last_ticket==132&&delayed.first_invalid.rows==2);
    selected_open_pair();put(evaluation,0x60,uint32_t(0));assert(invoke_evaluate()==evaluate_result);
    assert(delayed.first_invalid.trigger.site==Site::eval_extra&&delayed.first_invalid.original_reason==Reason::incomplete&&delayed.first_invalid.trigger.incoming.ticket==132);
    selected_open_pair();assert(invoke_create()==create_result);
    assert(delayed.first_invalid.trigger.site==Site::create_entry&&delayed.first_invalid.original_reason==Reason::new_create&&delayed.first_invalid.epoch==1&&delayed.epoch==2);
    delayed_reset();arm();frame(100);ticket(101);end_ticket(0,false,{true,false,false,1,0xface});assert(!delayed.first_invalid.valid); // Waiting resets do not consume receipt storage.
}
}
#if defined(GEARS_NGX_OUTPUT_PRESERVE_TEST_INCLUDED)
#define main ngx_preserve_delayed_regression_main
#endif
int main(){pure_requests();worker_requests();wrapper_cases();concurrent_cases();create_attestation_cases();invalidation_receipt_cases();std::puts("NgxOutputDelayedTest passed (pure request/gate and actual wrappers; fake GPU, no game)");return 0;}
#if defined(GEARS_NGX_OUTPUT_PRESERVE_TEST_INCLUDED)
#undef main
#endif
