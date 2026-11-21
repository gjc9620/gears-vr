#include "audio_guard_policy.h"
#include <cstdio>
#include <cstdlib>
using namespace gears_test_audio;
static unsigned checks{};
#define CHECK(x) do { ++checks; if (!(x)) { std::printf("FAIL %u line %d\n",checks,__LINE__); std::exit(1); } } while(false)
struct State { bool muted{}; unsigned writes{}, reads{}, fail_writes{}; bool fail_reads{}, ignore_mute_write{}; float volume{.25f}; unsigned volume_writes{}, fail_volume{}; };
struct Fake final : Session {
    std::wstring id; std::shared_ptr<State> state;
    Fake(std::wstring name, std::shared_ptr<State> value) : id(std::move(name)), state(std::move(value)) {}
    std::wstring key() const override { return id; }
    bool mute(bool& value) override { ++state->reads; value=state->muted; return !state->fail_reads; }
    bool set_volume(float value) override {
        ++state->volume_writes;
        if (state->fail_volume) { --state->fail_volume; return false; }
        state->volume=value;return true;
    }
    bool set_mute(bool value) override {
        ++state->writes;
        if (state->fail_writes) { --state->fail_writes; return false; }
        if(value&&state->ignore_mute_write)return true;
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
    CHECK(p.counts().verified_muted==1 && p.counts().initially_muted==0);
    CHECK(p.observe(session(L"endpoint1/session1",a),true)); CHECK(a->writes==1);
    CHECK(p.observe(session(L"endpoint2/session1",b),true)); CHECK(b->writes==0);
    CHECK(p.counts().matched==2 && p.counts().muted==1);
    CHECK(p.counts().initially_muted==1 && p.counts().verified_muted==3);
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
    { Policy q; auto initial=std::make_shared<State>();initial->muted=true;
      CHECK(q.observe(session(L"already-muted",initial),true));
      CHECK(q.counts().matched==1&&q.counts().muted==0&&q.counts().initially_muted==1&&q.counts().verified_muted==1);
      CHECK(initial->writes==0);CHECK(q.restore());CHECK(initial->muted&&initial->writes==0); }
    { Policy q; auto ignored=std::make_shared<State>();ignored->ignore_mute_write=true;
      CHECK(!q.observe(session(L"setter-success-but-unmuted",ignored),true));
      CHECK(q.counts().mute_failed==0&&q.counts().verify_failed==1&&q.counts().verified_muted==0);
      CHECK(q.pending()==1);CHECK(q.restore());CHECK(!ignored->muted); }
    { ManualPolicy q; auto late=std::make_shared<State>(); late->muted=true;
      CHECK(q.configured()==0); CHECK(!q.observe(session(L"late",late),false));
      CHECK(late->writes==0 && late->volume_writes==0);
      CHECK(q.observe(session(L"late",late),true)); CHECK(!late->muted && late->volume==1.0f);
      CHECK(q.configured()==1); late->muted=true; late->volume=.4f;
      CHECK(q.observe(session(L"late",late),true)); CHECK(late->muted && late->volume==.4f);
      CHECK(late->writes==1 && late->volume_writes==1);
      auto other=std::make_shared<State>(); CHECK(q.observe(session(L"new-endpoint/new-session",other),true));
      CHECK(q.configured()==2 && other->volume==1.0f); }
    { ManualPolicy q; auto fail=std::make_shared<State>(); fail->fail_volume=1;
      CHECK(!q.observe(session(L"failure",fail),true)); CHECK(q.configured()==0 && fail->writes==0);
      fail->fail_writes=1; CHECK(!q.observe(session(L"failure",fail),true)); CHECK(q.configured()==0);
      CHECK(q.observe(session(L"failure",fail),true)); CHECK(q.configured()==1 && q.failed==2); }
    std::printf("PASS %u audio policy checks\n",checks);
}
