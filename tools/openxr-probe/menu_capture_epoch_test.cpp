#include "menu_capture_epoch.h"
#include <cstdio>
int main(){
    MenuCaptureEpoch policy;
    if(policy.fresh(1)||policy.accept(1,1))return 1;
    policy.begin(100);
    if(policy.accept(99,200)||policy.accept(100,200)||policy.accept(201,200)||policy.fresh(200))return 2;
    if(!policy.accept(101,200)||!policy.fresh(10000101)||policy.fresh(10000102)||policy.fresh(100))return 3;
    // A callback arriving in this epoch can still contain an older capture.
    policy.begin(200);
    if(policy.fresh(201)||policy.accept(199,201)||policy.accept(200,201))return 4;
    if(!policy.accept(201,202)||!policy.fresh(202))return 5;
    if(policy.accept(202,10000203)||!policy.fresh(203))return 6;
    policy.begin(10000204);
    if(policy.fresh(10000205)||policy.accept(201,10000205))return 7;
    if(!policy.accept(10000205,10000206)||!policy.fresh(10000206))return 8;
    policy.begin(-1);if(policy.accept(1,2)||policy.fresh(2))return 9;
    std::puts("menu capture epoch: timestamp/age/reset/late-buffer checks passed");return 0;
}
