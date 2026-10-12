#include <windows.h>
#include <d3d12.h>
#include <dxgi.h>
#include <filesystem>
#include <string>
#include <cstdio>

static void owner(FILE *file, const char *name, const void *address)
{
  // MSVC may take the address of its local import jump thunk rather than the
  // imported function. Follow only the known x64 FF 25 [RIP+disp32] encoding.
  auto entry=static_cast<const unsigned char *>(address);
  if(entry[0]==0xff && entry[1]==0x25)
  {
    int displacement{};memcpy(&displacement,entry+2,sizeof(displacement));
    memcpy(&address,entry+6+displacement,sizeof(address));
  }
  HMODULE module{}; wchar_t path[32768]{};
  if(GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                       reinterpret_cast<LPCWSTR>(address),&module)) GetModuleFileNameW(module,path,32768);
  // Text report avoids hand-written JSON path escaping.
  fwprintf(file,L"%hs address=%p owner=%ls\n",name,address,path);
}

// Only inspect our DLL's resolved imports. No game-memory writes, hooks,
// device creation or capture requests are performed here.
extern "C" __declspec(dllexport) DWORD WINAPI GearsRenderDocProbe(void *queryAddress)
{
  HMODULE self{};wchar_t filename[32768]{};
  if(!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                        reinterpret_cast<LPCWSTR>(&GearsRenderDocProbe),&self))return 1;
  GetModuleFileNameW(self,filename,32768);
  auto root=std::filesystem::path(filename).parent_path().parent_path().parent_path().parent_path();
  auto output=root/L"analysis"/L"renderdoc"/(L"bridge-"+std::to_wstring(GetCurrentProcessId())+L".txt");
  FILE *file{};if(_wfopen_s(&file,output.c_str(),L"w, ccs=UTF-8"))return 2;
  // Taking the address reads this DLL's import address table, not the game's.
  owner(file,"D3D12CreateDevice",reinterpret_cast<const void *>(&D3D12CreateDevice));
  owner(file,"CreateDXGIFactory1",reinterpret_cast<const void *>(&CreateDXGIFactory1));
  if(queryAddress)
  {
    HMODULE queryModule{};
    if(!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                         reinterpret_cast<LPCWSTR>(queryAddress),&queryModule) || queryModule!=GetModuleHandleW(L"renderdoc.dll"))
    {fclose(file);return 3;}
    auto query=reinterpret_cast<FARPROC(WINAPI *)(HMODULE,LPCSTR)>(queryAddress);
    owner(file,"queried.D3D12CreateDevice",reinterpret_cast<const void *>(query(GetModuleHandleW(L"d3d12.dll"),"D3D12CreateDevice")));
    owner(file,"queried.CreateDXGIFactory1",reinterpret_cast<const void *>(query(GetModuleHandleW(L"dxgi.dll"),"CreateDXGIFactory1")));
  }
  fclose(file);return 0;
}
