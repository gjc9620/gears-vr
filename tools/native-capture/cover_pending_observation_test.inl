// Runs after an actual Host Native/Kernel/Compose consumed-cover transaction.
{
    namespace pending_obs=live::cover_pending_observation;
    const auto prior_clock=host_now;
    const auto debt=current.kernel.pending();const auto mask=current.kernel.outstanding_mask();
    C(debt>0&&!live::stopping.load()&&!pending_obs::ready.load());
    live::diagnostic_first_failure=false;live::note_pending(entry);C(pending_obs::first.count==0);
    live::diagnostic_first_failure=true;
    std::thread foreign([&]{live::note_pending(entry);});foreign.join();C(pending_obs::first.count==0);
    const auto flags=current.context.source.flags;current.context.source.flags&=~gears_rig_tracking::cover_key_known;
    live::note_pending(entry);C(pending_obs::first.count==0);current.context.source.flags=flags;
    for(unsigned i=0;i<32;++i){
        host_now+=pending_obs::sample_gap_ns;live::note_pending(entry);
        C(pending_obs::first.count==i+1);
        if(i<31)C(!pending_obs::ready.load());
        live::note_pending(entry);C(pending_obs::first.count==i+1); // Same-time retry cannot fill the archive.
    }
    C(pending_obs::ready.load()&&pending_obs::first.serial==current.kernel.serial());
    C(pending_obs::first.frames[31].observed.fields==current.kernel.outstanding_fields());
    C(pending_obs::first.frames[31].observed.consumed);
    auto saved=std::make_unique<pending_obs::Record>(pending_obs::first);
    host_now+=pending_obs::sample_gap_ns;live::note_pending(entry);
    C(!std::memcmp(saved.get(),&pending_obs::first,sizeof *saved));
    pending_obs::export_record();C(!pending_obs::export_attempted.load()); // GT never writes a file.
    std::thread collector([]{pending_obs::export_record();});collector.join();
    C(pending_obs::export_written.load()&&!pending_obs::export_error.load());
    wchar_t archive[MAX_PATH]{};C(swprintf_s(archive,L"%s\\cover-pending-observation.json",path)>0);
    FILE* fp{};C(_wfopen_s(&fp,archive,L"rb")==0&&fp);
    if(fp){C(fseek(fp,0,SEEK_END)==0);const auto length=ftell(fp);C(length>0&&fseek(fp,0,SEEK_SET)==0);
        std::string contents(size_t(length),'\0');C(fread(contents.data(),1,contents.size(),fp)==contents.size()&&fgetc(fp)==EOF);
        C(fclose(fp)==0);C(contents.find("\"schema\":1")!=std::string::npos&&contents.find("\"sample_count\":32")!=std::string::npos);
        C(contents.find("\"pending_frames\":[{")!=std::string::npos&&contents.find("\"consumed\":1")!=std::string::npos);
    }
    C(DeleteFileW(archive));C(current.kernel.pending()==debt);
    const auto after=current.kernel.outstanding_mask();C(!std::memcmp(&mask,&after,sizeof mask));
    C(!std::memcmp(saved.get(),&pending_obs::first,sizeof *saved));
    host_now=prior_clock; // Synthetic fixture clock; keep later view-age checks independent.
}
