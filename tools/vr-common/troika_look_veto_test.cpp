#define GEARS_VR_GAMEPAD_FIXTURE_ONLY 1
#include "vr_gamepad_input_test.cpp"
unsigned checks{};
#define CHECK(x) do{++checks;if(!(x)){std::printf("failed %d: %s\n",__LINE__,#x);return 1;}}while(0)
int main(){
    pad::MountedLookVeto veto;CHECK(!veto.suppressed()&&!veto.ready());CHECK(!veto.arm_ready());CHECK(veto.suppressed());
    veto.game_poll_zero();CHECK(veto.ready()&&veto.arm_ready());veto.clear();CHECK(!veto.suppressed());
    veto.game_poll_zero();CHECK(!veto.ready()); // A late previous poll cannot rearm ordinary input.
    Fixture s;s.f.display_mode=0;s.f.display_observed_ns=0;s.advance();CHECK(s.observe());
    s.f.hands[0].stick_y=.7f;s.f.hands[1].stick_x=.8f;s.f.hands[1].stick_y=-.6f;s.advance();CHECK(s.observe());
    const auto original=s.sample();CHECK(original.ly!=0&&original.rx!=0&&original.ry!=0);const auto old_packet=s.packet;
    CHECK(!veto.arm_ready());const auto blocked=s.route.sample(s.now,true,s.packet,0,veto.suppressed());
    CHECK(blocked.ly==original.ly&&!blocked.rx&&!blocked.ry);CHECK(s.packet!=old_packet);veto.game_poll_zero();CHECK(veto.ready());
    const auto blocked_packet=s.packet;veto.clear();const auto handoff=s.route.sample(s.now,true,s.packet,0,false);
    CHECK(!handoff.rx&&!handoff.ry&&handoff.ly==original.ly);CHECK(s.packet==blocked_packet);
    s.advance();CHECK(s.observe());CHECK(!s.route.sample(s.now,true,s.packet).rx); // Held cannot emerge on ordinary handoff.
    s.f.hands[1].stick_x=s.f.hands[1].stick_y=0;s.advance();CHECK(s.observe());CHECK(!s.sample().rx);
    s.f.hands[1].stick_x=.8f;s.advance();CHECK(s.observe());CHECK(s.sample().rx!=0);
    s.button(1,controller_a,control_a,true);s.button(1,controller_a,control_a,false);
    CHECK((s.route.sample(s.now,true,s.packet,0,true).buttons&pad::a)!=0);
    CHECK((s.route.sample(s.now,true,s.packet,0,true).buttons&pad::a)==0); // Original pulse still consumed exactly once.
    s.now+=100000001;const auto expired=s.route.sample(s.now,true,s.packet,0,true);CHECK(expired.lx==0&&expired.ly==0&&expired.rx==0&&expired.ry==0);
    std::printf("Troika mounted look veto/ordinary neutral handoff: %u checks passed; synthetic route/poll state only.\n",checks);
}
