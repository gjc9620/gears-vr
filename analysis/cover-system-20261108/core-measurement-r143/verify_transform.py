"""Confirm actual measurement gates are text-identical except observer arguments."""
from pathlib import Path
import hashlib,json,subprocess
from instrument import instrument
H=Path(__file__).resolve().parent;R=H.parents[2]
name='tools/native-capture/body_c0_goal_policy_live.inl'
canonical=subprocess.check_output(['git','show','a6fc906e:'+name],cwd=R,text=True,encoding='utf-8')
frozen=(H.parent/'member-chain-r139/native-source'/name).read_text(encoding='utf-8-sig')
def function(text):
    a=text.index('void sample_current_cover_pose() noexcept {');b=text.index('\nvoid pose_begin(',a)
    return text[a:b]
def restore(text):
    return text.replace('auto check=[&policy]','auto check=[]').replace('note_cover_probe(stage,detail,&policy)','note_cover_probe(stage,detail)')
rows=[]
for mode,old,current in (('canonical',canonical,(R/name).read_text(encoding='utf-8-sig')),
                         ('frozen-r139',frozen,(H/'native-source'/name).read_text(encoding='utf-8-sig'))):
    assert instrument(old)==current
    assert restore(function(current))==function(old)
    rows.append(dict(mode=mode,measurement_gates_and_source_choice_unchanged=True,
        before_sha256=hashlib.sha256(old.encode()).hexdigest(),after_sha256=hashlib.sha256(current.encode()).hexdigest()))
(H/'transform-verification.json').write_text(json.dumps(rows,indent=2)+'\n')
print(json.dumps(rows))
