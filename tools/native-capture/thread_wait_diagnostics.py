"""Bounded, query-only evidence for a native startup thread that did not return.

WCT can block its caller, so run it in our own disposable child, never the game
or the main support-observer thread without a deadline. No privilege adjustment,
thread suspension, process-memory read, object names, or stack dumps.
"""
import ctypes
from ctypes import wintypes as w
import json
import os
from pathlib import Path
import subprocess
import sys


class LockInfo(ctypes.Structure):
    _fields_=[('name',w.WCHAR*128),('timeout',ctypes.c_longlong),('alertable',w.BOOL)]


class ThreadInfo(ctypes.Structure):
    _fields_=[('pid',w.DWORD),('tid',w.DWORD),('wait',w.DWORD),('switches',w.DWORD)]


class NodeData(ctypes.Union):
    _fields_=[('lock',LockInfo),('thread',ThreadInfo)]


class WaitNode(ctypes.Structure):
    _fields_=[('kind',ctypes.c_int),('status',ctypes.c_int),('data',NodeData)]


def project_nodes(nodes,count,pid):
    """Never serialize WCT object names or identities of unrelated processes."""
    result=[]
    for node in nodes[:min(count,16)]:
        row=dict(object_type=int(node.kind),object_status=int(node.status))
        if node.kind==8:  # WctThreadType
            same=int(node.data.thread.pid)==pid
            row.update(in_game_process=same,thread_id=int(node.data.thread.tid) if same else 0,
                       context_switches=int(node.data.thread.switches) if same else None)
        result.append(row)
    return result


def query(pid,born,tid,progress=None):
    base=dict(schema=1,pid=pid,creation_time=born,thread_id=tid,available=False,
              cpu_available=False,wait_chain_available=False,cycle_detected=None)
    if os.name!='nt':return dict(base,reason='windows_only')
    if ctypes.sizeof(WaitNode)!=280 or WaitNode.data.offset!=8:
        return dict(base,reason='unsupported_wct_layout')
    k=ctypes.WinDLL('kernel32',use_last_error=True);a=ctypes.WinDLL('advapi32',use_last_error=True)
    k.OpenProcess.argtypes=[w.DWORD,w.BOOL,w.DWORD];k.OpenProcess.restype=w.HANDLE
    k.OpenThread.argtypes=[w.DWORD,w.BOOL,w.DWORD];k.OpenThread.restype=w.HANDLE
    k.CloseHandle.argtypes=[w.HANDLE];k.CloseHandle.restype=w.BOOL
    for name in ('GetProcessTimes','GetThreadTimes'):
        f=getattr(k,name);f.argtypes=[w.HANDLE,*([ctypes.POINTER(w.FILETIME)]*4)];f.restype=w.BOOL
    for name in ('GetExitCodeProcess','GetExitCodeThread'):
        f=getattr(k,name);f.argtypes=[w.HANDLE,ctypes.POINTER(w.DWORD)];f.restype=w.BOOL
    k.GetProcessIdOfThread.argtypes=[w.HANDLE];k.GetProcessIdOfThread.restype=w.DWORD
    k.QueryThreadCycleTime.argtypes=[w.HANDLE,ctypes.POINTER(ctypes.c_ulonglong)];k.QueryThreadCycleTime.restype=w.BOOL
    for name in ('QueryPerformanceCounter','QueryPerformanceFrequency'):
        f=getattr(k,name);f.argtypes=[ctypes.POINTER(ctypes.c_longlong)];f.restype=w.BOOL
    a.OpenThreadWaitChainSession.argtypes=[w.DWORD,ctypes.c_void_p];a.OpenThreadWaitChainSession.restype=w.HANDLE
    a.CloseThreadWaitChainSession.argtypes=[w.HANDLE];a.CloseThreadWaitChainSession.restype=None
    a.GetThreadWaitChain.argtypes=[w.HANDLE,ctypes.c_size_t,w.DWORD,w.DWORD,ctypes.POINTER(w.DWORD),ctypes.POINTER(WaitNode),ctypes.POINTER(w.BOOL)];a.GetThreadWaitChain.restype=w.BOOL
    process=k.OpenProcess(0x1000,False,pid);thread=None;session=None
    def ft(value):return (value.dwHighDateTime<<32)|value.dwLowDateTime
    def alive(function,handle):
        code=w.DWORD();return bool(function(handle,ctypes.byref(code))) and code.value==259
    try:
        if not process:return dict(base,reason='process_open_failed',winerror=ctypes.get_last_error())
        c,e,kernel,user=w.FILETIME(),w.FILETIME(),w.FILETIME(),w.FILETIME()
        if not k.GetProcessTimes(process,ctypes.byref(c),ctypes.byref(e),ctypes.byref(kernel),ctypes.byref(user)) or ft(c)!=born:
            return dict(base,reason='process_identity_mismatch')
        if not alive(k.GetExitCodeProcess,process):return dict(base,reason='process_exited')
        thread=k.OpenThread(0x0800,False,tid)  # THREAD_QUERY_LIMITED_INFORMATION
        if not thread:return dict(base,reason='thread_open_failed',winerror=ctypes.get_last_error())
        if k.GetProcessIdOfThread(thread)!=pid:return dict(base,reason='thread_owner_mismatch')
        if not alive(k.GetExitCodeThread,thread):return dict(base,reason='thread_exited')
        base.update(available=True,identity_verified=True,thread_alive=True)
        if k.GetThreadTimes(thread,ctypes.byref(c),ctypes.byref(e),ctypes.byref(kernel),ctypes.byref(user)):
            base.update(cpu_available=True,thread_creation_time=ft(c),kernel_100ns=ft(kernel),user_100ns=ft(user))
        else:base['cpu_winerror']=ctypes.get_last_error()
        cycles=ctypes.c_ulonglong()
        if k.QueryThreadCycleTime(thread,ctypes.byref(cycles)):base['thread_cycles']=cycles.value
        qpc,frequency=ctypes.c_longlong(),ctypes.c_longlong()
        if k.QueryPerformanceCounter(ctypes.byref(qpc)) and k.QueryPerformanceFrequency(ctypes.byref(frequency)):
            base.update(cpu_qpc=qpc.value,qpc_frequency=frequency.value)
        if progress:progress(dict(base,reason='cpu_snapshot_before_wait_query'))
        session=a.OpenThreadWaitChainSession(0,None)
        if not session:return dict(base,reason='wct_open_failed',winerror=ctypes.get_last_error())
        nodes=(WaitNode*16)();count=w.DWORD(16);cycle=w.BOOL()
        ok=a.GetThreadWaitChain(session,0,0,tid,ctypes.byref(count),nodes,ctypes.byref(cycle))
        error=0 if ok else ctypes.get_last_error()
        # Partial chains remain usable only for the documented size limits.
        if ok or error in (234,565):
            base.update(wait_chain_available=True,cycle_detected=bool(cycle.value),wait_nodes=project_nodes(nodes,count.value,pid),
                        node_count=min(count.value,16),truncated=not bool(ok) or count.value>16,winerror=error,
                        scope='best_effort_os_wait_chain_dynamic_not_atomic_single_node_can_mean_unsupported_wait')
        else:base.update(reason='wct_unavailable',winerror=error)
        if not alive(k.GetExitCodeProcess,process) or not alive(k.GetExitCodeThread,thread):
            base.update(available=False,thread_alive=False,wait_chain_available=False,cycle_detected=None,wait_nodes=[],reason='exited_during_query')
        return base
    finally:
        if session:a.CloseThreadWaitChainSession(session)
        if thread:k.CloseHandle(thread)
        if process:k.CloseHandle(process)


def sample_thread(game,tid,*,runner=subprocess.run,timeout=1.5):
    """Only a fixed trusted helper path and decimal identities enter argv."""
    expected=(game.get('pid'),game.get('creation_time'),tid)
    if any(type(v) is not int or v<=0 for v in expected):return dict(available=False,reason='invalid_identity')
    command=[sys.executable,'-I','-B',str(Path(__file__).resolve()),'--collect',*(str(v) for v in expected)]
    def parse(raw):
        if not raw or len(raw)>16384:return None
        rows=raw.splitlines()
        if len(rows)>2:return None
        selected=None
        for line in rows:
            try:data=json.loads(line)
            except (ValueError,UnicodeError):return None
            if not isinstance(data,dict) or (data.get('pid'),data.get('creation_time'),data.get('thread_id'))!=expected:return None
            selected={k:v for k,v in data.items() if k!='schema'}
        return selected
    try:
        run=runner(command,stdin=subprocess.DEVNULL,stdout=subprocess.PIPE,stderr=subprocess.PIPE,
                   timeout=timeout,creationflags=getattr(subprocess,'CREATE_NO_WINDOW',0),check=False)
    except subprocess.TimeoutExpired as error:
        partial=parse(error.stdout)
        return dict(partial or dict(available=False),reason='thread_query_timeout',wait_chain_available=False,cycle_detected=None)
    except OSError as error:return dict(available=False,reason='thread_query_start_failed',exception_type=type(error).__name__)
    if run.returncode or len(run.stdout)>16384:return dict(available=False,reason='thread_query_failed')
    data=parse(run.stdout)
    return data if data is not None else dict(available=False,reason='thread_query_invalid_output_or_identity')


if __name__=='__main__':
    if len(sys.argv)!=5 or sys.argv[1]!='--collect':raise SystemExit(2)
    try:
        identity=tuple(int(v) for v in sys.argv[2:])
        if any(v<=0 for v in identity) or identity[0]>0xffffffff or identity[2]>0xffffffff:raise ValueError()
        def output(row):print(json.dumps(row,separators=(',',':')),flush=True)
        output(query(*identity,progress=output))
    except Exception as error:print(json.dumps(dict(schema=1,pid=0,creation_time=0,thread_id=0,available=False,reason='thread_query_exception',exception_type=type(error).__name__)))
