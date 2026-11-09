#include "first_person_eye_trace.h"
#include <memory>
namespace trace=gears_first_person_eye_trace;
unsigned checks{},failures{};
#define C(x) do{++checks;if(!(x)){++failures;printf("FAIL %d %s\n",__LINE__,#x);}}while(0)
int main(){
    bool enabled=true;
    C(trace::option(nullptr,false,enabled)&&!enabled);
    C(trace::option(L"0",false,enabled)&&!enabled);
    C(trace::option(L"1",true,enabled)&&enabled);
    C(!trace::option(L"1",false,enabled));
    for(auto value:{L"true",L"01",L"10",L" 1",L"1 ",L"2"})C(!trace::option(value,true,enabled));
    auto journal=std::make_unique<trace::Journal>();
    C(!journal->due(false,100)&&journal->due_count==0);
    C(journal->due(true,100));C(!journal->due(true,199));C(!journal->due(true,99));
    C(journal->due(true,200));
    auto row=std::make_unique<trace::Row>();row->scene=23;row->thread=7;
    C(journal->append(*row));row->scene=99;C(journal->rows[0].scene==23);
    for(unsigned i=1;i<trace::capacity;++i){row->scene=i+23;C(journal->append(*row));}
    const auto first=journal->rows[0].scene,last=journal->rows[trace::capacity-1].scene;
    C(!journal->due(true,300)&&journal->overflow==1);
    C(!journal->append(*row)&&journal->overflow==2);
    C(journal->count==trace::capacity&&journal->rows[0].scene==first&&journal->rows[trace::capacity-1].scene==last);
    FILE* file{};C(!tmpfile_s(&file)&&file);
    if(file){
        C(!trace::write(file,*journal,0));C(ftell(file)==0);
        C(trace::write(file,*journal,10000000));
        C(ftell(file)==long(sizeof(trace::Header)+sizeof(trace::Row)*trace::capacity));rewind(file);
        trace::Header header{};C(fread(&header,sizeof header,1,file)==1);
        C(!memcmp(header.magic,"EYEGEO1",8)&&header.version==1&&header.row_bytes==sizeof(trace::Row));
        C(header.count==trace::capacity&&header.overflow==2&&header.due==3&&header.qpc_frequency==10000000);
        C(fread(row.get(),sizeof *row,1,file)==1&&row->scene==23);C(!fclose(file));
    }
    C(!trace::write(nullptr,*journal,1));
    printf("first_person_eye_trace: %u checks, %u failures; CPU journal/format only\n",checks,failures);
    return failures?1:0;
}
