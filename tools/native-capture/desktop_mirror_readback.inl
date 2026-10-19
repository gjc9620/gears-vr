// Private one-shot desktop acceptance artifact. Included after Activity; native
// readback ownership, Map/Unmap, file I/O and Releases stay on the real worker.
namespace desktop_mirror_readback {
#if defined(GEARS_DESKTOP_MIRROR_READBACK) && GEARS_DESKTOP_MIRROR_READBACK
using Readback=gears_vr::DesktopMirror::Readback;
struct Receipt {
    bool arm_attempted{},armed{},taken{},completed{},image_saved{},metadata_saved{};
    unsigned state{};HRESULT mirror_error{S_OK},map_error{E_PENDING};DWORD image_error{},metadata_error{},worker{};
    uint64_t bytes{},offset{},generation{},ticket{},draw_serial{},source_width{},destination_width{};
    unsigned source_height{},destination_height{},format{},pitch{};int64_t received_ns{},present_ns{};
    gears_vr::MirrorRectangle rectangle{};wchar_t image_path[MAX_PATH]{},metadata_path[MAX_PATH]{};
} receipt;
std::mutex receipt_mutex;
std::atomic<DWORD> worker_thread{};
void start_worker(){
    DWORD empty{};if(!worker_thread.compare_exchange_strong(empty,GetCurrentThreadId()))return;
    NativeOwnedShutdownRhiScope::Activity activity;
    Receipt result;result.worker=GetCurrentThreadId();result.arm_attempted=true;result.armed=desktop_mirror.arm_readback();
    const auto status=desktop_mirror.readback_status();result.state=unsigned(status.state);result.mirror_error=status.error;
    std::lock_guard<std::mutex> lock(receipt_mutex);receipt=result;
}
bool write_bytes(HANDLE file,void const* data,DWORD size,DWORD& error){
    DWORD written{};if(!WriteFile(file,data,size,&written,nullptr)){error=GetLastError();return false;}
    if(written!=size){error=ERROR_WRITE_FAULT;return false;}return true;
}
bool finish_file(HANDLE file,bool good,DWORD& error){
    if(good&&!FlushFileBuffers(file)){error=GetLastError();good=false;}
    if(!CloseHandle(file)){if(!error)error=GetLastError();good=false;}return good;
}
bool paths(Receipt& result){
    wchar_t parent[MAX_PATH]{};const auto length=GetModuleFileNameW(own_module,parent,MAX_PATH);
    auto* slash=length&&length<MAX_PATH?wcsrchr(parent,L'\\'):nullptr;
    if(!slash){result.image_error=length>=MAX_PATH?ERROR_BUFFER_OVERFLOW:ERROR_PATH_NOT_FOUND;return false;}
    *slash=0;
    if(_snwprintf_s(result.image_path,MAX_PATH,_TRUNCATE,L"%s\\desktop-mirror-%lu-%llu-%llu.ppm",parent,GetCurrentProcessId(),result.ticket,result.draw_serial)<0||
       _snwprintf_s(result.metadata_path,MAX_PATH,_TRUNCATE,L"%s\\desktop-mirror-%lu-%llu-%llu.txt",parent,GetCurrentProcessId(),result.ticket,result.draw_serial)<0){result.image_error=ERROR_BUFFER_OVERFLOW;return false;}
    return true;
}
void save_image(Readback const& readback,Receipt& result){
    const auto& d=readback.destination;const auto& f=readback.footprint;const auto format=d.Format;
    const bool rgba=format==DXGI_FORMAT_R8G8B8A8_UNORM||format==DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
    const bool bgra=format==DXGI_FORMAT_B8G8R8A8_UNORM||format==DXGI_FORMAT_B8G8R8A8_UNORM_SRGB;
    const bool rgb10=format==DXGI_FORMAT_R10G10B10A2_UNORM;
    const auto buffer=readback.buffer->GetDesc();
    if((!rgba&&!bgra&&!rgb10)||d.Dimension!=D3D12_RESOURCE_DIMENSION_TEXTURE2D||!d.Width||d.Width>16384||!d.Height||d.Height>16384||
       d.DepthOrArraySize!=1||d.MipLevels!=1||d.SampleDesc.Count!=1||f.Footprint.Width!=d.Width||f.Footprint.Height!=d.Height||
       f.Footprint.Depth!=1||f.Footprint.Format!=format||f.Footprint.RowPitch<d.Width*4||
       buffer.Dimension!=D3D12_RESOURCE_DIMENSION_BUFFER||!readback.bytes||readback.bytes>256ull*1024*1024||buffer.Width<readback.bytes||
       f.Offset>readback.bytes){result.image_error=ERROR_INVALID_DATA;return;}
    const uint64_t extent=uint64_t(d.Height-1)*f.Footprint.RowPitch+d.Width*4;
    if(extent>readback.bytes-f.Offset){result.image_error=ERROR_INVALID_DATA;return;}
    void* mapped{};D3D12_RANGE range{static_cast<SIZE_T>(f.Offset),static_cast<SIZE_T>(f.Offset+extent)};
    result.map_error=readback.buffer->Map(0,&range,&mapped);
    if(FAILED(result.map_error)||!mapped){if(SUCCEEDED(result.map_error))result.image_error=ERROR_INVALID_DATA;return;}
    struct Unmap {ID3D12Resource* resource;~Unmap(){D3D12_RANGE written{};resource->Unmap(0,&written);}} unmap{readback.buffer.Get()};
    HANDLE file=CreateFileW(result.image_path,GENERIC_WRITE,0,nullptr,CREATE_NEW,FILE_ATTRIBUTE_NORMAL,nullptr);
    if(file==INVALID_HANDLE_VALUE){result.image_error=GetLastError();return;}
    char header[64]{};const int length=snprintf(header,sizeof(header),"P6\n%llu %u\n255\n",d.Width,d.Height);
    bool good=length>0&&size_t(length)<sizeof(header)&&write_bytes(file,header,DWORD(length),result.image_error);
    if(!good&&!result.image_error)result.image_error=ERROR_INVALID_DATA;
    std::array<unsigned char,16384*3> row{};
    for(unsigned y=0;good&&y<d.Height;++y){
        const auto* source=static_cast<unsigned char const*>(mapped)+f.Offset+uint64_t(y)*f.Footprint.RowPitch;
        for(unsigned x=0;x<d.Width;++x){const auto* pixel=source+size_t(x)*4;auto* rgb=row.data()+size_t(x)*3;
            if(rgb10){uint32_t packed{};memcpy(&packed,pixel,4);
                rgb[0]=static_cast<unsigned char>(((packed&1023u)*255u+511u)/1023u);
                rgb[1]=static_cast<unsigned char>((((packed>>10)&1023u)*255u+511u)/1023u);
                rgb[2]=static_cast<unsigned char>((((packed>>20)&1023u)*255u+511u)/1023u);
            }else {rgb[0]=pixel[bgra?2:0];rgb[1]=pixel[1];rgb[2]=pixel[bgra?0:2];}}
        good=write_bytes(file,row.data(),DWORD(d.Width*3),result.image_error);
    }
    result.image_saved=finish_file(file,good,result.image_error);
}
void save_metadata(Receipt& r){
    HANDLE file=CreateFileW(r.metadata_path,GENERIC_WRITE,0,nullptr,CREATE_NEW,FILE_ATTRIBUTE_NORMAL,nullptr);
    if(file==INVALID_HANDLE_VALUE){r.metadata_error=GetLastError();return;}
    char text[2048]{};const int length=snprintf(text,sizeof(text),
        "kind=desktop_mirror_completed_destination\nworker=%lu\ngeneration=%llu\nticket=%llu\ndraw_serial=%llu\nsource=%llu,%u\ndestination=%llu,%u\nformat=%u\nrectangle=%u,%u,%u,%u\nreceived_ns=%lld\npresent_ns=%lld\nbytes=%llu\nfootprint_offset=%llu\nrow_pitch=%u\nmap_hr=%08lx\nimage_saved=%u\nimage_error=%lu\nconversion=stored_RGBA_or_BGRA_bytes_or_RGB10_round_to_8_no_gamma_transform\nalpha_discarded=1\nimage_scope=full_destination_including_letterbox\nheadset_acceptance=0\n",
        r.worker,r.generation,r.ticket,r.draw_serial,r.source_width,r.source_height,r.destination_width,r.destination_height,r.format,
        r.rectangle.x,r.rectangle.y,r.rectangle.width,r.rectangle.height,r.received_ns,r.present_ns,r.bytes,r.offset,r.pitch,
        static_cast<unsigned long>(r.map_error),unsigned(r.image_saved),r.image_error);
    const bool good=length>0&&size_t(length)<sizeof(text)&&write_bytes(file,text,DWORD(length),r.metadata_error);
    if(!good&&!r.metadata_error)r.metadata_error=ERROR_INVALID_DATA;r.metadata_saved=finish_file(file,good,r.metadata_error);
}
void poll(){
    const auto worker=worker_thread.load();if(!worker||worker!=GetCurrentThreadId())return;
    NativeOwnedShutdownRhiScope::Activity activity;Readback readback;Receipt result;
    {std::lock_guard<std::mutex> lock(receipt_mutex);result=receipt;if(result.completed)return;}
    const auto state=desktop_mirror.readback_status();result.state=unsigned(state.state);result.mirror_error=state.error;
    if(desktop_mirror.take_readback(readback)){
        result.taken=true;result.state=unsigned(gears_vr::DesktopMirror::ReadbackState::taken);result.generation=readback.generation;result.ticket=readback.ticket;result.draw_serial=readback.draw_serial;
        result.source_width=readback.source.Width;result.source_height=readback.source.Height;result.destination_width=readback.destination.Width;
        result.destination_height=readback.destination.Height;result.format=unsigned(readback.destination.Format);result.rectangle=readback.rectangle;
        result.bytes=readback.bytes;result.offset=readback.footprint.Offset;result.pitch=readback.footprint.Footprint.RowPitch;
        result.received_ns=readback.received_ns;result.present_ns=readback.present_ns;
        if(paths(result)){save_image(readback,result);save_metadata(result);}
        readback.buffer.Reset();result.completed=true;
    }
    // Activity remains alive through the readback's final Release. No mirror or
    // frame mutex is held during Map/Unmap, conversion, I/O or that Release.
    std::lock_guard<std::mutex> lock(receipt_mutex);receipt=result;
}
void print(FILE* file){
    if(!file)return;std::lock_guard<std::mutex> lock(receipt_mutex);auto const& r=receipt;
    fprintf(file,"desktop_mirror_readback enabled=1 arm_attempted=%u armed=%u state=%u mirror_error=%08lx taken=%u completed=%u image_saved=%u metadata_saved=%u map_hr=%08lx image_error=%lu metadata_error=%lu worker=%lu generation=%llu ticket=%llu draw_serial=%llu source=%llu,%u destination=%llu,%u format=%u rectangle=%u,%u,%u,%u bytes=%llu offset=%llu pitch=%u image_path=%ls metadata_path=%ls create_new=1 worker_only=1 image_scope=full_destination headset_acceptance=0\n",
        unsigned(r.arm_attempted),unsigned(r.armed),r.state,static_cast<unsigned long>(r.mirror_error),unsigned(r.taken),unsigned(r.completed),unsigned(r.image_saved),unsigned(r.metadata_saved),static_cast<unsigned long>(r.map_error),r.image_error,r.metadata_error,r.worker,
        r.generation,r.ticket,r.draw_serial,r.source_width,r.source_height,r.destination_width,r.destination_height,r.format,r.rectangle.x,r.rectangle.y,r.rectangle.width,r.rectangle.height,r.bytes,r.offset,r.pitch,r.image_path,r.metadata_path);
}
#else
void start_worker(){}void poll(){}void print(FILE*){}
#endif
}
