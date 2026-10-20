// Seal our own patches once. Changed entries never become a new baseline.
std::array<gears_hook_entry::Proof,session_hook_entries.size()> hook_entry_proofs{};
unsigned hook_entry_proof_count{};
bool hook_entry_manifest_attempted{};
std::atomic<bool> hook_entry_manifest_ready{};
bool seal_session_hook_manifest(FILE* status){
    if(!continuous_session||hook_entry_manifest_attempted)return false;
    hook_entry_manifest_attempted=true;
    for(unsigned i=0;i<session_hook_entry_count;++i){
        auto const& entry=session_hook_entries[i];HMODULE detour_module{};
        if(!entry.detour||!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
              reinterpret_cast<LPCWSTR>(entry.detour),&detour_module)||detour_module!=own_module)continue;
        gears_hook_entry::Proof proof{};
        if(gears_hook_entry::seal(reinterpret_cast<uintptr_t>(entry.target),reinterpret_cast<uintptr_t>(entry.detour),entry.bytes,safe_copy,proof))
            hook_entry_proofs[hook_entry_proof_count++]=proof;
    }
    // reserve_ui_hooks checked original constants before creating trampolines.
    if(ui_observer_enabled)for(unsigned i=0;i<ui_targets.size();++i){bool found=false;
        for(unsigned j=0;j<hook_entry_proof_count;++j){auto const& proof=hook_entry_proofs[j];
            if(proof.target==reinterpret_cast<uintptr_t>(ui_targets[i])&&proof.detour==reinterpret_cast<uintptr_t>(ui_detours[i])&&
               std::equal(gears_ui::binding_bytes[i].begin(),gears_ui::binding_bytes[i].end(),proof.original32.begin()))found=true;}
        if(!found){fprintf(status,"hook_entry_manifest available=0 missing_ui=%u\n",i);return false;}
    }
#if defined(GEARS_UI_CALL_BOUNDARY_OBSERVER) && GEARS_UI_CALL_BOUNDARY_OBSERVER
    if(ui_observer_enabled){bool found=false;
        for(unsigned j=0;j<hook_entry_proof_count;++j){auto const& proof=hook_entry_proofs[j];
            if(proof.target==reinterpret_cast<uintptr_t>(ui_call_boundary::target)&&
               proof.detour==reinterpret_cast<uintptr_t>(ui_call_boundary::hook)&&
               proof.original32==ui_call_boundary::entry_bytes)found=true;}
        if(!found){fprintf(status,"hook_entry_manifest available=0 missing_ui_call_boundary=1\n");return false;}
    }
#endif
    FILETIME creation{},exit{},kernel{},user{};
    if(!hook_entry_proof_count||!GetProcessTimes(GetCurrentProcess(),&creation,&exit,&kernel,&user))return false;
    wchar_t temporary[MAX_PATH]{},destination[MAX_PATH]{};
    path_for(temporary,L"hook-entry-manifest.json.tmp");path_for(destination,L"hook-entry-manifest.json");
    FILE* output=_wfsopen(temporary,L"wb",_SH_DENYWR);if(!output)return false;
    fprintf(output,"{\"schema\":1,\"pid\":%lu,\"game_creation\":%llu,\"owner_pid\":%lu,\"owner_creation\":%llu,\"module_base\":%llu,\"native_base\":%llu,\"manifest_generation\":%llu,\"entries\":[",
        GetCurrentProcessId(),static_cast<unsigned long long>(gears_session::creation_time(creation)),session_owner_pid,
        static_cast<unsigned long long>(session_owner_creation_time),static_cast<unsigned long long>(base),
        static_cast<unsigned long long>(reinterpret_cast<uintptr_t>(own_module)),static_cast<unsigned long long>(now()));
    for(unsigned i=0;i<hook_entry_proof_count;++i){auto const& proof=hook_entry_proofs[i];
        fprintf(output,"%s{\"target\":%llu,\"detour\":%llu,\"relay\":%llu,\"original\":\"",i?",":"",
            static_cast<unsigned long long>(proof.target),static_cast<unsigned long long>(proof.detour),static_cast<unsigned long long>(proof.relay));
        for(auto value:proof.original32)fprintf(output,"%02x",value);
        fprintf(output,"\",\"patched\":\"");for(auto value:proof.patched32)fprintf(output,"%02x",value);
        fprintf(output,"\",\"relay_bytes\":\"");for(auto value:proof.relay14)fprintf(output,"%02x",value);fprintf(output,"\"}");
    }
    fprintf(output,"]}\n");bool written=!ferror(output)&&fflush(output)==0;
    if(fclose(output))written=false;
    if(written)written=MoveFileExW(temporary,destination,MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH)!=0;
    if(!written){DeleteFileW(temporary);return false;}
    hook_entry_manifest_ready=true;
    fprintf(status,"hook_entry_manifest available=1 entries=%u immutable=1\n",hook_entry_proof_count);return true;
}
// Separate from safe_copy: UObject/property/unlisted code always stays live.
bool read_attested_hook_entry(void* output,void const* source,size_t size){
    const auto address=reinterpret_cast<uintptr_t>(source);
    if(hook_entry_manifest_ready)for(unsigned i=0;i<hook_entry_proof_count;++i){auto const& proof=hook_entry_proofs[i];
        if(address>=proof.target&&address-proof.target<proof.original32.size())
            return gears_hook_entry::read_original(proof,address,output,size,safe_copy);
    }
    return safe_copy(output,source,size);
}
