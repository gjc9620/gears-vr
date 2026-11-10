"""Actual generated pre-Reader admission; synthetic files/identities only."""
from pathlib import Path
import argparse,ast,copy,json
from types import SimpleNamespace
from unittest.mock import patch
from prepare import HERE,ROOT,sha,load
parser=argparse.ArgumentParser();parser.add_argument('--name',required=True);name=parser.parse_args().name
proof=load(HERE/name/'prepared.json');selected=Path(proof['instance'])
source=selected/'cover-output/read_private_input.py';tree=ast.parse(source.read_text())
start=next(i for i,n in enumerate(tree.body) if isinstance(n,ast.Assign) and any(isinstance(v,ast.Name) and v.id=='control' for v in n.targets))
finish=next(i for i,n in enumerate(tree.body) if i>=start and isinstance(n,ast.Assign) and any(isinstance(v,ast.Name) and v.id=='reader' for v in n.targets))
program=compile(ast.fix_missing_locations(ast.Module(body=tree.body[start:finish+1],type_ignores=[])),'actual-reader-admission','exec')
manifest=load(selected/'manifest.json');expected=(ROOT/manifest['xr_build']/'OpenXRPoseProbe.exe').resolve()
game=dict(pid=1,creation_time=100,image='game');owner=dict(pid=2,creation_time=200,image='owner');xr=dict(pid=3,creation_time=300,image=str(expected))
state=dict(instance_root=str(selected),session_report=str(selected/'fake-session.json'),session_id='same-session',phase='session_owned',
    owner_identity=owner,game_identity=game,vr_gamepad={'source':1})
session=dict(phase='running',session_id='same-session',owner_identity=owner,game_identity=game,xr_identity=xr)
checks=0
def evaluate(change,accepted):
    global checks
    current=copy.deepcopy(state);report=copy.deepcopy(session);alive={x['pid']:copy.deepcopy(x) for x in (game,owner,xr)}
    change(current,report,alive);constructed=[]
    files={selected/'launcher-session.json':current,selected/'fake-session.json':report,selected/'manifest.json':manifest}
    def read(path,*args,**kwargs):return json.dumps(files[path])
    def reader(*args):constructed.append(args);return object()
    ns=dict(Path=Path,json=json,ROOT=ROOT,WindowsControl=lambda:SimpleNamespace(identity=lambda pid:alive.get(pid)),w=SimpleNamespace(Reader=reader))
    rejected=False
    with patch.object(Path,'read_text',autospec=True,side_effect=read):
        try:exec(program,ns)
        except (AssertionError,KeyError):rejected=True
    assert (not rejected)==accepted and len(constructed)==int(accepted),(change,rejected,constructed)
    checks+=1
evaluate(lambda a,b,c:None,True)
evaluate(lambda a,b,c:a.update(phase='waiting_for_level'),True)
evaluate(lambda a,b,c:a.update(phase='navigating_menu'),False)
evaluate(lambda a,b,c:a.update(phase='running'),False)
evaluate(lambda a,b,c:a.update(phase='finished'),False)
evaluate(lambda a,b,c:b.update(phase='awaiting_level'),False)
evaluate(lambda a,b,c:b.update(phase='stopped'),False)
evaluate(lambda a,b,c:a.update(instance_root=str(selected.parent)),False)
evaluate(lambda a,b,c:a['vr_gamepad'].update(source=0),False)
evaluate(lambda a,b,c:b.update(session_id='other'),False)
evaluate(lambda a,b,c:b['owner_identity'].update(creation_time=999),False)
evaluate(lambda a,b,c:b['game_identity'].update(creation_time=999),False)
evaluate(lambda a,b,c:b['xr_identity'].update(image='wrong-executable'),False)
for pid in (1,2,3):evaluate(lambda a,b,c,pid=pid:c[pid].update(creation_time=999),False)
record=dict(checks=checks,passed=True,scope='actual generated reader admission; fake files, identities and Reader constructor',
    source_sha256=sha(source),game_started=False,input_published=False)
(HERE/name/'reader-check.json').write_text(json.dumps(record,indent=2)+'\n');print(json.dumps(record))
