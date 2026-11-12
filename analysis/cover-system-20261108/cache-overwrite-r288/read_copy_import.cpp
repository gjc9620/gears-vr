#include <array>
#include <memory>
#include "../../../tools/native-capture/fresh_pose_producer.h"
namespace {namespace fresh_pose_producer_live {using PendingBranch=::fresh_pose_producer_live::PendingBranch;}}
#define main PriorFreshProducerTestMain
#include "../../../tools/native-capture/fresh_pose_producer_test.cpp"
#undef main
int main(int argc,char**argv){
    if(argc!=2)return 2;FILE* in{};if(fopen_s(&in,argv[1],"rb")||!in)return 3;
    f::Header h{};
    if(fread(&h,1,sizeof h,in)!=sizeof h||memcmp(h.magic,"FPOSE001",8)||h.header_size!=sizeof h||
       h.record_size!=sizeof(f::Record)||h.count!=1)return 4;
    auto record=std::make_unique<f::Record>();
    if(fread(record.get(),1,sizeof *record,in)!=sizeof *record||fgetc(in)!=EOF)return 5;fclose(in);
    for(auto const* item:{&record->copy_before,&record->copy_c0}){
        printf("mask=%u slot=%llx target=%llx module=%llx path=%ls prefix=",item->read_mask,
            (unsigned long long)item->slot,(unsigned long long)item->target,(unsigned long long)item->module_base,item->module_path.data());
        for(auto b:item->prefix)printf("%02x",b);puts("");
    }
    return 0;
}
