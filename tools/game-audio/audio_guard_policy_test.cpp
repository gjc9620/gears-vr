#include "audio_guard_policy.h"
#include <cstdio>
#include <cstdlib>
using namespace gears_test_audio;
static unsigned checks{};
#define CHECK(x) do { ++checks; if (!(x)) { std::printf("FAIL %u line %d\n",checks,__LINE__); std::exit(1); } } while(false)
struct State { bool muted{}; unsigned writes{}, reads{}, fail_writes{}; bool fail_reads{}; };
struct Fake final : Session {
    std::wstring id; std::shared_ptr<State> state;
    Fake(std::wstring name, std::shared_ptr<State> value) : id(std::move(name)), state(std::move(value)) {}
    std::wstring key() const override { return id; }
    bool mute(bool& value) override { ++state->reads; value=state->muted; return !state->fail_reads; }
    bool set_mute(bool value) override {
        ++state->writes;
        if (state->fail_writes) { --state->fail_writes; return false; }
        state->muted=value; return true;
    }
};
static auto session(wchar_t const* key, std::shared_ptr<State> const& state) { return std::make_unique<Fake>(key,state); }
int main() {
    Policy p; auto a=std::make_shared<State>(); auto b=std::make_shared<State>(); b->muted=true;
    CHECK(p.counts().matched==0); CHECK(p.restore()); // A delayed session is not success.
    CHECK(!p.observe(session(L"endpoint1/session1",a),false)); CHECK(a->writes==0);
    CHECK(p.observe(session(L"endpoint1/session1",a),true)); CHECK(a->muted);
    CHECK(p.counts().matched==1 && p.counts().muted==1);
    CHECK(p.observe(session(L"endpoint1/session1",a),true)); CHECK(a->writes==1);
    CHECK(p.observe(session(L"endpoint2/session1",b),true)); CHECK(b->writes==0);
    CHECK(p.counts().matched==2 && p.counts().muted==1);
    // A later user unmute is suppressed during the test; first original state remains.
    a->muted=false; CHECK(p.observe(session(L"endpoint1/session1",a),true)); CHECK(a->muted);
    a->fail_writes=1; CHECK(!p.restore()); CHECK(p.pending()==1); CHECK(p.counts().restored==0);
    CHECK(p.restore()); CHECK(!a->muted && b->muted); CHECK(p.pending()==0);
    CHECK(p.counts().restored==1); CHECK(p.restore()); CHECK(p.counts().restored==1);
    // Normal stop, exception, cancellation, target death and owner death all use
    // the same retained-object restoration, without a new PID/session lookup.
    for (unsigned reason=0;reason<5;++reason) {
        Policy q; auto late=std::make_shared<State>();
        CHECK(q.counts().matched==0); CHECK(q.observe(session(L"late",late),true));
        CHECK(late->muted); CHECK(q.restore()); CHECK(!late->muted); CHECK(q.counts().restored==1);
    }
    { Policy q; auto fail=std::make_shared<State>(); fail->fail_writes=1;
      CHECK(!q.observe(session(L"failed-mute",fail),true)); CHECK(q.pending()==1);
      CHECK(q.counts().mute_failed==1 && q.counts().muted==0); CHECK(q.restore()); CHECK(!fail->muted); }
    { Policy q; auto fail=std::make_shared<State>(); fail->fail_reads=true;
      CHECK(!q.observe(session(L"unreadable",fail),true)); CHECK(fail->writes==0);
      CHECK(q.counts().matched==0 && q.counts().read_failed==1); CHECK(q.restore()); }
    std::printf("PASS %u audio policy checks\n",checks);
}
