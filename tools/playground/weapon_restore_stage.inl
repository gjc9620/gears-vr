// Include at PG namespace scope after Windows/atomic/type_traits headers.
// Only a caller with an independently validated, live Native grant may run a
// stage. This coordinator creates no permission and never forwards original.
namespace weaponrestore {
enum class Step:unsigned {skipped,clean,exempt,unresolved};
enum class Outcome:unsigned {denied,complete,exempt,unresolved};
struct Result {
    Outcome outcome{Outcome::denied};Step hands{Step::skipped},stow{Step::skipped};
    bool hands_finished{},all_finished{};
};
std::atomic<bool> sticky_fault{};
thread_local void* active_stage{}; // Recursion fence, never an ambient permit.
inline bool healthy() noexcept {return !sticky_fault.load(std::memory_order_acquire);}
inline bool active() noexcept {return active_stage!=nullptr;}
inline int exception_filter(unsigned code) noexcept {
    return code==0xe06d7363ul||code==EXCEPTION_STACK_OVERFLOW?EXCEPTION_CONTINUE_SEARCH:EXCEPTION_EXECUTE_HANDLER;
}
enum Flags:unsigned {created=1,consumed=2,entered=4,body_returned=8,cleanup_done=16,cpp_fault=32,seh_fault=64};
template<class Snapshot>struct Journal {
    Snapshot saved{};Result result{};void* previous{};unsigned flags{},seh_code{};
};
template<class Backend>void poison(Backend& backend) noexcept {
    sticky_fault.store(true,std::memory_order_release);
    __try{backend.poison_no_engine();}
    __except(exception_filter(GetExceptionCode())){ /* Sticky fault already retains residency. */ }
}
template<class Backend>void body_cpp(Backend& backend,Journal<typename Backend::Snapshot>& journal) noexcept {
    try{
        // begin records a local action-depth hold BEFORE an old helper can
        // invoke the engine. Its Action destructor then cannot finish/drain.
        backend.begin();
        journal.result.hands=backend.hands();
        journal.result.hands_finished=backend.finish_hands();
        journal.result.stow=backend.stow();
        journal.result.all_finished=backend.finish_all();
        journal.result.outcome=backend.verdict(journal.result);
        journal.flags|=body_returned;
    }catch(...){
        journal.flags|=cpp_fault;journal.result.outcome=Outcome::unresolved;poison(backend);
    }
}
template<class Backend>void guarded(Backend& backend,Journal<typename Backend::Snapshot>& journal){
    journal.previous=active_stage;journal.flags|=entered;active_stage=&journal;
    __try{body_cpp(backend,journal);}
    __finally{
        __try{
            const bool normal=(journal.flags&body_returned)!=0;
            if(!normal){journal.result.outcome=Outcome::unresolved;poison(backend);}
            // Cleanup is POD-only lexical bookkeeping. It must not call finish,
            // restore, reflection, logging, or reset a resource/private lease.
            const bool guards_closed=backend.cleanup_no_engine(journal.saved,normal);
            if(active_stage!=&journal||!guards_closed){journal.result.outcome=Outcome::unresolved;poison(backend);}
            journal.flags|=cleanup_done;
        }__finally{active_stage=journal.previous;}
    }
}
template<class Backend>Result run(Backend& backend,Journal<typename Backend::Snapshot>& journal) noexcept {
    static_assert(std::is_trivially_copyable_v<typename Backend::Snapshot>);
    static_assert(std::is_trivially_destructible_v<Journal<typename Backend::Snapshot>>);
    static_assert(noexcept(backend.admissible())&&noexcept(backend.snapshot_no_engine(journal.saved))&&
        noexcept(backend.poison_no_engine())&&noexcept(backend.cleanup_no_engine(journal.saved,false)));
    if(journal.flags){journal.result.outcome=Outcome::unresolved;poison(backend);return journal.result;}
    journal.flags=created;
    __try{
        if(!healthy()||active()||!backend.admissible())return journal.result;
        if(!backend.snapshot_no_engine(journal.saved))return journal.result;
        // This bit belongs to this invocation. Reentrant PE/bridge/Gate callers
        // cannot inherit it, and retry cannot execute a second restoration stage.
        journal.flags|=consumed;
        guarded(backend,journal);
    }
    __except(exception_filter(GetExceptionCode())){
        journal.seh_code=GetExceptionCode();journal.flags|=seh_fault;
        journal.result.outcome=Outcome::unresolved;poison(backend);
    }
    if(!healthy())journal.result.outcome=Outcome::unresolved;
    return journal.result;
}
} // namespace weaponrestore
