"""Wait for the existing admission gate, then call the existing runner once."""
from pathlib import Path
import json
import subprocess
import sys
import time

ROOT=Path(__file__).resolve().parents[3]
HERE=Path(__file__).resolve().parent
sys.path.insert(0,str(ROOT/'tools/multi-instance'))
sys.path.insert(0,str(ROOT/'tools/native-capture'))
import isolated_sim
from run_stereo_continuous import WindowsControl
control=WindowsControl()
record=dict(name='cover-1111-r147-cloak-current',waiting_identity=control.identity(__import__('os').getpid()),
            started_ns=time.time_ns(),observations=[],admitted=False)
path=HERE/'r147-admission.json'
attempt=0
while True:
    attempt+=1
    try:
        memory=isolated_sim.require_memory()
        disk=isolated_sim.require_disk(ROOT)
    except RuntimeError as error:
        reason=str(error)
        if not any(token in reason for token in ('Two automatic instances','Insufficient currently available memory','disk')):
            raise
        record['observations'].append(dict(at_ns=time.time_ns(),reason=reason))
        path.write_text(json.dumps(record,indent=2),encoding='utf-8')
        if attempt%10==1:print(reason,flush=True)
        time.sleep(2)
        continue
    record.update(admitted=True,admitted_ns=time.time_ns(),memory=memory,disk=disk)
    path.write_text(json.dumps(record,indent=2),encoding='utf-8')
    print('Resource gate available; calling the prepared runner once.',flush=True)
    break
with (HERE/'r147-runner.log').open('xb') as log:
    result=subprocess.run([sys.executable,'-E','-s','-B','-X','utf8',
        str(ROOT/'analysis/cover-system-20261108/isolated-r105/run.py'),
        '--name',record['name'],'--probe','path','--run'],cwd=ROOT,
        stdout=log,stderr=subprocess.STDOUT,creationflags=subprocess.CREATE_NO_WINDOW)
record.update(runner_exit=result.returncode,finished_ns=time.time_ns())
path.write_text(json.dumps(record,indent=2),encoding='utf-8')
print(json.dumps(dict(runner_exit=result.returncode)),flush=True)
raise SystemExit(result.returncode)
