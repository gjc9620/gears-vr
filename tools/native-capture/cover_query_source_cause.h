#pragma once
#include <cstdint>

// Scoped observation of an existing rejection callback. No time, game read,
// guard decision, lease, retry, publication permission, or input is produced.
namespace gears_cover_query_source_cause {
struct Key {std::uintptr_t entry{};std::uint64_t compose{};unsigned thread{},stage{};};
inline bool same(Key const& a,Key const& b) noexcept {
    return a.entry==b.entry&&a.compose==b.compose&&a.thread==b.thread&&a.stage==b.stage;
}
struct Witness {Key key{};unsigned verdict{},causes{};bool observed{};};
struct Active {Key key{};unsigned verdict{};Witness* output{};Active* previous{};};
inline thread_local Active* active{};
inline bool armed() noexcept {return active!=nullptr;}
class Scope {
    Active value_{};bool installed_{};
public:
    Scope(bool enabled,Key key,unsigned verdict,Witness& output) noexcept {
        if(!enabled||!key.entry||!key.compose||!key.thread)return;
        value_={key,verdict,&output,active};active=&value_;installed_=true;
    }
    ~Scope(){if(installed_)active=value_.previous;}
    Scope(Scope const&)=delete;Scope& operator=(Scope const&)=delete;
};
inline void note(Key key,unsigned verdict,unsigned causes) noexcept {
    auto* a=active;
    if(!a||!same(a->key,key)||verdict!=a->verdict||a->output->observed)return;
    *a->output={key,verdict,causes,true}; // First matching callback only.
}
} // namespace gears_cover_query_source_cause
