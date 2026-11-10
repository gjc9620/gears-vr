"""Execute the runner's actual nested window guard with synthetic Win32 seams."""
from pathlib import Path
import ast,ctypes,json,hashlib
from types import SimpleNamespace
H=Path(__file__).resolve().parent
tree=ast.parse((H/'run.py').read_text())
main=next(x for x in tree.body if isinstance(x,ast.FunctionDef) and x.name=='main')
guard=next(x for x in main.body if isinstance(x,ast.FunctionDef) and x.name=='check_background')
code=compile(ast.fix_missing_locations(ast.Module(body=[guard],type_ignores=[])),'actual-runner-guard','exec')
g=dict(pid=10,creation_time=20,image='game');x=dict(pid=11,creation_time=21,image='xr')
state=dict(game_identity=g);session=dict(xr_identity=x)
checks=0
def check(mode,expected):
    global checks
    result=dict(foreground_observations=dict(samples=0,owned_activation=False),cloak_observations=dict(samples=0,failures=[]))
    identities={10:g,11:x}
    def identity(pid):return dict(g,creation_time=999) if mode=='recycled' and pid==10 else identities.get(pid)
    def window_pid(hwnd,out):out._obj.value=10 if hwnd==100 else 11 if hwnd==101 else 99
    def enum(callback,_):
        for hwnd in (100,101,102):callback(hwnd,0)
        return mode!='enum_error'
    def get_cloak(hwnd,attribute,out,size):
        assert attribute==14 and size==4
        out._obj.value=0 if mode=='uncloaked' and hwnd==100 else 1
        return -1 if mode=='dwm_error' and hwnd==101 else 0
    user=SimpleNamespace(GetForegroundWindow=lambda:100 if mode=='foreground' else 102,
        GetWindowThreadProcessId=window_pid,EnumWindows=enum,
        IsWindowVisible=lambda hwnd:not(mode=='hidden_style' and hwnd==100))
    def allowed():
        if mode=='paused':raise RuntimeError('pause marker')
    ns=dict(ctypes=ctypes,result=result,require_start_allowed=allowed,user=user,
        control=SimpleNamespace(identity=identity),enum_callback=lambda callback:callback,
        dwm=SimpleNamespace(DwmGetWindowAttribute=get_cloak))
    exec(code,ns)
    rejected=False
    try:ns['check_background'](state,session)
    except RuntimeError:rejected=True
    assert rejected==expected,(mode,rejected,result)
    if mode=='normal':assert result['cloak_observations']['samples']==2 and not result['cloak_observations']['failures']
    if mode in ('recycled','hidden_style'):assert result['cloak_observations']['samples']==1
    if mode=='foreground':assert result['foreground_observations']['owned_activation']
    checks+=1
for mode,expected in [('normal',False),('recycled',False),('hidden_style',False),('uncloaked',True),('dwm_error',True),('enum_error',True),('foreground',True),('paused',True)]:check(mode,expected)
record=dict(checks=checks,passed=True,scope='actual runner guard AST; synthetic Win32 APIs only',game_started=False,
    runner_sha256=hashlib.sha256((H/'run.py').read_bytes()).hexdigest())
(H/'cloak-guard-check.json').write_text(json.dumps(record,indent=2)+'\n')
print(json.dumps(record))
