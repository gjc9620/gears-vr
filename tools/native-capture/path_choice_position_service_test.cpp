// Synthetic execution of native service worker/owner wrappers. No game/DLL/input.
#define GEARS_PATH_CHOICE_POSITION_SERVICE 1
#define GEARS_PATH_CHOICE_POSITION_SERVICE_TEST 1
#define GEARS_PATH_CHOICE_REFLOW_CONTROL 1
#include "path_choice_close_restore_test.cpp"
static std::string position_wire(trial::Config const& c){
    std::string text="[PathChoicePositionTrial]\n";
    auto number=[&](char const* k,uint64_t v){text+=k;text+='=';text+=std::to_string(v);text+='\n';};
    auto address=[&](char const* k,uintptr_t v){char b[32]{};sprintf_s(b,"0x%llx",static_cast<unsigned long long>(v));text+=k;text+='=';text+=b;text+='\n';};
    number("schema",c.schema);number("nonce",c.nonce);number("pid",c.pid);number("creation_time",c.creation_time);number("started_qpc",c.started_qpc);
    number("world_generation",c.world_generation);address("base",c.base);address("client",c.client);address("scene",c.scene);address("panel",c.panel);
    address("panel_vtable",c.panel_vtable);number("top_bits",c.top_bits);number("bottom_bits",c.bottom_bits);number("duration_ms",c.duration_ms);
    if(c.schema==2)text+="registry_header_policy=fixed_prefix_v1\n";
    if(c.vertical_offset_reference_pixels!=16)number("vertical_offset_reference_pixels",c.vertical_offset_reference_pixels);
    number("check_count",c.checks.size());unsigned index{};
    for(auto const& check:c.checks){char b[32]{};sprintf_s(b,"0x%llx:",static_cast<unsigned long long>(check.address));
        text+="check_"+std::to_string(index++)+'='+b;
        for(auto byte:check.bytes){char digits[3]{};sprintf_s(digits,"%02x",unsigned(byte));text+=digits;}text+='\n';}
    return text;
}
struct ServiceFixture:RestoreFixture {
    std::string pw,cw;
    ServiceFixture(){
        position::disable();position::state={};position::applied=0;position::pending_restore=false;position::ever_modified=false;
        position::active_nonce=0;position::consumed_nonce_ack=0;position::last_restored_nonce=0;position::last_restored_tick=0;position::loaded_nonce=0;
        position::last_poll=0;position::seen_file={};position::service_renewed=0;
        std::atomic_store(&position::last_request_receipt,std::shared_ptr<position::Request const>{});
        close_obs::disable();close_obs::loaded_nonce=0;close_obs::last_poll=0;close_obs::seen_file={};close_obs::load_rejected=0;
        std::atomic_store(&close_obs::last_request_receipt,std::shared_ptr<close_obs::Active const>{});
        f.c.schema=2;f.c.registry_header_policy=trial::RegistryHeaderPolicy::fixed_prefix_v1;f.c.vertical_offset_reference_pixels=374;f.c.duration_ms=6000;
        put(f.c.panel+0x64,f.c.top_bits);put(f.c.panel+0x6c,f.c.bottom_bits);
        pw=position_wire(f.c);cw=request_text(request);trial::Config parsed;assert(trial::parse(pw,parsed));
    }
    void load(){std::exception_ptr error;std::thread worker([&]{try{
            close_obs::load_request(cw,GetTickCount64(),safe_copy,[]{return true;});
            position::load_request(pw,GetTickCount64(),safe_copy,[]{return true;});
        }catch(...){error=std::current_exception();}});worker.join();if(error)std::rethrow_exception(error);
        assert(position::loaded_nonce==f.c.nonce&&close_obs::loaded_nonce==f.c.nonce&&position::active_nonce==0&&position::state.consumed_nonce==0);}
    void apply_owner(){struct Candidate{uintptr_t client,scene;} c{f.c.client,f.c.scene};
        assert(position::invoke_with(c,[]{return sentinel;},safe_copy,f.setter(),[]{return true;})==sentinel);
        assert(position::active_nonce==f.c.nonce&&position::consumed_nonce_ack==f.c.nonce&&position::last_restored_nonce==0&&position::pending_restore);}
};
static void renew_both(ServiceFixture& f){
    std::exception_ptr error;std::thread worker([&]{try{close_obs::load_request(f.cw,GetTickCount64(),safe_copy,[]{return true;});
        position::load_request(f.pw,GetTickCount64(),safe_copy,[]{return true;});}catch(...){error=std::current_exception();}});worker.join();if(error)std::rethrow_exception(error);
}
static void first_renew_close_and_no_resurrection(){
    ServiceFixture f;f.load();f.apply_owner();auto owned=f.positions();close_obs::slots=2;close_obs::restore_slots=2;
    for(unsigned i=0;i<6;++i){renew_both(f);assert(f.positions()==owned&&position::applied==1&&position::restored==0);}
    auto p=std::atomic_load(&position::selected);auto c=std::atomic_load(&close_obs::selected);
    assert(p->wire==f.pw&&c->wire==f.cw&&p->renewals==6&&c->renewals==6&&position::service_renewed==6);
    assert(p->expires==p->accepted_tick+6000&&c->expires==c->accepted_tick+8000&&p->first_tick<=p->accepted_tick);
    unsigned originals{};assert(f.run([&](void*,void*){++originals;f.original_bits();return sentinel;})==sentinel);
    assert(originals==1&&position::last_restored_nonce==f.f.c.nonce&&position::active_nonce==0&&position::last_restored_tick>0&&!position::unresolved);
    assert(std::atomic_load(&position::last_request_receipt)->renewals==6&&std::atomic_load(&close_obs::last_request_receipt)->renewals==6);
    try{position::load_request(f.pw,GetTickCount64(),safe_copy,[]{return true;});assert(false);}catch(position::CompletedRenewal const& completed){assert(completed.nonce==f.f.c.nonce);}
    try{close_obs::load_request(f.cw,GetTickCount64(),safe_copy,[]{return true;});assert(false);}catch(position::CompletedRenewal const& completed){assert(completed.nonce==f.f.c.nonce);}
    // Even a stale selected pointer that races the restore cannot resurrect this consumed nonce.
    std::atomic_store(&position::selected,p);struct Candidate{uintptr_t client,scene;} candidate{f.f.c.client,f.f.c.scene};unsigned sets{};
    assert(position::invoke_with(candidate,[]{return sentinel;},safe_copy,[&](auto,float,unsigned,unsigned,int){++sets;return true;},[]{return true;})==sentinel);
    assert(sets==0&&position::applied==1&&position::restored==1&&session_stop_calls==0);
}
static void renewal_rejections(){
    for(unsigned kind=0;kind<13;++kind){ServiceFixture f;f.load();f.apply_owner();auto before=f.positions();auto text=f.pw;auto close_text=f.cw;
        if(kind==0){text+='\n';close_text+='\n';} // Even semantically identical altered wire is denied.
        if(kind==1){auto at=text.find("duration_ms=6000");text.replace(at,16,"duration_ms=5000");auto at2=close_text.find("duration_ms=8000");close_text.replace(at2,16,"duration_ms=7000");}
        if(kind==2){auto p=std::make_shared<position::Request>(*std::atomic_load(&position::selected));p->expires=0;std::atomic_store(&position::selected,std::shared_ptr<position::Request const>(p));
            auto c=std::make_shared<close_obs::Active>(*std::atomic_load(&close_obs::selected));c->expires=0;std::atomic_store(&close_obs::selected,std::shared_ptr<close_obs::Active const>(c));}
        if(kind==3)++committed_world_generation;
        if(kind==4)++session_started_qpc;
        if(kind==5){text+="unknown=1\n";close_text+="unknown=1\n";}
        if(kind==6){position::disable();close_obs::disable();}
        if(kind==7)position::active_nonce=0;
        if(kind==8)position::consumed_nonce_ack=0;
        if(kind==9)position::last_restored_nonce=f.f.c.nonce;
        if(kind==10)memory[0x5000]^=1;
        if(kind==11){auto p=std::make_shared<position::Request>(*std::atomic_load(&position::selected));p->renewals=UINT64_MAX;std::atomic_store(&position::selected,std::shared_ptr<position::Request const>(p));
            auto c=std::make_shared<close_obs::Active>(*std::atomic_load(&close_obs::selected));c->renewals=UINT64_MAX;std::atomic_store(&close_obs::selected,std::shared_ptr<close_obs::Active const>(c));}
        if(kind==12){auto p=std::make_shared<position::Request>(*std::atomic_load(&position::selected));p->expires=0;std::atomic_store(&position::selected,std::shared_ptr<position::Request const>(p));}
        bool rejected_p=false,rejected_c=false;
        std::thread worker([&]{try{position::load_request(text,GetTickCount64(),safe_copy,[]{return true;});}catch(...){rejected_p=true;}
            try{close_obs::load_request(close_text,GetTickCount64(),safe_copy,[]{return true;});}catch(...){rejected_c=true;}});worker.join();
        assert(rejected_p&&rejected_c&&f.positions()==before&&position::service_renewed==0&&session_stop_calls==0);
        if(kind==4)--session_started_qpc;
    }
    {ServiceFixture f;f.load();try{position::load_request(f.pw,GetTickCount64(),safe_copy,[]{return true;});assert(false);}catch(int){}
        assert(position::active_nonce==0&&position::state.consumed_nonce==0);}
}
static void prefix_growth_renewal(){
    ServiceFixture f;f.load();f.apply_owner();
    for(int count:{200,180,128}){put(base+0x1c24730,trial::RegistryHeader{0x9000,count,256});put(base+0x1c34800,trial::RegistryHeader{0xa000,count,256});
        renew_both(f);assert(position::active_nonce==1&&position::service_renewed>0&&session_stop_calls==0);}
    assert(std::atomic_load(&position::selected)->renewals==3&&std::atomic_load(&close_obs::selected)->renewals==3);
}
static void boundary_expiry_context_and_first_publish(){
    for(bool close_worker:{false,true})for(unsigned variant=0;variant<3;++variant){ServiceFixture f;f.load();f.apply_owner();bool denied=false;
        if(variant==0){if(close_worker){auto p=std::make_shared<close_obs::Active>(*std::atomic_load(&close_obs::selected));p->expires=GetTickCount64()+10;
                std::atomic_store(&close_obs::selected,std::shared_ptr<close_obs::Active const>(p));}
            else{auto p=std::make_shared<position::Request>(*std::atomic_load(&position::selected));p->expires=GetTickCount64()+10;
                std::atomic_store(&position::selected,std::shared_ptr<position::Request const>(p));}}
        auto certify=[&]{if(variant==0)Sleep(30);else if(variant==1)++session_started_qpc;else ++committed_world_generation;return true;};
        try{if(close_worker)close_obs::load_request(f.cw,GetTickCount64(),safe_copy,certify);else position::load_request(f.pw,GetTickCount64(),safe_copy,certify);}catch(...){denied=true;}
        assert(denied&&position::service_renewed==0&&session_stop_calls==0);if(variant==1)--session_started_qpc;
    }
    for(bool close_worker:{false,true}){ServiceFixture f;auto text=close_worker?f.cw:f.pw;
        const auto original="pid="+std::to_string(f.f.c.pid);auto at=text.find(original);assert(at!=std::string::npos);
        text.replace(at,original.size(),"pid="+std::to_string(f.f.c.pid+1));bool denied=false;
        try{if(close_worker)close_obs::load_request(text,GetTickCount64(),safe_copy,[]{return true;});else position::load_request(text,GetTickCount64(),safe_copy,[]{return true;});}catch(...){denied=true;}
        assert(denied&&position::loaded_nonce==0&&close_obs::loaded_nonce==0&&position::active_nonce==0&&session_stop_calls==0);
    }
}
struct TempRequest {
    wchar_t path[MAX_PATH]{};FILE* log{};
    TempRequest(){wchar_t dir[MAX_PATH]{};assert(GetTempPathW(MAX_PATH,dir)&&GetTempFileNameW(dir,L"p99",0,path));assert(!tmpfile_s(&log)&&log);}
    void write(std::string const& text){FILE* output{};assert(!_wfopen_s(&output,path,L"wb")&&output);assert(fwrite(text.data(),1,text.size(),output)==text.size());fclose(output);}
    ~TempRequest(){DeleteFileW(path);fclose(log);}
};
static void actual_poll_and_unknown_file_isolation(){
    {ServiceFixture f;TempRequest position_file,close_file;position_file.write(f.pw);close_file.write(f.cw);
        std::thread worker([&]{close_obs::poll_with(close_file.path,close_file.log,safe_copy,[]{return true;});position::poll_with(position_file.path,position_file.log,safe_copy,[]{return true;});});worker.join();
        assert(position::loaded_nonce==1&&close_obs::loaded_nonce==1&&position::state.consumed_nonce==0);f.apply_owner();
        position::seen_file={};position::last_poll=0;close_obs::seen_file={};close_obs::last_poll=0;
        std::thread renewal([&]{close_obs::poll_with(close_file.path,close_file.log,safe_copy,[]{return true;});position::poll_with(position_file.path,position_file.log,safe_copy,[]{return true;});});renewal.join();
        assert(std::atomic_load(&position::selected)->renewals==1&&std::atomic_load(&close_obs::selected)->renewals==1&&session_stop_calls==0);
        position_file.write("unknown replacement\n");position::seen_file={};position::last_poll=0;
        std::thread bad([&]{position::poll_with(position_file.path,position_file.log,safe_copy,[]{return true;});});bad.join();
        assert(position::terminal&&position::unresolved&&position::pending_restore&&!std::atomic_load(&position::selected)&&position::rejected>0&&session_stop_calls==0);
        assert(std::atomic_load(&position::last_request_receipt)->renewals==1);}
    {ServiceFixture f;f.load();f.apply_owner();TempRequest close_file;close_file.write(f.cw+"\n");
        std::thread bad([&]{close_obs::poll_with(close_file.path,close_file.log,safe_copy,[]{return true;});});bad.join();
        assert(close_obs::load_rejected==1&&position::terminal&&position::unresolved&&!std::atomic_load(&position::selected)&&session_stop_calls==0);}
}
static void owner_completion_during_worker_certification(){
    for(bool close_worker:{false,true}){ServiceFixture f;f.load();f.apply_owner();std::promise<void> cert_entered,release_cert;auto wait=release_cert.get_future().share();bool denied=false;
        auto certify=[&]{cert_entered.set_value();wait.wait();return true;};
        std::thread worker([&]{try{if(close_worker)close_obs::load_request(f.cw,GetTickCount64(),safe_copy,certify);
            else position::load_request(f.pw,GetTickCount64(),safe_copy,certify);}catch(...){denied=true;}});
        cert_entered.get_future().wait();unsigned originals{};assert(f.run([&](void*,void*){++originals;return sentinel;})==sentinel);
        release_cert.set_value();worker.join();assert(denied&&originals==1&&position::active_nonce==0&&position::last_restored_nonce==1&&position::restored==1&&session_stop_calls==0);
    }
}
// Exact immutable renewals may arrive before or finish certification after a
// normal owner completion. They must not reopen or poison that finished lease.
static void completed_owner_poll_stage_diagnostic(){
    for(bool during_certification:{false,true}){
        ServiceFixture f;position::rejected=0;f.load();f.apply_owner();TempRequest file;file.write(f.pw);
        std::promise<void> cert_arrived,release;auto wait=release.get_future().share();
        auto certify=[&]{if(during_certification){cert_arrived.set_value();wait.wait();}return true;};
        auto restore=[&]{unsigned originals{};assert(f.run([&](void*,void*){++originals;return sentinel;})==sentinel);
            assert(originals==1&&position::restored==1&&position::active_nonce==0&&!position::pending_restore&&!position::terminal);};
        if(!during_certification)restore();
        std::thread worker([&]{position::poll_with(file.path,file.log,safe_copy,certify);});
        if(during_certification){cert_arrived.get_future().wait();restore();release.set_value();}
        worker.join();
        assert(position::rejected==0&&!position::terminal&&!position::unresolved&&position::restored==1&&!position::pending_restore);
        assert(position::service_renewed==0&&session_stop_calls==0&&position::active_nonce==0&&position::consumed_nonce_ack==1);
        assert(std::atomic_load(&position::selected)->renewals==0);
        rewind(file.log);std::string log;char buffer[256]{};while(auto n=fread(buffer,1,sizeof(buffer),file.log))log.append(buffer,n);
        assert(log.find(during_certification?"stage=completed_after_auth":"stage=completed_before_auth")!=std::string::npos);
    }
}
static void completed_renewal_proof_cannot_mask_real_failures(){
    for(unsigned variant=0;variant<8;++variant){
        ServiceFixture f;position::rejected=0;f.load();f.apply_owner();
        assert(f.run([&](void*,void*){return sentinel;})==sentinel);
        auto wire=f.pw;
        if(variant==0)wire+='\n';
        if(variant==1)position::consumed_nonce_ack=0;
        if(variant==2)position::active_nonce=1;
        if(variant==3)position::last_restored_nonce=2;
        if(variant==4)++committed_world_generation;
        if(variant==5)position::last_restored_tick=0;
        if(variant==6)position::pending_restore=true;
        if(variant==7)++session_started_qpc;
        TempRequest file;file.write(wire);std::thread worker([&]{position::poll_with(file.path,file.log,safe_copy,[]{return true;});});worker.join();
        assert(position::rejected==1&&position::terminal&&position::unresolved&&position::restored==1&&position::service_renewed==0);
        if(variant==7)--session_started_qpc;
    }
    for(bool cert_ok:{false,true}){
        ServiceFixture f;position::rejected=0;f.load();f.apply_owner();TempRequest file;file.write(f.pw);
        std::promise<void> cert_arrived,release;auto wait=release.get_future().share();
        auto certify=[&]{cert_arrived.set_value();wait.wait();return cert_ok;};
        std::thread worker([&]{position::poll_with(file.path,file.log,safe_copy,certify);});
        cert_arrived.get_future().wait();assert(f.run([&](void*,void*){return sentinel;})==sentinel);
        // A false authentication result is not accepted: the separate native
        // completion receipt discards only this previously accepted renewal.
        release.set_value();worker.join();assert(!position::terminal&&!position::unresolved&&position::rejected==0&&position::restored==1);
    }
    for(unsigned bad_read_mode=0;bad_read_mode<3;++bad_read_mode){
        ServiceFixture f;position::rejected=0;f.load();f.apply_owner();TempRequest file;file.write(bad_read_mode==2?"invalid request bytes":f.pw);
        auto failed_read=[&](void*,void const*,std::size_t)->bool{if(bad_read_mode==1)throw 7;return false;};
        std::thread worker([&]{position::poll_with(file.path,file.log,failed_read,[]{return true;});});worker.join();
        assert(position::terminal&&position::unresolved&&position::rejected==1&&position::restored==0&&position::pending_restore);
    }
    for(bool already_terminal:{false,true}){
        ServiceFixture f;position::rejected=0;f.load();f.apply_owner();assert(f.run([&](void*,void*){return sentinel;})==sentinel);
        TempRequest file;file.write(f.pw);position::terminal=already_terminal;position::unresolved=true;position::drain_requested=!already_terminal;
        std::thread worker([&]{position::poll_with(file.path,file.log,safe_copy,[]{return true;});});worker.join();
        assert(position::terminal==already_terminal&&position::unresolved&&position::rejected==0&&position::service_renewed==0);
    }

}
static void normal_close_during_authentication_false_is_retired(){
    ServiceFixture f;position::rejected=0;f.load();f.apply_owner();TempRequest file;file.write(f.pw);
    std::promise<void> read_arrived,release_read;auto wait=release_read.get_future().share();
    auto read=[&](void*,void const*,std::size_t){read_arrived.set_value();wait.wait();return false;};
    std::thread worker([&]{position::poll_with(file.path,file.log,read,[]{return true;});});
    read_arrived.get_future().wait();assert(f.run([&](void*,void*){return sentinel;})==sentinel);
    assert(position::last_restored_nonce==1&&!position::pending_restore&&position::restored==1);
    release_read.set_value();worker.join();
    assert(position::rejected==0&&!position::terminal&&!position::unresolved&&position::service_renewed==0);
    assert(position::active_nonce==0&&std::atomic_load(&position::selected)->renewals==0);
    rewind(file.log);std::string log;char buffer[256]{};while(auto n=fread(buffer,1,sizeof(buffer),file.log))log.append(buffer,n);
    assert(log.find("stage=completed_during_auth")!=std::string::npos);
    {ServiceFixture ongoing;position::rejected=0;ongoing.load();ongoing.apply_owner();TempRequest invalid;invalid.write(ongoing.pw);
        std::thread rejected_worker([&]{position::poll_with(invalid.path,invalid.log,safe_copy,[]{return false;});});rejected_worker.join();
        assert(position::terminal&&position::unresolved&&position::rejected==1&&position::restored==0&&position::pending_restore);}
}
static void close_worker_completed_poll_is_retired(){
    for(unsigned phase=0;phase<4;++phase){
        ServiceFixture f;position::rejected=0;f.load();f.apply_owner();TempRequest file;file.write(f.cw);
        auto prior_close=std::atomic_load(&close_obs::selected);auto prior_receipt=std::atomic_load(&close_obs::last_request_receipt);
        std::promise<void> boundary_arrived,release_boundary;auto wait=release_boundary.get_future().share();
        auto read=[&](void* to,void const* from,std::size_t size){if(phase==1){boundary_arrived.set_value();wait.wait();return false;}return safe_copy(to,from,size);};
        auto certify=[&]{if(phase>=2){boundary_arrived.set_value();wait.wait();}return phase!=3;};
        auto restore=[&]{assert(f.run([&](void*,void*){return sentinel;})==sentinel);assert(position::restored==1&&!position::pending_restore);};
        if(phase==0)restore();
        std::thread worker([&]{close_obs::poll_with(file.path,file.log,read,certify);});
        if(phase){boundary_arrived.get_future().wait();restore();release_boundary.set_value();}worker.join();
        assert(close_obs::load_rejected==0&&position::rejected==0&&!position::terminal&&!position::unresolved);
        assert(position::active_nonce==0&&position::last_restored_nonce==1&&position::consumed_nonce_ack==1&&position::restored==1);
        assert(std::atomic_load(&close_obs::selected)==prior_close&&std::atomic_load(&close_obs::last_request_receipt)==prior_receipt);
        rewind(file.log);std::string log;char buffer[256]{};while(auto n=fread(buffer,1,sizeof(buffer),file.log))log.append(buffer,n);
        const char* expected[]={"close_completed_before_auth","close_completed_during_position_auth","close_completed_after_auth","close_completed_during_cert"};
        assert(log.find(expected[phase])!=std::string::npos);
    }
}
static void close_worker_completion_requires_own_immutable_request(){
    for(unsigned variant=0;variant<11;++variant){
        ServiceFixture f;f.load();f.apply_owner();assert(f.run([&](void*,void*){return sentinel;})==sentinel);
        auto wire=f.cw;
        if(variant==0)wire+='\n';
        if(variant==1){auto prior=std::make_shared<close_obs::Active>(*std::atomic_load(&close_obs::selected));++prior->request.nonce;std::atomic_store(&close_obs::selected,std::shared_ptr<close_obs::Active const>(prior));}
        if(variant==2){auto prior=std::make_shared<close_obs::Active>(*std::atomic_load(&close_obs::selected));prior->wire+='\n';std::atomic_store(&close_obs::selected,std::shared_ptr<close_obs::Active const>(prior));}
        if(variant==3){auto prior=std::make_shared<position::Request>(*std::atomic_load(&position::last_request_receipt));prior->config.panel+=8;std::atomic_store(&position::last_request_receipt,std::shared_ptr<position::Request const>(prior));}
        if(variant==4)position::consumed_nonce_ack=0;
        if(variant==5)position::active_nonce=1;
        if(variant==6)position::last_restored_nonce=2;
        if(variant==7)position::pending_restore=true;
        if(variant==8)position::last_restored_tick=0;
        if(variant==9)++committed_world_generation;
        if(variant==10)++session_started_qpc;
        TempRequest file;file.write(wire);std::thread worker([&]{close_obs::poll_with(file.path,file.log,safe_copy,[]{return true;});});worker.join();
        assert(close_obs::load_rejected==1&&position::terminal&&position::unresolved&&position::restored==1);
        if(variant==10)--session_started_qpc;
    }
    for(unsigned failure=0;failure<4;++failure){
        ServiceFixture f;f.load();f.apply_owner();TempRequest file;file.write(failure==2?"invalid close request":f.cw);
        auto read=[&](void* to,void const* from,std::size_t size){if(failure==0)return false;if(failure==1)throw 7;return safe_copy(to,from,size);};
        std::thread worker([&]{close_obs::poll_with(file.path,file.log,read,[&]{return failure!=3;});});worker.join();
        assert(close_obs::load_rejected==1&&position::terminal&&position::unresolved&&position::restored==0&&position::pending_restore);
    }
}
static void product_failures_never_stop_vr(){
    for(unsigned kind=0;kind<4;++kind){ServiceFixture f;f.load();f.apply_owner();struct Candidate{uintptr_t client,scene;} c{f.f.c.client,f.f.c.scene};unsigned originals{},sets{};
        if(kind==0)memory[0x5000]^=1;
        if(kind==1)put(f.f.c.panel+0x64,position::state.lease.owned[0]+1);
        if(kind==2)++committed_world_generation;
        if(kind==3)position::disable();
        auto setter=[&](auto,float,unsigned,unsigned,int){++sets;return false;};
        assert(position::invoke_with(c,[&]{++originals;return sentinel;},safe_copy,setter,[]{return true;})==sentinel);
        assert(originals==1&&position::terminal&&position::unresolved&&session_stop_calls==0&&position::last_restored_nonce==0);
        auto previous_sets=sets;position::invoke_with(c,[]{return sentinel;},safe_copy,setter,[]{return true;});assert(sets==previous_sets);
    }
    {ServiceFixture f;f.load();f.apply_owner();unsigned calls{};auto setter=[&](auto panel,float v,unsigned face,unsigned input,int origin){if(face==3)return false;return f.f.setter()(panel,v,face,input,origin);};
        assert(f.run([&](void*,void*){++calls;return sentinel;},setter)==sentinel);
        assert(calls==1&&position::setter_failed==1&&position::terminal&&position::unresolved&&position::pending_restore&&position::active_nonce==1&&session_stop_calls==0);}
    {ServiceFixture f;f.load();f.apply_owner();bool wrote=false;auto read=[&](void* to,void const* from,std::size_t n){if(wrote)throw 9;return safe_copy(to,from,n);};
        auto setter=[&](auto panel,float v,unsigned face,unsigned input,int origin){auto ok=f.f.setter()(panel,v,face,input,origin);wrote=true;return ok;};
        assert(f.run([](void*,void*){return sentinel;},read,setter,[]{return true;})==sentinel);
        assert(position::terminal&&position::unresolved&&position::pending_restore&&session_stop_calls==0);}
    {ServiceFixture f;f.load();f.apply_owner();unsigned writes{},calls{};
        auto setter=[&](auto panel,float v,unsigned face,unsigned input,int origin){++writes;const auto ok=f.f.setter()(panel,v,face,input,origin);
            position::isolate_ui_failure();return ok;};
        assert(f.run([&](void*,void*){++calls;return sentinel;},setter)==sentinel);
        assert(writes==1&&calls==1&&position::terminal&&position::unresolved&&position::pending_restore&&session_stop_calls==0);
        assert(!position::state.lease.touched[0]&&position::state.lease.touched[1]);}
    {ServiceFixture f;f.load();f.apply_owner();observer::slots=observer::row_limit;auto before=position::state.lease.owned;
        // Fixture's raw region metadata intentionally violates the controlled contract.
        assert(observer::hook(expected_object)==sentinel);
        assert(observer::reflow_control::failed==1&&position::terminal&&position::unresolved&&position::active_nonce==0&&session_stop_calls==0);
        assert(position::state.lease.owned==before);}
}
static void expiry_and_ack_status(){
    ServiceFixture f;f.load();f.apply_owner();auto req=std::make_shared<position::Request>(*std::atomic_load(&position::selected));req->expires=0;
    std::atomic_store(&position::selected,std::shared_ptr<position::Request const>(req));struct Candidate{uintptr_t client,scene;} c{f.f.c.client,f.f.c.scene};
    assert(position::invoke_with(c,[]{return sentinel;},safe_copy,f.f.setter(),[]{return true;})==sentinel);
    assert(position::last_restored_nonce==1&&position::active_nonce==0&&position::consumed_nonce_ack==1&&!position::pending_restore&&!position::unresolved);
    FILE* log{};assert(!tmpfile_s(&log)&&log);position::progress(log);close_obs::progress(log);rewind(log);std::string text;char b[1024]{};
    while(auto n=fread(b,1,sizeof(b),log))text.append(b,n);fclose(log);
    for(auto key:{"path_position_service_compiled=1","path_position_active_nonce=0","path_position_last_restored_nonce=1","path_position_expires_tick=","path_position_renewals=0","path_close_expires_tick=","path_close_renewals=0"})assert(text.find(key)!=std::string::npos);
}
int main(){first_renew_close_and_no_resurrection();renewal_rejections();prefix_growth_renewal();boundary_expiry_context_and_first_publish();actual_poll_and_unknown_file_isolation();owner_completion_during_worker_certification();completed_owner_poll_stage_diagnostic();completed_renewal_proof_cannot_mask_real_failures();normal_close_during_authentication_false_is_retired();close_worker_completed_poll_is_retired();close_worker_completion_requires_own_immutable_request();product_failures_never_stop_vr();expiry_and_ack_status();
    std::puts("PathChoicePositionServiceTest passed");return 0;}
