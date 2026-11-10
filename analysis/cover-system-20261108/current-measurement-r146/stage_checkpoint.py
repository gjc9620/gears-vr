"""Stage exact owned text/image evidence; never stage build trees or binaries."""
from pathlib import Path
import subprocess
H=Path(__file__).resolve().parent;R=H.parents[2];prior=H.parent/'core-measurement-r143'
assert not subprocess.check_output(['git','diff','--cached','--name-only'],cwd=R,text=True).strip()
paths=[]
for p in H.iterdir():
    if p.is_file() and p.suffix in ('.py','.cpp','.patch','.json'):paths.append(p)
for folder in H.glob('checks-v*'):
    paths.extend(p for p in folder.iterdir() if p.is_file() and p.suffix in ('.log','.json'))
for name in ('wait_run_r144.py','wait_run_r145.py','review_r145.py','save_r145.py',
             'r144-admission.json','r144-runner.log','r145-admission.json','r145-runner.log'):
    paths.append(prior/name)
for folder in (prior/'runtime-r144-prebirth',prior/'runtime-r145'):
    paths.extend(p for p in folder.iterdir() if p.is_file() and p.suffix in ('.json','.jpg'))
for name in ('cover-1111-r144-cloak-core','cover-1111-r145-cloak-core'):
    for file in ('prepared.json','operator-prepared.json'):paths.append(H.parent/'isolated-r105'/name/file)
paths.extend((H.parent/'isolated-r105/prepare.py',H.parent/'requirements-owner-sync.json',R/'docs/cover-system-development-20261108.md'))
relative=sorted({p.relative_to(R).as_posix() for p in paths});assert all((R/p).is_file() for p in relative)
subprocess.run(['git','add','-f','--']+relative,cwd=R,check=True)
staged=subprocess.check_output(['git','diff','--cached','--name-only'],cwd=R,text=True).splitlines()
assert set(staged)<=set(relative) and staged
print('\n'.join(staged))
