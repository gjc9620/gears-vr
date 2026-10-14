#include "origin_trial.h"
#include "submitted_view_lifetime.h"
#include <cstdio>
#include <cstring>
#include <limits>
using namespace gears_origin_trial;
using namespace gears_vr;
#define CHECK(condition) do{if(!(condition)){std::printf("Failed line %d: %s\n",__LINE__,#condition);return __LINE__;}}while(0)
static StereoMatrix identity(){return {1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1};}
static bool same(Selection const& a,Selection const& b){return a.phase==b.phase&&a.phase_qpc==b.phase_qpc&&a.epoch_qpc==b.epoch_qpc&&a.frequency==b.frequency;}
// Independent general 4x4 inverse, not view_with_origin's 3x3 adjugate.
static Vector inverse_origin(StereoMatrix const& view){
    double a[4][8]{};
    for(unsigned i=0;i<4;++i){for(unsigned j=0;j<4;++j)a[i][j]=view[4*i+j];a[i][4+i]=1;}
    for(unsigned col=0;col<4;++col){
        unsigned pivot=col;for(unsigned i=col+1;i<4;++i)if(std::abs(a[i][col])>std::abs(a[pivot][col]))pivot=i;
        for(unsigned j=0;j<8;++j)std::swap(a[col][j],a[pivot][j]);
        double divisor=a[col][col];for(double& x:a[col])x/=divisor;
        for(unsigned i=0;i<4;++i)if(i!=col){double factor=a[i][col];for(unsigned j=0;j<8;++j)a[i][j]-=factor*a[col][j];}
    }
    return {a[3][4],a[3][5],a[3][6]};
}
int main(){
    constexpr std::int64_t hz=1000,epoch=100;
    Trial trial;Selection first{},selected{};
    CHECK(trial.select(10,hz,first)&&first.phase==Phase::a1&&first.epoch_qpc==10);
    gears_submitted::Lifetime life;
    CHECK(!trial.commit(first,life.begin(0))&&trial.epoch_qpc()==0);
    CHECK(trial.select(epoch,hz,selected)&&selected.phase==Phase::a1&&selected.epoch_qpc==epoch);
    CHECK(trial.commit(selected,life.begin(1))&&trial.epoch_qpc()==epoch);
    CHECK(!trial.commit(first,true)&&trial.epoch_qpc()==epoch); // stale failed first candidate
    for(auto delta:{0LL,9999LL,10000LL,19999LL,20000LL,20001LL}){
        CHECK(trial.select(epoch+delta,hz,selected));
        CHECK(selected.phase==(delta<10000?Phase::a1:delta<20000?Phase::b:Phase::a2));
        CHECK(selected.epoch_qpc==epoch&&selected.phase_qpc==epoch+delta);
    }
    // Selection freezes before construction. Both delayed commits cross a
    // phase boundary; no later consume time is accepted by the helper.
    Selection a1{},b{},a2{};
    CHECK(trial.select(epoch+9999,hz,a1));
    CHECK(trial.commit(a1,life.begin(2))); // pretend construction ends at +10001
    CHECK(a1.phase==Phase::a1);
    CHECK(trial.select(epoch+19999,hz,b));
    CHECK(trial.commit(b,life.begin(3))); // pretend consume happens at +20001
    CHECK(b.phase==Phase::b);
    CHECK(trial.select(epoch+20000,hz,a2)&&a2.phase==Phase::a2);
    CHECK(!trial.commit(a2,life.begin(3))&&trial.epoch_qpc()==epoch);
    auto old_view=identity();old_view[12]=123;old_view[13]=-456;old_view[14]=789;
    auto new_view=identity();new_view[12]=-91;new_view[13]=87;new_view[14]=65;
    StereoMatrix output{};
    CHECK(base_view(a1,old_view,{1,2,3},output)&&std::memcmp(output.data(),old_view.data(),sizeof(output))==0);
    CHECK(base_view(a2,new_view,{1,2,3},output)&&std::memcmp(output.data(),new_view.data(),sizeof(output))==0);
    CHECK(output!=old_view);
    double maximum_error{};unsigned accepted{};
    for(double angle:{0.,.7,-1.2}){
        auto raw=old_view;auto r=rotation_matrix({0,std::sin(angle/2),0,std::cos(angle/2)});
        for(unsigned i=0;i<3;++i)for(unsigned j=0;j<3;++j)raw[4*i+j]=static_cast<float>(r[i][j]);
        for(Vector eye:{Vector{},Vector{1,2,3},Vector{35049,-632.849976,9719},Vector{-99999.123,88888.45,77777.001}}){
            auto before=raw;CHECK(base_view(b,raw,eye,output));CHECK(raw==before);
            for(unsigned i=0;i<16;++i)if(i<12||i==15)CHECK(std::memcmp(&output[i],&raw[i],sizeof(float))==0);
            auto error=stereo_distance(inverse_origin(output),eye);CHECK(error<=view_origin_max_error);
            maximum_error=(std::max)(maximum_error,error);++accepted;
            auto alias=raw;CHECK(base_view(b,alias,eye,alias)&&alias==output);
        }
    }
    Selection sentinel=a2;
    for(auto qpc:{0LL,-1LL,epoch-1,epoch+19998}){
        selected=sentinel;CHECK(!trial.select(qpc,hz,selected)&&same(selected,sentinel));
    }
    for(auto frequency:{0LL,-1LL,1001LL}){
        selected=sentinel;CHECK(!trial.select(epoch+20000,frequency,selected)&&same(selected,sentinel));
    }
    Trial huge;auto limit=(std::numeric_limits<std::int64_t>::max)();
    CHECK(huge.select(1,limit,selected)&&huge.commit(selected,true));
    CHECK(huge.select(limit,limit,selected)&&selected.phase==Phase::a1); // 10*frequency would overflow
    Trial span;CHECK(span.select(1,1,selected)&&span.commit(selected,true));
    CHECK(span.select(limit,1,selected)&&selected.phase==Phase::a2); // safely spans nearly INT64_MAX
    for(unsigned failure=0;failure<10;++failure){
        auto raw=identity();auto phase=b;Vector eye{1,2,3};output=new_view;auto before=output;
        if(failure==0)eye.x=std::numeric_limits<double>::quiet_NaN();
        if(failure==1)eye.y=std::numeric_limits<double>::infinity();
        if(failure==2)eye.x=(std::numeric_limits<double>::max)();
        if(failure==3)eye={10000000.25,0,0};
        if(failure==4)raw[5]=std::numeric_limits<float>::quiet_NaN();
        if(failure==5)raw[12]=std::numeric_limits<float>::infinity();
        if(failure==6)raw[0]=2;
        if(failure==7)phase.frequency=0;
        if(failure==8)phase.phase_qpc=phase.epoch_qpc-1;
        if(failure==9)phase.phase=static_cast<Phase>(99);
        CHECK(!base_view(phase,raw,eye,output)&&output==before&&trial.epoch_qpc()==epoch);
    }
    auto invalid=identity();invalid[7]=1;auto before=invalid;
    CHECK(!base_view(b,invalid,{1,2,3},invalid)&&invalid==before);
    // A phases don't require an unused eye origin, but reject nonfinite view.
    CHECK(base_view(a2,new_view,{std::numeric_limits<double>::quiet_NaN(),0,0},output)&&output==new_view);
    auto nonfinite=new_view;nonfinite[12]=std::numeric_limits<float>::quiet_NaN();auto saved=output;
    CHECK(!base_view(a1,nonfinite,{},output)&&output==saved);
    auto invalid_selection=a2;invalid_selection.phase=Phase::a1;
    CHECK(!trial.commit(invalid_selection,true)&&trial.epoch_qpc()==epoch);
    // Invalid first selection or failed construction cannot establish epoch.
    Trial fresh;Selection bad{Phase::a1,1,1,0};CHECK(!fresh.commit(bad,true)&&fresh.epoch_qpc()==0);
    CHECK(fresh.select(100000,hz,selected)&&selected.epoch_qpc==100000&&selected.phase==Phase::a1);
    CHECK(!fresh.commit(selected,false)&&fresh.epoch_qpc()==0);
    CHECK(fresh.select(200000,hz,selected)&&selected.epoch_qpc==200000&&selected.phase==Phase::a1);
    CHECK(fresh.commit(selected,true)&&fresh.epoch_qpc()==200000);
    // A receipt must bind to the exact current family/source/thread and all
    // 128 renderer matrix bytes. Failed binding never changes caller output.
    Trial binding;gears_anchor::Receipt receipt{};
    receipt.invocation=7;receipt.thread=42;receipt.scene_qpc=10;receipt.family_qpc=11;receipt.renderer_qpc=12;
    receipt.key.family=100;receipt.key.owner=100;receipt.key.source=200;receipt.key.local=300;
    receipt.anchor.location={9879.f,-35097.f,-708.85f};receipt.anchor.eye_height=60.f;
    auto bindview=identity(),bindprojection=identity();
    std::memcpy(receipt.renderer_matrices.data(),bindview.data(),64);
    std::memcpy(receipt.renderer_matrices.data()+64,bindprojection.data(),64);
    Metadata metadata{};
    CHECK(prepare(binding,&receipt,100,200,300,42,bindview,bindprojection,100,hz,metadata));
    CHECK(metadata.enabled&&metadata.selection.phase==Phase::a1&&metadata.base==bindview);
    CHECK(binding.commit(metadata.selection,true));
    CHECK(prepare(binding,&receipt,100,200,300,42,bindview,bindprojection,10100,hz,metadata));
    CHECK(metadata.selection.phase==Phase::b);
    auto bound_origin=inverse_origin(metadata.base);
    CHECK(stereo_distance(bound_origin,{9879,-35097,receipt.anchor.location[2]+60.f})<=view_origin_max_error);
    const auto saved_metadata=metadata;
    for(unsigned failure=0;failure<12;++failure){
        auto badreceipt=receipt;auto badview=bindview,badprojection=bindprojection;
        const gears_anchor::Receipt* pointer=&badreceipt;
        if(failure==0)pointer=nullptr;if(failure==1)badreceipt.invocation=0;
        if(failure==2)badreceipt.key.source++;if(failure==3)badreceipt.key.family++;
        if(failure==4)badreceipt.key.owner++;if(failure==5)badreceipt.key.local++;
        if(failure==6)badreceipt.thread++;if(failure==7)badreceipt.scene_qpc=0;
        if(failure==8)badreceipt.family_qpc=9;if(failure==9)badreceipt.renderer_qpc=10101;
        if(failure==10)badview[12]=1;if(failure==11)badprojection[0]=2;
        CHECK(!prepare(binding,pointer,100,200,300,42,badview,badprojection,10100,hz,metadata));
        CHECK(metadata.enabled==saved_metadata.enabled&&same(metadata.selection,saved_metadata.selection)&&metadata.base==saved_metadata.base);
    }
    gears_submitted::FrameTicket admitted{};admitted.enabled=true;admitted.id=55;admitted.origin=metadata;
    gears_submitted::Lifetime copied;CHECK(copied.begin(0x1000,{},admitted));
    admitted.origin.receipt.anchor.location[0]=0;admitted.origin.base[12]=0;
    gears_submitted::FrameTicket consumed{};CHECK(copied.frame(0x1000,consumed));
    CHECK(consumed.origin.receipt.anchor.location[0]==9879.f&&consumed.origin.base==saved_metadata.base);
    std::printf("origin-trial CPU: first-success epoch, admission failures, 10/20s boundaries, frozen phases, fresh A2 views, reverse/overflow/finite/alias checks passed; B accepted=%u max_inverse_error=%.9g bound=%.2f\n",accepted,maximum_error,view_origin_max_error);
    return 0;
}
