"""Bounded, passive session support journal. Never changes game/XR decisions."""
import ctypes
from ctypes import wintypes as w
import json
import os
from pathlib import Path
import re
import threading
import time

MAX_SOURCE = 4*1024*1024
GRAPHICS = re.compile(r'openxr|xr_api|xrdriver|dxgi|d3d|nv|amd|ati|pimax|pvr|sboys|virtualdesktop|vrclient|steam|reshade|vorpx|gearscapture|gameoverlay|rtss|renderdoc', re.I)


def message(value):
    text = str(value)[:2000]
    text = re.sub(r'[A-Z]:[\\/][^\r\n\"\'<>]*|(?<![\w:])/(?:[^\s\"\'<>]+)', '<PATH>', text, flags=re.I)
    text = re.sub(r'[\w.+-]+@[\w.-]+\.[A-Za-z]{2,}|(?<!\d)\d{17}(?!\d)', '<PRIVATE>', text)
    return re.sub(r'(?i)\b(password|token|secret|account|username)\s*[:=]\s*\S+', r'\1=<PRIVATE>', text)


def safe_name(value):
    name = str(value).replace('\\', '/').rsplit('/', 1)[-1]
    return name if re.fullmatch(r'[A-Za-z0-9_. -]{1,120}\.(dll|exe)', name, re.I) else '<MODULE>'


class WindowsInventory:
    """Read query handles, module snapshots and file-version resources only."""
    def __init__(self):
        if os.name != 'nt':raise OSError('Windows inventory unavailable')
        self.k=ctypes.WinDLL('kernel32', use_last_error=True)
        self.v=ctypes.WinDLL('version', use_last_error=True)
        self.handles={}
        self.k.OpenProcess.argtypes=[w.DWORD,w.BOOL,w.DWORD];self.k.OpenProcess.restype=w.HANDLE
        self.k.CloseHandle.argtypes=[w.HANDLE];self.k.CloseHandle.restype=w.BOOL
        self.k.GetProcessTimes.argtypes=[w.HANDLE,*([ctypes.POINTER(w.FILETIME)]*4)];self.k.GetProcessTimes.restype=w.BOOL
        self.k.GetExitCodeProcess.argtypes=[w.HANDLE,ctypes.POINTER(w.DWORD)];self.k.GetExitCodeProcess.restype=w.BOOL
        self.k.CreateToolhelp32Snapshot.argtypes=[w.DWORD,w.DWORD];self.k.CreateToolhelp32Snapshot.restype=w.HANDLE
        self.v.GetFileVersionInfoSizeW.argtypes=[w.LPCWSTR,ctypes.POINTER(w.DWORD)];self.v.GetFileVersionInfoSizeW.restype=w.DWORD
        self.v.GetFileVersionInfoW.argtypes=[w.LPCWSTR,w.DWORD,w.DWORD,ctypes.c_void_p];self.v.GetFileVersionInfoW.restype=w.BOOL
        self.v.VerQueryValueW.argtypes=[ctypes.c_void_p,w.LPCWSTR,ctypes.POINTER(ctypes.c_void_p),ctypes.POINTER(w.UINT)];self.v.VerQueryValueW.restype=w.BOOL
        class Module(ctypes.Structure):
            _fields_=[('size',w.DWORD),('id',w.DWORD),('pid',w.DWORD),('global_count',w.DWORD),('process_count',w.DWORD),('base',ctypes.c_void_p),('bytes',w.DWORD),('module',w.HMODULE),('name',w.WCHAR*256),('path',w.WCHAR*260)]
        class Process(ctypes.Structure):
            _fields_=[('size',w.DWORD),('usage',w.DWORD),('pid',w.DWORD),('heap',ctypes.c_size_t),('module',w.DWORD),('threads',w.DWORD),('parent',w.DWORD),('priority',w.LONG),('flags',w.DWORD),('name',w.WCHAR*260)]
        self.Module,self.Process=Module,Process
        for name,kind in (('Module32FirstW',Module),('Module32NextW',Module),('Process32FirstW',Process),('Process32NextW',Process)):
            f=getattr(self.k,name);f.argtypes=[w.HANDLE,ctypes.POINTER(kind)];f.restype=w.BOOL
        self.registered_names=set()
        self.version_cache={}

    def version(self,path):
        if path in self.version_cache:return self.version_cache[path]
        result=None;size=self.v.GetFileVersionInfoSizeW(path,None)
        if 0<size<=1024*1024:
            data=ctypes.create_string_buffer(size);pointer=ctypes.c_void_p();length=w.UINT()
            if self.v.GetFileVersionInfoW(path,0,size,data) and self.v.VerQueryValueW(data,'\\',ctypes.byref(pointer),ctypes.byref(length)) and length.value>=52:
                values=ctypes.cast(pointer,ctypes.POINTER(w.DWORD))
                if values[0]==0xFEEF04BD:result='.'.join(str(n) for n in (values[2]>>16,values[2]&65535,values[3]>>16,values[3]&65535))
        self.version_cache[path]=result
        return result

    def process(self,expected):
        pid=expected['pid'];cached=self.handles.get(pid)
        if cached is None:
            handle=self.k.OpenProcess(0x1000,False,pid)
            if not handle:return dict(available=False,reason='open_failed',winerror=ctypes.get_last_error())
            born,a,b,c=w.FILETIME(),w.FILETIME(),w.FILETIME(),w.FILETIME()
            if not self.k.GetProcessTimes(handle,ctypes.byref(born),ctypes.byref(a),ctypes.byref(b),ctypes.byref(c)) or ((born.dwHighDateTime<<32)|born.dwLowDateTime)!=expected['creation_time']:
                self.k.CloseHandle(handle);return dict(available=False,reason='identity_mismatch')
            self.handles[pid]=handle;cached=handle
        code=w.DWORD()
        if not self.k.GetExitCodeProcess(cached,ctypes.byref(code)):return dict(available=False,reason='query_failed',winerror=ctypes.get_last_error())
        return dict(available=True,state='running' if code.value==259 else 'exited',exit_code=None if code.value==259 else code.value)

    def modules(self,expected):
        before=self.process(expected)
        if before.get('state')!='running':return dict(**before,modules=[])
        handle=self.k.CreateToolhelp32Snapshot(0x8|0x10,expected['pid'])
        if handle==ctypes.c_void_p(-1).value:return dict(available=False,reason='snapshot_failed',winerror=ctypes.get_last_error(),modules=[])
        rows=[];count=0;entry=self.Module();entry.size=ctypes.sizeof(entry)
        try:
            valid=self.k.Module32FirstW(handle,ctypes.byref(entry))
            while valid and count<512:
                count+=1;name=safe_name(entry.name)
                if GRAPHICS.search(name) or name.casefold() in self.registered_names:
                    rows.append(dict(module_name=name,file_version=self.version(entry.path)))
                valid=self.k.Module32NextW(handle,ctypes.byref(entry))
            error=ctypes.get_last_error()
        finally:self.k.CloseHandle(handle)
        after=self.process(expected)
        if after.get('state')!='running':return dict(available=False,reason='exited_during_snapshot',modules=[])
        return dict(available=count>0 and (error==18 or count>=512),modules=rows,total_modules_observed=count,truncated=count>=512,
                    winerror=0 if error==18 else error,scope='loaded_graphics_modules_not_proof_of_active_hooks')

    def software(self):
        import winreg
        components=(('steamvr',('steamvr',)),('pimax',('pimax','pitool')),('sboys',('sboys',)),('virtual_desktop',('virtual desktop',)),('openxr_toolkit',('openxr toolkit',)))
        rows=[]
        for hive in (winreg.HKEY_LOCAL_MACHINE,winreg.HKEY_CURRENT_USER):
            for view in (winreg.KEY_WOW64_64KEY,winreg.KEY_WOW64_32KEY):
                try:
                    with winreg.OpenKey(hive,r'SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall',0,winreg.KEY_READ|view) as parent:
                        for i in range(min(winreg.QueryInfoKey(parent)[0],2048)):
                            try:
                                with winreg.OpenKey(parent,winreg.EnumKey(parent,i)) as key:
                                    name=winreg.QueryValueEx(key,'DisplayName')[0].lower()
                                    for component,patterns in components:
                                        if any(p in name for p in patterns):
                                            try:version=str(winreg.QueryValueEx(key,'DisplayVersion')[0])
                                            except OSError:version='unknown'
                                            if not re.fullmatch(r'[0-9][A-Za-z0-9.+() _-]{0,63}',version):version='unknown'
                                            row=dict(component=component,version=version,source='installed_registration')
                                            if row not in rows:rows.append(row)
                            except (OSError,AttributeError):continue
                except OSError:continue
                # Observe registered layer DLL names for later loaded-module matching.
                for mode in ('Implicit','Explicit'):
                    try:
                        with winreg.OpenKey(hive,'SOFTWARE\\Khronos\\OpenXR\\1\\ApiLayers\\'+mode,0,winreg.KEY_READ|view) as key:
                            for i in range(min(winreg.QueryInfoKey(key)[1],64)):
                                path=Path(winreg.EnumValue(key,i)[0])
                                try:
                                    if path.stat().st_size<=1024*1024:
                                        data=json.loads(path.read_text(encoding='utf-8-sig'));name=safe_name(data.get('api_layer',{}).get('library_path',''))
                                        if name!='<MODULE>':self.registered_names.add(name.casefold())
                                except (OSError,ValueError):continue
                    except OSError:continue
        handle=self.k.CreateToolhelp32Snapshot(2,0)
        if handle!=ctypes.c_void_p(-1).value:
            query=self.k.QueryFullProcessImageNameW;query.argtypes=[w.HANDLE,w.DWORD,w.LPWSTR,ctypes.POINTER(w.DWORD)];query.restype=w.BOOL
            entry=self.Process();entry.size=ctypes.sizeof(entry);count=0
            try:
                valid=self.k.Process32FirstW(handle,ctypes.byref(entry))
                while valid and count<4096:
                    count+=1
                    if re.search(r'vrserver|vrcompositor|pimax|pitool|pvr|sboys|virtualdesktop',entry.name,re.I):
                        process=self.k.OpenProcess(0x1000,False,entry.pid)
                        if process:
                            try:
                                path=ctypes.create_unicode_buffer(32768);length=w.DWORD(len(path))
                                if query(process,0,path,ctypes.byref(length)):
                                    rows.append(dict(module_name=safe_name(entry.name),file_version=self.version(path.value),source='running_vr_component'))
                            finally:self.k.CloseHandle(process)
                    valid=self.k.Process32NextW(handle,ctypes.byref(entry))
            finally:self.k.CloseHandle(handle)
        return dict(available=True,software=rows,scope='installed_and_running_versions_at_session_start_not_connection_proof')

    def close(self):
        for handle in self.handles.values():self.k.CloseHandle(handle)
        self.handles.clear()


class SupportJournal:
    def __init__(self,prefix,game,build,*,owner=None,provider_factory=WindowsInventory,clock=time.perf_counter_ns):
        self.prefix,self.game,self.build=Path(prefix),dict(game),Path(build)
        self.path=self.prefix.with_suffix('.support.jsonl');self.first=self.prefix.with_suffix('.first-failure.json')
        self.clock=clock;self.provider_factory=provider_factory;self.targets={'game':dict(game)}
        self.lock=threading.RLock();self.stop_event=threading.Event();self.thread=None;self.error=None
        self.cursors={};self.adapter_ids={};self.native_adapters={};self.next_modules={};self.first_written=self.first.exists()
        self.records=0;self.rotations=0
        self.context_rows={}
        self.last_progress={}
        self.native_snapshot_state=None
        self.owner=owner
        try:self.native_floor=(self.build/'session-status.txt').stat().st_mtime_ns
        except OSError:self.native_floor=0

    def write(self,role,event,**fields):
        row=dict(support_diagnostics=True,schema=1,role=role,event=event,observed_ns=self.clock(),**fields)
        try:
            with self.lock:
                # Repeat immutable observations with their original timestamps
                # after rotation, so a long session retains its device context.
                if event in ('system','adapter','adapter_required','software_inventory','adapter_comparison'):
                    key=(role,event,fields.get('chain_id',0))
                    if key in self.context_rows or len(self.context_rows)<128:self.context_rows[key]=row
                if self.path.exists() and self.path.stat().st_size>2*1024*1024:
                    os.replace(self.path,self.path.with_name(self.path.name+'.1'));self.rotations+=1
                    with self.path.open('w',encoding='utf-8') as stream:
                        for context in self.context_rows.values():stream.write(json.dumps(context,separators=(',',':'))+'\n')
                    self.records+=len(self.context_rows)
                with self.path.open('a',encoding='utf-8') as stream:stream.write(json.dumps(row,separators=(',',':'))+'\n')
                self.records+=1
        except OSError as error:self.error=type(error).__name__
        return row

    def failure(self,role,category,value,**fields):
        with self.lock:
            row=self.write(role,'first_failure',category=category,message=message(value),first_observed_failure=not self.first_written,ordering='observer_receipt_order',**fields)
            if self.first_written:return
            temporary=self.first.with_name(self.first.name+'.tmp')
            try:
                with temporary.open('w',encoding='utf-8') as stream:
                    json.dump(row,stream);stream.flush();os.fsync(stream.fileno())
                try:os.link(temporary,self.first)
                except FileExistsError:pass
                self.first_written=True
            except OSError as error:self.error=type(error).__name__
            finally:
                try:temporary.unlink(missing_ok=True)
                except OSError:pass

    def bind_xr(self,identity):
        with self.lock:self.targets['xr']=dict(identity)

    def consume(self,path,role,expected=None):
        try:
            info=path.stat();size=info.st_size;identity=(info.st_dev,info.st_ino)
            previous,offset=self.cursors.get(str(path),(identity,0))
            if size<offset or previous!=identity:offset=0
            with path.open('rb') as stream:stream.seek(offset);data=stream.read(MAX_SOURCE)
            complete=data.rfind(b'\n')+1
            self.cursors[str(path)]=(identity,offset+complete)
            for line in data[:complete].splitlines():
                if len(line)>65536:continue
                try:row=json.loads(line)
                except (ValueError,UnicodeError):continue
                if not isinstance(row,dict) or row.get('support_diagnostics') is not True:continue
                if expected and (row.get('pid')!=expected['pid'] or row.get('creation_time')!=expected['creation_time']):continue
                event=row.get('event');fields={k:v for k,v in row.items() if k not in ('role','event','schema','support_diagnostics','observed_ns')}
                self.write(role,event,**fields)
                if event=='adapter':
                    adapter=(row.get('adapter_luid_high'),row.get('adapter_luid_low'))
                    self.adapter_ids.setdefault(role,set()).add(adapter)
                    if role=='native':self.native_adapters[row.get('chain_id',0)]=adapter
                    if 'native' in self.adapter_ids and 'xr' in self.adapter_ids:
                        for chain,selected in self.native_adapters.items():
                            self.write('controller','adapter_comparison',chain_id=chain,adapter_match=selected in self.adapter_ids['xr'],scope='per_observed_chain_inspect_binding_and_present_progress')
                if event=='first_failure' or (event=='graphics_event' and row.get('hresult',0)<0):
                    self.failure(role,'runtime_failure',row.get('action',event),runtime_event=row)
        except FileNotFoundError:pass
        except OSError as error:self.write('observer','observer_error',component=role,exception_type=type(error).__name__)

    def tick(self,provider):
        with self.lock:targets=dict(self.targets)
        for role,expected in targets.items():
            process=provider.process(expected)
            self.write(role,'process_state',pid=expected['pid'],**process)
            if process.get('state')=='exited' and process.get('exit_code') not in (None,0) and not self.stop_event.is_set():
                self.failure(role,'process_exit','Process exited',exit_code=process.get('exit_code'))
            if self.clock()>=self.next_modules.get(role,0):
                self.write(role,'loaded_modules',pid=expected['pid'],**provider.modules(expected))
                self.next_modules[role]=self.clock()+15_000_000_000
        self.consume(self.prefix.with_suffix('.stdout.txt'),'xr',targets.get('xr'))
        self.consume(self.build/f"support-native-{self.game['pid']}.jsonl.1",'native',self.game)
        self.consume(self.build/f"support-native-{self.game['pid']}.jsonl",'native',self.game)
        self.consume(self.build/f"support-native-{self.game['pid']}.jsonl.first.json",'native',self.game)
        # Persist the live native snapshot as it advances, before cleanup/crash.
        source=self.build/'session-status.txt'
        try:
            info=source.stat()
            with source.open('rb') as stream:raw=stream.read(256*1024)
            values=dict(re.findall(rb'\b([a-zA-Z][a-zA-Z0-9_]*)=([^\s]+)',raw))
            owner_ok=not self.owner or (values.get(b'owner_pid')==str(self.owner['pid']).encode() and values.get(b'owner_creation_time')==str(self.owner['creation_time']).encode())
            if info.st_mtime_ns>self.native_floor and owner_ok and values.get(b'pid')==str(self.game['pid']).encode() and values.get(b'complete')==b'1':
                self.snapshot_state(True,'identity_verified')
                destination=self.prefix.with_suffix('.native-live-status.txt');temporary=destination.with_name(destination.name+'.tmp')
                temporary.write_bytes(raw);os.replace(temporary,destination)
                progress={}
                for key in (b'telemetry_present_calls',b'telemetry_present_failures',b'published',b'epoch_submit_total',b'support_capture_attempts',b'support_capture_skipped',b'support_publisher_state',b'support_publisher_busy',b'support_frame_unbound',b'support_frame_changed',b'owned_consumed',b'owned_published',b'owned_error',b'owned_pipeline_rejected',b'owned_list_failure_hr',b'owned_execute_failure_count'):
                    try:progress[key.decode()]=int(values[key],16 if key.endswith(b'_hr') else 10)
                    except (KeyError,ValueError):continue
                stamp=self.clock();ages={}
                for key,value in progress.items():
                    previous,changed=self.last_progress.get(key,(None,stamp))
                    if previous!=value:changed=stamp
                    self.last_progress[key]=(value,changed);ages[key]=(stamp-changed)//1_000_000
                self.write('native','pipeline_progress',counters=progress,unchanged_ms=ages)
                if progress.get('owned_error') or progress.get('owned_execute_failure_count') or progress.get('owned_list_failure_hr'):
                    self.failure('native','owned_pipeline_failure','Native owned renderer reported failure',counters=progress)
            else:
                reason='stale_file' if info.st_mtime_ns<=self.native_floor else 'owner_mismatch' if not owner_ok else 'pid_mismatch' if values.get(b'pid')!=str(self.game['pid']).encode() else 'incomplete_file'
                self.snapshot_state(False,reason)
        except FileNotFoundError:self.snapshot_state(False,'not_produced_or_removed')
        except OSError as error:self.snapshot_state(False,'io_error',type(error).__name__,getattr(error,'winerror',None))

    def snapshot_state(self,available,reason,exception_type=None,winerror=None):
        state=(available,reason,exception_type,winerror)
        if state!=self.native_snapshot_state:
            self.write('observer','native_snapshot',available=available,reason=reason,exception_type=exception_type,winerror=winerror)
            self.native_snapshot_state=state

    def _run(self):
        provider=None
        try:
            provider=self.provider_factory()
            try:self.write('environment','software_inventory',**provider.software())
            except Exception as error:self.error=type(error).__name__;self.write('observer','observer_error',category='software_inventory',exception_type=type(error).__name__)
            while not self.stop_event.is_set():
                try:self.tick(provider)
                except Exception as error:self.error=type(error).__name__;self.write('observer','observer_error',exception_type=type(error).__name__)
                self.stop_event.wait(1)
            self.tick(provider)
        except Exception as error:self.error=type(error).__name__;self.write('observer','observer_error',exception_type=type(error).__name__)
        finally:
            if provider:
                try:provider.close()
                except Exception:pass

    def start(self):
        try:
            self.write('controller','context',game_pid=self.game['pid'],game_creation_time=self.game['creation_time'])
            self.thread=threading.Thread(target=self._run,name='GearsVR support observer',daemon=True);self.thread.start()
        except Exception as error:self.error=type(error).__name__
        return self

    def close(self):
        self.stop_event.set()
        if self.thread:self.thread.join(timeout=3)
        return dict(records=self.records,rotations=self.rotations,observer_error=self.error,
                    worker_joined=not self.thread or not self.thread.is_alive(),first_failure_available=self.first_written)
