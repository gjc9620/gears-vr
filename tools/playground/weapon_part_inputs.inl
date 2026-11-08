// File/CPU input only. No native objects, gameplay calls or render resources.
namespace weapon_part_inputs {
namespace catalog=gears_weapon::part_input_catalog;
namespace geometry=gears_weapon::part_geometry;
enum class Result {ready,unknown_asset,missing_root,invalid_file,hash_mismatch,invalid_geometry};
std::atomic<bool> original_source_enabled{};
bool read(const std::wstring& path,std::size_t expected,std::vector<std::uint8_t>& out) {
    if(!expected||expected>2*1024*1024)return false;
    const auto file=CreateFileW(path.c_str(),GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL|FILE_FLAG_SEQUENTIAL_SCAN,nullptr);
    if(file==INVALID_HANDLE_VALUE)return false;
    struct Close {HANDLE file;~Close(){CloseHandle(file);}} close{file};
    LARGE_INTEGER size{};
    if(!GetFileSizeEx(file,&size)||size.QuadPart!=LONGLONG(expected)||GetFileType(file)!=FILE_TYPE_DISK)return false;
    std::vector<std::uint8_t> value(expected);DWORD bytes=0;
    if(!ReadFile(file,value.data(),DWORD(value.size()),&bytes,nullptr)||bytes!=value.size())return false;
    out=std::move(value);return true;
}
bool hash(std::vector<std::uint8_t> const& value,std::array<std::uint8_t,32> const& wanted) noexcept {
    if(value.empty()||value.size()>2*1024*1024)return false;
    BCRYPT_ALG_HANDLE algorithm{};
    if(BCryptOpenAlgorithmProvider(&algorithm,BCRYPT_SHA256_ALGORITHM,nullptr,0)<0)return false;
    std::array<std::uint8_t,32> digest{};
    const auto result=BCryptHash(algorithm,nullptr,0,const_cast<PUCHAR>(value.data()),ULONG(value.size()),
        digest.data(),ULONG(digest.size()));
    BCryptCloseAlgorithmProvider(algorithm,0);
    return result>=0&&digest==wanted;
}
bool folder(std::wstring& out,const wchar_t* relative=L"/assets/weapon-6dof/offline-v2/engine-input") {
    std::array<wchar_t,32768> buffer{};
    const auto size=GetModuleFileNameW(module,buffer.data(),DWORD(buffer.size()));
    if(!size||size>=buffer.size())return false;
    std::wstring path(buffer.data(),size);
    for(unsigned depth=0;depth<8;++depth){
        const auto slash=path.find_last_of(L"\\/");
        if(slash==std::wstring::npos)return false;path.resize(slash);
        const auto candidate=path+relative;
        const auto attributes=GetFileAttributesW(candidate.c_str());
        if(attributes!=INVALID_FILE_ATTRIBUTES&&(attributes&FILE_ATTRIBUTE_DIRECTORY)){
            out=candidate;return true;
        }
    }
    return false;
}
Result load(const char* id,geometry::Conversion const& conversion,geometry::Prepared& out) noexcept {
    if(!id)return Result::unknown_asset;
    const catalog::Asset* selected=nullptr;
    const bool lancer=!std::strcmp(id,"lancer-magazine");
    const bool private_input=lancer||!std::strcmp(id,"snub-magazine");
    if(private_input){
        if(!original_source_enabled.load(std::memory_order_acquire))return Result::unknown_asset;
        // This cache already uses the original engine's mesh axes. Applying
        // the separate authored-GLB axis map would rotate/mirror it twice.
        for(unsigned i=0;i<3;++i)for(unsigned j=0;j<3;++j)
            if(std::abs(conversion.axes[i][j]-(i==j?1.:0.))>1e-9)return Result::invalid_geometry;
        selected=lancer?&gears_weapon::original_part_input::lancer_magazine:&gears_weapon::original_part_input::snub;
    }
    for(const auto& asset:catalog::assets)if(!std::strcmp(asset.id,id)){selected=&asset;break;}
    if(!selected)return Result::unknown_asset;
    try {
        std::wstring root;
        if(!folder(root,private_input?(lancer?gears_weapon::original_part_input::lancer_magazine_cache:gears_weapon::original_part_input::snub_cache):
                   L"/assets/weapon-6dof/offline-v2/engine-input"))return Result::missing_root;
        std::vector<std::uint8_t> vertices,indices;
        if(!read(root+L"/"+selected->vertex_file,selected->vertices*60,vertices)||
           !read(root+L"/"+selected->index_file,selected->indices*2,indices))return Result::invalid_file;
        if(!hash(vertices,selected->vertex_hash)||!hash(indices,selected->index_hash))return Result::hash_mismatch;
        const geometry::Input input{vertices.data(),vertices.size(),indices.data(),indices.size(),
            selected->sections,selected->section_count};
        return geometry::prepare(input,conversion,out)==geometry::Result::ready?Result::ready:Result::invalid_geometry;
    }catch(...){return Result::invalid_file;}
}
} // namespace weapon_part_inputs
