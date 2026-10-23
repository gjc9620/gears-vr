#include "support_diagnostics.h"
#include <cassert>
#include <fstream>
#include <iterator>
int wmain(int argc,wchar_t** argv){
    assert(argc==2);gears_support::Recorder recorder;
    SetLastError(1234);recorder.event(2,0x12345678,640,480,28,2,S_OK,1);assert(GetLastError()==1234);
    for(unsigned i=0;i<10000;++i){recorder.entry(false);recorder.present(0x12345678,0,true,false);}
    recorder.event(3,0x12345678,1280,720,28,2);recorder.event(4,0x12345678,1280,720,28,2,E_FAIL);
    recorder.failure(0x12345678,DXGI_ERROR_DEVICE_REMOVED);recorder.stop(4,999);
    recorder.flush(argv[1]);
    std::ifstream source(argv[1]);std::string text((std::istreambuf_iterator<char>(source)),{});
    assert(text.find("12345678")==std::string::npos);assert(text.find("305419896")==std::string::npos);
    assert(text.find("\"present_entries\":10000")!=std::string::npos);
    assert(text.find("\"generation\":1")!=std::string::npos);
    assert(text.find("\"source_line\":999")!=std::string::npos);
    std::wstring first=std::wstring(argv[1])+L".first.json";
    std::ifstream saved(first);std::string failure((std::istreambuf_iterator<char>(saved)),{});
    assert(failure.find("\"event_code\":4")!=std::string::npos);
    assert(failure.find("\"event_code\":5")==std::string::npos);
    source.close();saved.close();
    for(unsigned i=0;i<300;++i)recorder.event(3,0x12345678,1280,720,28,2);
    recorder.stop(6,12345);recorder.flush(argv[1]);
    std::ifstream recent(argv[1]);std::string latest((std::istreambuf_iterator<char>(recent)),{});
    assert(latest.find("\"source_line\":12345")!=std::string::npos);
    assert(latest.find("\"generation\":301")!=std::string::npos);
    assert(recorder.overflow.load()==45);
    std::ifstream original(first);std::string still_first((std::istreambuf_iterator<char>(original)),{});
    assert(still_first==failure);
    puts("Support diagnostics: anonymous lifecycle, rolling recent events, first failure and counters passed");
}
