#define NOMINMAX
#include <windows.h>
#include <iostream>
#include <cstdlib>
#include <cerrno>
#include <limits>
#include "renderdoc_replay.h"
REPLAY_PROGRAM_MARKER();

// Connect through RenderDoc's supported API; never force a busy connection.
// With no PID argument this only inventories targets. With a PID it captures
// exactly one frame from that process, then disconnects without terminating it.
int main(int argc, char **argv)
{
  if(argc > 2) { std::cerr << "usage: TargetControl [expected-pid]\n"; return 2; }
  char *end = nullptr; errno = 0;
  const unsigned long parsed = argc == 2 ? std::strtoul(argv[1], &end, 10) : 0;
  if(argc == 2 && (errno || !parsed || !end || *end || argv[1][0]=='-' ||
                  parsed > std::numeric_limits<uint32_t>::max())) return 2;
  const uint32_t wanted = uint32_t(parsed);
  uint32_t ident = 0;
  for(unsigned scanned = 0; scanned < 32; ++scanned)
  {
    ident = RENDERDOC_EnumerateRemoteTargets("", ident);
    if(!ident) break;
    ITargetControl *target = RENDERDOC_CreateTargetControl("", ident, "GearsVR single-frame diagnostic", false);
    if(!target) continue;
    uint32_t previousCapture = 0; bool havePreviousCapture = false, drained = false;
    for(unsigned i = 0; i < 64 && target->Connected(); ++i)
    {
      auto message = target->ReceiveMessage(nullptr);
      if(message.type == TargetControlMessageType::RegisterAPI)
        std::cout << "api-status name=" << message.apiUse.name.c_str()
                  << " presenting=" << message.apiUse.presenting
                  << " supported=" << message.apiUse.supported
                  << " reason=" << message.apiUse.supportMessage.c_str() << std::endl;
      if(message.type == TargetControlMessageType::NewCapture)
      { previousCapture=message.newCapture.captureId; havePreviousCapture=true; }
      if(message.type == TargetControlMessageType::Noop){drained=true;break;}
    }
    const uint32_t pid = target->GetPID();
    std::cout << "target ident=" << ident << " pid=" << pid << " name="
              << target->GetTarget().c_str() << " api=" << target->GetAPI().c_str() << std::endl;
    if(wanted && pid == wanted && target->GetAPI().empty())
    {
      target->Shutdown();
      std::cerr << "No graphics API registered; no capture requested.\n";
      return 5;
    }
    if(wanted && pid == wanted && drained && target->Connected() && target->GetBusyClient().empty())
    {
      target->TriggerCapture(1);
      const ULONGLONG deadline = GetTickCount64() + 60000;
      while(target->Connected() && GetTickCount64() < deadline)
      {
        auto message = target->ReceiveMessage(nullptr);
        if(message.type == TargetControlMessageType::NewCapture &&
           (!havePreviousCapture || message.newCapture.captureId > previousCapture))
        {
          std::cout << "capture frame=" << message.newCapture.frameNumber
                    << " bytes=" << message.newCapture.byteSize
                    << " path=" << message.newCapture.path.c_str() << std::endl;
          target->Shutdown();
          return 0;
        }
        Sleep(20);
      }
      target->Shutdown();
      std::cerr << "Capture did not complete within 60 seconds; do not blindly retry.\n";
      return 3;
    }
    target->Shutdown();
  }
  return wanted ? 4 : 0;
}
