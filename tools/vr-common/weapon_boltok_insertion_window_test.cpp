#include "weapon_boltok_perfect_window.h"
#include "weapon_boltok_insertion_window.h"
#include "weapon_reload_transaction.h"
#include <cstdio>
#include <cstdlib>

namespace perfect=gears_boltok_perfect_window;
namespace close=gears_boltok_insertion_window;
namespace txn=gears_weapon;
unsigned checks{};
#define C(x) do{++checks;if(!(x)){std::fprintf(stderr,"FAIL %d: %s\n",__LINE__,#x);std::exit(1);}}while(false)

int main(){
    perfect::ClosingWindow fast{};
    C(fast.begin(11,1000,600));
    C(fast.arm_first_alignment(11,1100));
    C(fast.valid_insert(11,1600));
    C(fast.confirm_insertion(11,1600));
    C(!fast.confirm_insertion(11,1600));
    const auto fast_window=fast.window_for_close(11,2400);
    C(fast_window==1400);
    C(txn::physical_reload_result(6,6,1000,2400,fast_window)==txn::ReloadTxnResult::perfect);

    perfect::ClosingWindow immutable{};
    C(immutable.begin(12,1000,600));
    C(immutable.arm_first_alignment(12,1200));
    C(!immutable.arm_first_alignment(12,1800));
    C(immutable.confirm_insertion(12,1800));
    C(immutable.window_for_close(12,2100)==1100);

    perfect::ClosingWindow slow{};
    C(slow.begin(13,1000,600));
    C(slow.arm_first_alignment(13,1300));
    C(!slow.valid_insert(13,1901));
    C(slow.confirm_insertion(13,1901));
    C(slow.window_for_close(13,2500)==0);
    C(txn::physical_reload_result(6,6,1000,2500,slow.window_for_close(13,2500))==txn::ReloadTxnResult::ordinary);

    perfect::ClosingWindow lost{};
    C(lost.begin(14,1000,600));
    C(lost.arm_first_alignment(14,1100));
    C(lost.alignment_lost());
    C(!lost.arm_first_alignment(14,1200)&&lost.seen_alignment());
    C(lost.confirm_insertion(14,1500)&&lost.window_for_close(14,1700)==0);

    perfect::ClosingWindow wrong{};
    C(wrong.begin(15,1000,600));
    C(wrong.arm_first_alignment(15,1000));
    C(!wrong.confirm_insertion(16,1400));
    C(wrong.confirm_insertion(15,1400));
    C(wrong.window_for_close(16,1600)==0);
    C(wrong.window_for_close(15,5000)==4000);
    C(txn::physical_reload_result(6,6,1000,5000,4000)==txn::ReloadTxnResult::perfect);

    C(!wrong.begin(0,1,600));
    C(!wrong.begin(17,-1,600));
    C(!wrong.begin(17,1,0));
    C(wrong.begin(17,10,600));
    C(!wrong.arm_first_alignment(17,9));
    C(wrong.arm_first_alignment(17,10));
    C(!wrong.confirm_insertion(17,9));
    C(wrong.confirm_insertion(17,10));
    C(wrong.window_for_close(17,10)==0);
    wrong.reset();
    C(!wrong.seen_alignment()&&!wrong.armed()&&wrong.serial()==0);

    C(txn::physical_reload_result(6,6,1000,1600,600)==txn::ReloadTxnResult::perfect);
    C(txn::physical_reload_result(6,1,1000,1600,600)==txn::ReloadTxnResult::ordinary);
    C(txn::physical_reload_result(6,0,1000,1600,600)==txn::ReloadTxnResult::inspection);
    C(txn::physical_reload_result(6,7,1000,1600,600)==txn::ReloadTxnResult::none);

    close::PendingClose pending{};
    C(pending.begin(7001,7002,7003));
    C(!pending.begin(7001,7002,7004));
    C(!pending.consumes(7000,7002,7003));
    C(!pending.consumes(7001,7000,7003));
    C(!pending.consumes(7001,7002,7000));
    C(pending.armed);
    C(pending.consumes(7001,7002,7003));
    C(!pending.armed);
    C(!pending.consumes(7001,7002,7003));

    pending.reset();
    C(!pending.begin(0,7002,7003));
    C(!pending.begin(7001,0,7003));
    C(!pending.begin(7001,7002,0));
    C(!pending.armed);

    C(pending.begin(9001,9002,9003));
    C(!pending.begin(9001,9002,9003));
    pending.reset();
    C(!pending.armed);
    C(pending.begin(9004,9005,9006));

    std::printf("Boltok perfect insertion policy: %u checks; first alignment fixed, auto-close latency excluded, receipt-bound close consumed once; no game/reward acceptance.\n",checks);
}
