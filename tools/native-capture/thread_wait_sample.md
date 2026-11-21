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

---

## 中文

# 外部线程采样器

Win64 仅使用 MSVC/ Windows SDK 构建 :

```bat
call E:\vs\VC\Auxiliary\Build\vcvars64.bat
cl /nologo /std:c++17 /EHsc /W4 /WX thread_wait_sample.cpp /link dbghelp.lib
```

使用小数身份和新输出文件名运行 :

```text
thread_wait_sample.exe PID PROCESS_CREATION_FILETIME TID OUTPUT.jsonl
thread_wait_sample.exe --self-test OUTPUT.jsonl
```

生产取样窗口持续到 8 本地 DbgHelp 初始化后秒,最多为 400 样本和标价 20 ms 周期。 身份检查在所有悬浮之前。 进程/ 线程控件仍然打开。 不使用高程、 下载、 符号服务器、 环境符号路径, 或嵌入式代码View PDB路径 。

每个样本都增加了一个暂停计数,副本CAURXT,最多 64 承诺堆栈的 KiB, 然后在解开前恢复, 模块/ 符号解析度、 分配或文件 I/O。 已存在的悬浮计数会恢复并停止采样。 更改的计数、 API 错误, 或者测量暂停 20 ms 停止取样时有不同的终端原因,并且不为零。暂停预算是 **事后检查, 不是硬的最后期限**: WinAPI 呼叫可以屏蔽。 当样本可能拥有暂停计数时, 请不要杀死或外部超时。 RAII 无法从进程终止或无法恢复的复刻器失败中恢复 。

框架完全来自 GetThread 康特/ StackWalk64,从不进行原始地址扫描。第一个 StackWalk64 如果重复了抓取的 PC/ SP 框, 初始化框会跳过一次。 Stack 读取时会使用冻结字节; 拒绝堆栈分配中未抓取的部分。 该分配表以外的未风码/表可以在恢复后实时读取( 最多是) 1024 调用已读/2 原著:MiB,收录于JSONL. 128- 读取诊断在到达游戏呼叫者之前达到了极限。 这个更大的预算只影响恢复后撤消。 框架以活的未变风元数据仍然有效为条件; 截断和错误被报告 。 `good` 计数已确认的已打开的有效快照, 而非完整的堆栈。 部分堆栈仍然可以识别一个等待, 但不能显示为完整 。

没有 PDB 的符号可能只是前导出的最接近的符号,特别是在游戏中,驱动程序或抓取 DLL. Presure module RVA 和 `symbol_displacement`; 永远不能从远程导出名称中识别内部函数。

JSONL包括生的QPC时间戳/频率,捕获的PC/SP/帧指针/EFlags,暂停/重置计数,模块基数/RVA,当地有的符号,以及终端状态. Alried 生的QPC 与使用报告频率捕获的QPC 对应. `--self-test` 检查在暂停前错误的创建身份拒绝,连续的睡眠行进和恢复,实际的KernelBase睡眠调用帧,以及保存已有的中止.

API 参考文献 : [GetThreadContext](https://learn.microsoft.com/en-us/windows/win32/api/processthreadsapi/nf-processthreadsapi-getthreadcontext), [StackWalk64](https://learn.microsoft.com/en-us/windows/win32/api/dbghelp/nf-dbghelp-stackwalk64), [ResumeThread](https://learn.microsoft.com/en-us/windows/win32/api/processthreadsapi/nf-processthreadsapi-resumethread)。这些API合同并不确定游戏的等待原因;仍然需要抽样的运行时间证据。
