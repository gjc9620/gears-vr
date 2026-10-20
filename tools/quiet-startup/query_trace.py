"""Query completed diagnostic records only; no hook installation or focus calls."""
import ctypes as c
from ctypes import wintypes as w
from pathlib import Path
import hashlib,json,struct,sys,argparse
ROOT=Path(__file__).resolve().parents[2]
sys.path[:0]=[str(ROOT/'tools/native-capture'),str(ROOT/'tools/package-inspector'),str(ROOT/'tools/package-inspector/python-libs')]
from run_stereo_continuous import WindowsControl
from start_capture import k,modules,remote_call
import pefile
parser=argparse.ArgumentParser()
parser.add_argument('--observed',type=Path,required=True)
parser.add_argument('--dll',type=Path,required=True)
parser.add_argument('--sha256',required=True)
parser.add_argument('--output',type=Path,required=True)
args=parser.parse_args()
if args.output.exists():raise FileExistsError(args.output)
observed=json.loads(args.observed.read_text())
game=observed['game'];control=WindowsControl()
assert control.identity(game['pid'])==game
handle=k.OpenProcess(0x43a,False,game['pid'])
if not handle:raise c.WinError(c.get_last_error())
argument=None;finished=False
try:
    loaded=modules(handle);base,path=loaded['gearsquietstartup.dll']
    assert path.resolve()==args.dll.resolve()
    digest=hashlib.sha256(path.read_bytes()).hexdigest()
    assert digest==args.sha256
    pe=pefile.PE(str(path));symbol=next(s for s in pe.DIRECTORY_ENTRY_EXPORT.symbols if s.name==b'GearsQuietTrace2')
    assert not symbol.forwarder
    query=base+symbol.address;pe.close()
    class ModuleInfo(c.Structure):
        _fields_=[('base',c.c_void_p),('size',w.DWORD),('entry',c.c_void_p)]
    ps=c.WinDLL('psapi');ps.GetModuleInformation.argtypes=[w.HANDLE,w.HMODULE,c.POINTER(ModuleInfo),w.DWORD]
    regions=[]
    for name,(start,module_path) in loaded.items():
        mi=ModuleInfo()
        if ps.GetModuleInformation(handle,start,c.byref(mi),c.sizeof(mi)):
            regions.append(dict(name=name,base=start,size=mi.size,path=str(module_path)))
    def location(address):
        for item in regions:
            if item['base']<=address<item['base']+item['size']:
                return dict(address=hex(address),module=item['name'],rva=hex(address-item['base']))
        return dict(address=hex(address),module=None)
    size=54376;argument=k.VirtualAllocEx(handle,None,size,0x3000,4)
    if not argument:raise c.WinError(c.get_last_error())
    assert control.identity(game['pid'])==game
    code=remote_call(handle,query,argument);finished=True
    if code:raise RuntimeError('Trace query returned '+str(code))
    k.ReadProcessMemory.argtypes=[w.HANDLE,c.c_void_p,c.c_void_p,c.c_size_t,c.POINTER(c.c_size_t)]
    raw=c.create_string_buffer(size);read=c.c_size_t()
    if not k.ReadProcessMemory(handle,argument,raw,size,c.byref(read)) or read.value!=size:raise c.WinError(c.get_last_error())
    head=struct.unpack_from('<8I',raw)
    assert head[0]==2 and head[1]==size and head[3]==128 and head[5]<=128
    names=['ShowWindow','SetForegroundWindow','SetActiveWindow','SetFocus','SetWindowPos','BringWindowToTop','CreateWindowExW','CreateWindowExA','DefWindowProcW','DefWindowProcA','NtUserSetForegroundWindow','ShowWindowAsync']
    keys=['sequence','call','thread','suppressed','stack_count','reserved','target','related','result_window','argument','effective_argument','extra_argument','enter_tick','leave_tick','foreground_before','foreground_after']
    def parse_records(raw,count,start,names):
        records=[]
        for i in range(count):
            values=struct.unpack_from('<6I34Q',raw,start+i*424);v=dict(zip(keys,values[:16]))
            assert v['stack_count']<=24 and (v['call'] in names if isinstance(names,dict) else v['call']<len(names))
            extra=struct.unpack_from('<4Q4I',raw,start+i*424+296)
            v.update(zip(['enter_qpc','leave_qpc','returned_value','native_value','return_valid','native_valid','native_call','native_last_error'],extra))
            for label,offset in [('gui_before',344),('gui_after',384)]:
                v[label]=dict(zip(['valid','error','flags','reserved','active','focus','capture'],struct.unpack_from('<4I3Q',raw,start+i*424+offset)))
            v['api']=names[v['call']];v['stack']=[location(x) for x in values[16:16+v['stack_count']]];records.append(v)
        return records
    records=parse_records(raw,head[5],104,names)
    report=dict(game=game,policy_sha256=digest,header=dict(zip(['version','size','enabled','capacity','reserved_count','completed','overflow','busy_skipped'],head)),qpc_frequency=struct.unpack_from('<Q',raw,32)[0],startup=dict(zip(['flags','show_window','x','y','width','height','columns','rows','fill','reserved','input','output','error'],struct.unpack_from('<10I3Q',raw,40))),foreground_events=observed.get('game_events',[]),records=records,modules=regions)
    pe=pefile.PE(str(path));cbt_symbol=next((s for s in pe.DIRECTORY_ENTRY_EXPORT.symbols if s.name==b'GearsQuietCbtTrace'),None)
    if cbt_symbol is not None:
        assert not cbt_symbol.forwarder
        assert control.identity(game['pid'])==game
        finished=False
        code=remote_call(handle,base+cbt_symbol.address,argument);finished=True
        if code:raise RuntimeError('CBT query returned '+str(code))
        cbt_size=54328;cbt_raw=c.create_string_buffer(cbt_size)
        if not k.ReadProcessMemory(handle,argument,cbt_raw,cbt_size,c.byref(read)) or read.value!=cbt_size:raise c.WinError(c.get_last_error())
        cbt_head=struct.unpack_from('<12I',cbt_raw)
        assert cbt_head[0]==1 and cbt_head[1]==cbt_size and cbt_head[3]==128 and cbt_head[5]<=128
        report['cbt']=dict(header=dict(zip(['version','size','enabled','capacity','reserved_count','completed','overflow','busy_skipped','thread','install_error','installed','reserved'],cbt_head)),qpc_frequency=struct.unpack_from('<Q',cbt_raw,48)[0],records=parse_records(cbt_raw,cbt_head[5],56,{1:'HCBT_MINMAX',5:'HCBT_ACTIVATE',7:'HCBT_SYSCOMMAND'}))
        for record in report['cbt']['records']:
            if record['call']==1:
                record['minmax']=dict(show_command=record['argument'],iconic_before=bool(record['related']),
                    iconic_after_next_hook=bool(record['result_window']),style_before=record['extra_argument']&0xffffffff,
                    style_after_next_hook=record['extra_argument']>>32)
    pe.close()
    args.output.write_text(json.dumps(report,indent=2))
    print(json.dumps(dict(output=str(args.output),header=report['header'],qpc_frequency=report['qpc_frequency'])))
finally:
    if argument and finished:k.VirtualFreeEx(handle,argument,0,0x8000)
    k.CloseHandle(handle)
