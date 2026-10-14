# Bounded external thread sampler

Win64 only. Build with the existing MSVC/Windows SDK:

```bat
call E:\vs\VC\Auxiliary\Build\vcvars64.bat
cl /nologo /std:c++17 /EHsc /W4 /WX thread_wait_sample.cpp /link dbghelp.lib
```

Run with decimal identities and a new output filename:

```text
thread_wait_sample.exe PID PROCESS_CREATION_FILETIME TID OUTPUT.jsonl
thread_wait_sample.exe --self-test OUTPUT.jsonl
```

The production sampling window lasts up to 8 seconds after local DbgHelp initialization, with at most 400 samples and a nominal 20 ms period. Identity checks precede all suspensions. Process/thread handles remain open. No elevation, downloads, symbol server, environment symbol path, or embedded CodeView PDB path is used.

Each sample adds one suspend count, copies CONTEXT and at most 64 KiB of committed stack, then resumes before unwinding, module/symbol resolution, allocation, or file I/O. A preexisting suspend count is restored and stops sampling. A changed count, API error, or measured pause over 20 ms stops sampling with a distinct terminal reason and nonzero exit. The pause budget is an **after-the-fact check, not a hard deadline**: WinAPI calls can block. Do not kill or externally time out this sampler while it may own a suspend count. RAII cannot recover from process termination or an irrecoverable ResumeThread failure.

Frames come exclusively from GetThreadContext/StackWalk64, never a raw address scan. The first StackWalk64 initialization frame is skipped once if it repeats the captured PC/SP. Stack reads use the frozen bytes; uncaptured portions of the stack allocation are rejected. Unwind code/tables outside that allocation can be read live after resume (at most 1024 callback reads/2 MiB, recorded in JSONL). The original 128-read diagnostic hit its limit before reaching game callers. This larger budget only affects post-resume unwinding. Frames are conditional on live unwind metadata remaining valid; truncation and errors are reported. `good` counts valid snapshots admitted to unwinding, not complete stacks. Partial stacks can still identify a wait, but must not be presented as complete.

Symbols without PDBs may be only the nearest preceding export, especially inside the game, driver or capture DLL. Preserve module RVA and `symbol_displacement`; never identify an interior function from a distant export name alone.

JSONL includes raw QPC timestamps/frequency, captured PC/SP/frame pointer/EFlags, suspend/resume counts, module bases/RVAs, symbols where locally available, and terminal status. Align raw QPC with capture QPC using the reported frequency. `--self-test` checks wrong creation identity rejection before suspension, continuous Sleep-thread progress and restoration, actual KernelBase Sleep caller frames, and preservation of an existing suspension.

API references: [GetThreadContext](https://learn.microsoft.com/en-us/windows/win32/api/processthreadsapi/nf-processthreadsapi-getthreadcontext), [StackWalk64](https://learn.microsoft.com/en-us/windows/win32/api/dbghelp/nf-dbghelp-stackwalk64), [ResumeThread](https://learn.microsoft.com/en-us/windows/win32/api/processthreadsapi/nf-processthreadsapi-resumethread). These API contracts do not establish a game's wait cause; sampled runtime evidence is still required.
