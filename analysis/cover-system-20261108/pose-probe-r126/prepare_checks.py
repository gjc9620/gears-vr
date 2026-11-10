"""Reproduce the focused actual Goal tests without shared in-progress sources."""
from pathlib import Path
import io,subprocess,tarfile,shutil
H=Path(__file__).resolve().parent;R=H.parents[2];out=H/'check-source'
assert not out.exists()
raw=subprocess.check_output(['git','archive','61e93fd1','tools/native-capture','tools/vr-common'],cwd=R)
out.mkdir();tarfile.open(fileobj=io.BytesIO(raw)).extractall(out,filter='data')
for name in ('body_c0_goal_policy_live.inl','body_c0_goal_policy_live_test.cpp'):
    shutil.copyfile(R/'tools/native-capture'/name,out/'tools/native-capture'/name)
# The cosmetic include is unused in this fixture and has an unfinished
# untracked dependency in the shared checkout. It grants no cover behavior.
p=out/'tools/native-capture/host_native_live_prefix.inl';s=p.read_text()
assert s.count('#include "../vr-common/hand_part_grasp.h"')==1
s=s.replace('#include "../vr-common/hand_part_grasp.h"','// Unused cosmetic header excluded from this private cover measurement fixture.')
p.write_text(s)
# Only run modes16..23. The newer completed-cover-consumer tests require
# shared uncommitted APIs and are not part of this measurement diagnostic.
p=out/'tools/native-capture/body_c0_goal_policy_live_test.cpp';s=p.read_text()
a=s.index('    if(mode>=26){');b=s.index('{',a)+1;depth=1
while depth:
    if s[b]=='{':depth+=1
    elif s[b]=='}':depth-=1
    b+=1
s=s[:a]+s[b:]
assert 'goal::pose_begin(entry);C(!std::memcmp(&entry,&entry_before,sizeof entry));' in s
p.write_text(s)
