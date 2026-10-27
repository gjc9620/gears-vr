// Actual coordinator; controlled host stage operations, not the real engine.
#include <windows.h>
#include <atomic>
#include <cassert>
#include <cstdio>
#include <cstring>
#include <stdexcept>
#include <type_traits>
#ifdef NDEBUG
#error WeaponRestoreStageTest requires active assertions
#endif
namespace {
#include "weapon_restore_stage.inl"
enum class Failure {none,cpp_hands,cpp_finish,seh_stow,late_finish,guard,nested_poison,cleanup_seh,poison_seh};
struct Backend {
    struct Snapshot {unsigned action{},operation{};bool inside{},releasing{};};
    unsigned action{},operation{},hands_calls{},stow_calls{},finish_calls{},cleanup_calls{},poisons{};
    bool permit{true},inside{},releasing{},private_busy{},dirty{true},exempt{};Failure failure{};
    bool admissible()const noexcept{return permit&&!action&&!operation&&!inside&&!releasing;}
    bool snapshot_no_engine(Snapshot& out)const noexcept {out={action,operation,inside,releasing};return true;}
    void begin(){assert(weaponrestore::active());inside=true;++action;}
    weaponrestore::Step hands(){
        assert(action==1);++hands_calls;releasing=true;private_busy=true;
        if(failure==Failure::cpp_hands)throw std::runtime_error("hands callback allocation");
        releasing=false;private_busy=false;dirty=false;return weaponrestore::Step::clean;
    }
    bool finish_hands(){
        assert(action==1);--action;++finish_calls;
        if(failure==Failure::cpp_finish)throw std::runtime_error("explicit finish");
        ++action;return true;
    }
    weaponrestore::Step stow(){
        assert(action==1);++stow_calls;
        if(failure==Failure::seh_stow){releasing=true;private_busy=true;RaiseException(0xe1420042ul,0,0,nullptr);}
        // A nested caller is denied; the parent's marker is never its permit.
        Backend child;weaponrestore::Journal<Snapshot> nested{};
        assert(weaponrestore::run(child,nested).outcome==weaponrestore::Outcome::denied&&!child.hands_calls);
        if(failure==Failure::nested_poison)assert(weaponrestore::run(child,nested).outcome==weaponrestore::Outcome::unresolved);
        assert(weaponrestore::active());
        return exempt?weaponrestore::Step::exempt:weaponrestore::Step::clean;
    }
    bool finish_all(){assert(action==1);--action;++finish_calls;return failure!=Failure::late_finish;}
    weaponrestore::Outcome verdict(weaponrestore::Result const& result)const noexcept {
        if(!result.hands_finished||!result.all_finished||dirty||private_busy)return weaponrestore::Outcome::unresolved;
        return result.stow==weaponrestore::Step::exempt?weaponrestore::Outcome::exempt:weaponrestore::Outcome::complete;
    }
    void poison_no_engine()noexcept {++poisons;if(failure==Failure::poison_seh)RaiseException(0xe1420044ul,0,0,nullptr);}
    bool cleanup_no_engine(Snapshot const& saved,bool normal)noexcept {
        ++cleanup_calls;const bool exact=!normal||(!action&&!operation&&!releasing);
        action=saved.action;operation=saved.operation;inside=saved.inside;releasing=saved.releasing;
        if(failure==Failure::cleanup_seh)RaiseException(0xe1420043ul,0,0,nullptr);
        return exact&&failure!=Failure::guard; // private_busy/dirty are NOT reset.
    }
};
}
int main(int argc,char** argv){
    assert(argc==2);Backend backend;weaponrestore::Journal<Backend::Snapshot> journal{};
    if(!std::strcmp(argv[1],"denied"))backend.permit=false;
    else if(!std::strcmp(argv[1],"cpp_hands"))backend.failure=Failure::cpp_hands;
    else if(!std::strcmp(argv[1],"cpp_finish"))backend.failure=Failure::cpp_finish;
    else if(!std::strcmp(argv[1],"seh_stow"))backend.failure=Failure::seh_stow;
    else if(!std::strcmp(argv[1],"late_finish"))backend.failure=Failure::late_finish;
    else if(!std::strcmp(argv[1],"guard"))backend.failure=Failure::guard;
    else if(!std::strcmp(argv[1],"nested_poison"))backend.failure=Failure::nested_poison;
    else if(!std::strcmp(argv[1],"cleanup_seh"))backend.failure=Failure::cleanup_seh;
    else if(!std::strcmp(argv[1],"poison_seh"))backend.failure=Failure::poison_seh;
    else if(!std::strcmp(argv[1],"exempt"))backend.exempt=true;
    else assert(!std::strcmp(argv[1],"normal")||!std::strcmp(argv[1],"retry"));
    if(backend.failure==Failure::poison_seh)journal.flags=weaponrestore::created;
    const auto result=weaponrestore::run(backend,journal);
    assert(!weaponrestore::active()&&!backend.inside&&!backend.action&&!backend.operation&&!backend.releasing);
    if(!backend.permit){assert(result.outcome==weaponrestore::Outcome::denied&&!backend.hands_calls&&!backend.cleanup_calls);}
    else if(backend.failure==Failure::none){
        assert(result.outcome==(backend.exempt?weaponrestore::Outcome::exempt:weaponrestore::Outcome::complete));
        assert(backend.hands_calls==1&&backend.stow_calls==1&&backend.finish_calls==2&&backend.cleanup_calls==1);
    }else{
        assert(result.outcome==weaponrestore::Outcome::unresolved);
        if(backend.failure!=Failure::late_finish)assert(!weaponrestore::healthy());
        if(backend.failure==Failure::cpp_hands||backend.failure==Failure::seh_stow)assert(backend.private_busy);
    }
    if(!std::strcmp(argv[1],"retry")){
        assert(weaponrestore::run(backend,journal).outcome==weaponrestore::Outcome::unresolved);
        assert(backend.hands_calls==1&&backend.stow_calls==1&&!weaponrestore::healthy());
        assert(journal.result.outcome==weaponrestore::Outcome::unresolved);
    }
    std::printf("WeaponRestoreStageTest %s passed; actual stage coordinator, host operations only\n",argv[1]);
}
