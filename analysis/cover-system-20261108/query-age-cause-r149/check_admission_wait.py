"""Check the actual runner's bounded wait and unchanged default dispatch budget."""
from pathlib import Path
import ast,json
H=Path(__file__).resolve().parent
tree=ast.parse((H.parent/'isolated-r105/run.py').read_text(encoding='utf-8'))
function=next(n for n in tree.body if isinstance(n,ast.FunctionDef) and n.name=='admission_settings')
env={};exec(compile(ast.fix_missing_locations(ast.Module(body=[function],type_ignores=[])),'run.py','exec'),env)
settings=env['admission_settings'];assert settings()==('60',90);count=1
for seconds in (0,1,60,600):assert settings(seconds)==(str(seconds),seconds+30);count+=1
for invalid in (-1,601,True,60.,'60',None):
    try:settings(invalid);raise AssertionError('Invalid wait accepted')
    except ValueError:pass
    count+=1
# Verify the actual subprocess dispatch consumes BOTH checked values, rather
# than exercising an unused standalone budget helper.
calls=[n for n in ast.walk(tree) if isinstance(n,ast.Call) and isinstance(n.func,ast.Attribute)
       and n.func.attr=='run' and any(isinstance(x,ast.Constant) and x.value=='--wait-resources-seconds' for x in ast.walk(n))]
assert len(calls)==1
call=calls[0];assert any(isinstance(n,ast.Name) and n.id=='wait_argument' for n in ast.walk(call.args[0]))
assert next(k.value for k in call.keywords if k.arg=='timeout').id=='launch_timeout';count+=1
result=dict(cases=count,failures=0,scope='Actual runner wait validation/dispatch; no reservation or game')
(H/'admission-wait-check.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result))
