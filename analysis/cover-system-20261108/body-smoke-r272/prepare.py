"""Prepare one private no-physics cover C0 smoke; never launches a game."""
from pathlib import Path
import ast,contextlib,hashlib,importlib.util,io,json,sys
ROOT=Path(__file__).resolve().parents[3];FOLDER=Path(__file__).resolve().parent
HERE=ROOT/'analysis/cover-system-20261108/isolated-r105';NAME='cover-1112-r272-cloak-body'
sha=lambda p:hashlib.sha256(Path(p).read_bytes()).hexdigest()
def load(p):return json.loads(Path(p).read_text(encoding='utf-8-sig'))
def replace(s,old,new):
    if s.count(old)!=1:raise RuntimeError('Unique source anchor required: '+old[:80])
    return s.replace(old,new)
def write(p,s):
    with Path(p).open('x',encoding='utf-8') as stream:stream.write(s)
def module(name,path):
    spec=importlib.util.spec_from_file_location(name,path);m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m);return m
if (ROOT/'analysis/instances'/NAME).exists():raise FileExistsError('Fresh attempt only')
source=ROOT/'analysis/cover-system-20261108/cover-hand-routing-r271'
receipt=load(source/'native-build-v2-result.json');manifest=load(source/'native-source-v2-manifest.json')
assert manifest['flags']['GEARS_COVER_PHYSX_QUERIES']=='OFF' and not receipt['external_worktree_dependencies']
for rel,digest in receipt['project_dependencies'].items():assert sha(source/'native-source-v2'/rel)==digest==manifest['files'][rel],rel
native=source/'native-build-v2/GearsCapture.dll';assert sha(native)==receipt['dll_sha256']=='da2b6fe06e71a4633a615a17342cada201c412fa941b5aa688050933dfc62320'
prep=module('cover_body_prepare_base',HERE/'prepare.py');original_load=prep.load
# Preserve the original preparation/isolation logic, replacing ONLY the selected
# immutable Native artifact in its two saved build descriptors.
def selected_load(path):
    value=original_load(path);path=Path(path).resolve()
    if path==(prep.FROZEN/'prepared.json').resolve():value['builds']['native']={'saved':str(native),'sha256':sha(native)}
    if path==(prep.R104/'plan.json').resolve():value['build_sha256']['native']=sha(native)
    return value
prep.load=selected_load;sys.argv=[str(HERE/'prepare.py'),'--name',NAME]
log=io.StringIO()
with contextlib.redirect_stdout(log):prep.main()
write(FOLDER/'prepare-base.log',log.getvalue())
selected=ROOT/'analysis/instances'/NAME;inputs=HERE/NAME;output=selected/'cover-output'
# Private entry preserves the original Source1 launch/identity/input policy.
entry_text=(prep.OLD/'sim_entry.py').read_text(encoding='utf-8')
entry_text=replace(entry_text,'ROOT=Path(__file__).resolve().parents[2]', 'ROOT=Path('+repr(str(ROOT))+')')
entry_text=replace(entry_text,'OUT=Path(__file__).resolve().parent','OUT=Path('+repr(str(prep.OLD))+')')
sections='\n[CoverBodyPrototype]\nenabled=1\npose_profile=top\ntarget_source=live\n[CoverCameraPreview]\nenabled=1\n[CoverInputPrototype]\nenabled=1\nheight_half_metres=0.3\nheight_top_metres=0.7\n'
entry_text=replace(entry_text,'    import sixdof_manual_profile as profile','    import sixdof_manual_profile as profile\n    profile.SECTIONS += '+repr(sections))
entry=output/'cover_body_entry.py';write(entry,entry_text);ast.parse(entry_text)
# Private copy changes only entry path plumbing. Pause/resource/ownership/quiet
# window gates remain the original implementations and canonical locations.
tool_text=(ROOT/'tools/multi-instance/isolated_sim.py').read_text(encoding='utf-8')
tool_text=replace(tool_text,'ROOT=Path(__file__).resolve().parents[2]','ROOT=Path('+repr(str(ROOT))+')')
tool_text=replace(tool_text,'sys.path.insert(0,str(Path(__file__).resolve().parent))','sys.path.insert(0,'+repr(str(ROOT/'tools/multi-instance'))+')')
tool_text=replace(tool_text,"Path(__file__).with_name('start.py')",'Path('+repr(str(ROOT/'tools/multi-instance/start.py'))+')')
tool_text=replace(tool_text,"str(BASE/'sim_entry.py')",'str(Path('+repr(str(entry))+'))')
tool_text=replace(tool_text,"Path(__file__).with_name('automatic-runtime-pause.json')",'Path('+repr(str(ROOT/'tools/multi-instance/automatic-runtime-pause.json'))+')')
tool_text=replace(tool_text,"Path(__file__).with_name('background-runtime.json')",'Path('+repr(str(ROOT/'tools/multi-instance/background-runtime.json'))+')')
tool=output/'cover_isolated_tool.py';write(tool,tool_text);ast.parse(tool_text)
private_tool=module('cover_body_tool_preflight',tool)
checked_env,checked_slot=private_tool.environment(selected)
assert Path(checked_env['GEARSVR_INSTANCE_ROOT']).resolve()==selected.resolve()
assert checked_env['GEARSVR_TEST_WINDOWS_BEHIND']=='1' and checked_env['XR_SIMULATOR_NO_ACTIVATE']=='1'
assert checked_env['GEARSVR_TEST_MUTE_GAME']=='1'
assert sha(checked_slot['manifest'])==checked_slot['manifest_sha256'] and sha(checked_slot['dll'])==checked_slot['dll_sha256']

prepared=load(inputs/'prepared.json');prepared.update(isolation_source_sha256=sha(tool),canonical_isolation_source_sha256=sha(ROOT/'tools/multi-instance/isolated_sim.py'),private_entry_sha256=sha(entry),private_entry=str(entry),private_isolation_tool=str(tool),body_native_receipt_sha256=sha(source/'native-build-v2-result.json'),preparation_wrapper_sha256=sha(__file__),shape_snapshot_supported=False,path_probe_supported=False)
(inputs/'prepared.json').write_text(json.dumps(prepared,indent=2)+'\n',encoding='utf-8')
# The existing wrapper already returns after five independent-HMD eye phases;
# add bounded current Native facts, and never enter any collision branch.
template=(prep.R104/'run_cover_eye_approach_operator.py').read_text(encoding='utf-8')
start="        report['scope'] = 'One original CT2 entry, neutral, fixed-position turn/return and tracked rise/return; one opt-in Native raw Sphere observation; no grasp/body movement'"
template=replace(template,start,"        report['scope'] = 'Original CT2 entry; production C0 cover/body/view and L2 points; five bounded HMD phases; no collision query or grip action in this first slice'")
anchor="            phase['end_qpc'] = qpc()"
addition="""
            import launcher as cover_launcher
            deadline=time.monotonic()+8
            while True:
                session_now=observer.verify()
                fields=cover_launcher.current_native_status(observer.backend,session_now)
                phase['native']={k:v for k,v in fields.items() if k.startswith(('cover_input_','cover_l2_','cover_camera_','cover_body_','body_goal_','body_cover_'))}
                save()
                consumed=int(fields.get('body_goal_cover_consumed','0'))
                previous=int(report.get('last_cover_consumed',0))
                point_calls=fields.get('cover_l2_reference_calls','0,0,0,0').split(',')
                if consumed>previous and len(point_calls)==4 and int(point_calls[1])>0:break
                if time.monotonic()>=deadline:raise RuntimeError('No new current cover C0 consumption/L2 points: '+str(phase['native']))
                time.sleep(.1);assert g.live()
            assert fields.get('cover_input_enabled')=='1' and fields.get('cover_camera_enabled')=='1'
            assert fields.get('cover_input_c0_owner')=='1' and fields.get('body_goal_cover_enabled')=='1'
            assert fields.get('cover_body_live_published')=='0', 'Legacy pose writer must remain inactive'
            report['last_cover_consumed']=consumed;report['body_modified']=True
            phase['body_consumption_observed']=True
"""
template=replace(template,anchor,anchor+addition)
template=template.replace("One strict current physics", "One current cover body")
wrapper=FOLDER/'operator-template.py';write(wrapper,template);ast.parse(template)
op_text=(HERE/'prepare_operator.py').read_text(encoding='utf-8')
op_text=replace(op_text,"(R104 / 'run_cover_eye_approach_operator.py').read_text(encoding='utf-8')", "Path("+repr(str(wrapper))+").read_text(encoding='utf-8')")
operator_prep=FOLDER/'prepare_operator_body.py';write(operator_prep,op_text);ast.parse(op_text)
sys.path.insert(0,str(HERE));op=module('cover_body_operator_prepare',operator_prep);sys.argv=[str(operator_prep),'--name',NAME]
with contextlib.redirect_stdout(log):op.main()
write(FOLDER/'prepare-operator.log',log.getvalue())
# Runner changes only add none mode, use private entry tool, retain recorder,
# exact identities, one-shot launch, normal stop and every original guard.
runner=(HERE/'run.py').read_text(encoding='utf-8')
runner=replace(runner,'from prepare import ROOT, HERE, OLD, sha, load, write_new','sys.path.insert(0,'+repr(str(HERE))+')\nfrom prepare import ROOT, HERE, OLD, sha, load, write_new')
runner=replace(runner,"TOOLS = ROOT / 'tools/multi-instance/isolated_sim.py'",'TOOLS = Path('+repr(str(tool))+')')
runner=replace(runner,"choices=('sphere','path','shape'), default='sphere'","choices=('none',), default='none'")
run_path=FOLDER/'run.py';write(run_path,runner);ast.parse(runner)
info={'name':NAME,'native_sha256':sha(native),'entry_sha256':sha(entry),'tool_sha256':sha(tool),'runner_sha256':sha(run_path),'operator_template_sha256':sha(wrapper),'prepared_only':True,'game_started':False,'physics_enabled':False,'legacy_height_curve':'0.3/0.7 compatibility values, unused by selected C0 body owner; not measurements'}
write(FOLDER/'prepared.json',json.dumps(info,indent=2)+'\n');print(json.dumps(info))
