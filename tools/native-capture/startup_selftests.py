"""Same-invocation startup test evidence; no persistent pass cache."""
import hashlib
import json
from pathlib import Path
import shutil
import tempfile
from types import SimpleNamespace


def encoded(value):return json.dumps(value,sort_keys=True,separators=(',',':')).encode('utf-8')
def digest(path):return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def capture_suite(args):
    suite=[]
    def capture(mode=None,isolated=False):suite.append(['CaptureSelfTest.exe',['@dll']+([mode] if mode else []),isolated])
    def test(name):suite.append([name,[],False])
    capture()
    for flag,mode in (('gpu_diagnostics','--gpu'),('first_view_mask_control','--first-view-mask'),
                      ('view_union_mask_fix','--view-union-mask')):
        if getattr(args,flag,False):capture(mode)
    if getattr(args,'frame_link_diagnostics',False):capture('--frame-link',True);test('FrameLinkTest.exe')
    if getattr(args,'color_copy_diagnostics',False):capture('--color-copy');capture('--color-copy-system')
    if getattr(args,'viewport_diagnostics',False):
        capture('--geometry');capture('--quad');test('GpuViewportStatsTest.exe');capture('--viewport')
    if getattr(args,'family_diagnostics',False):
        for mode in ('--family','--clone','--renderer','--lifetime','--prepare'):capture(mode)
    for flag,name in (('shadow_view','SourceViewShadowTest.exe'),('shadow_constructor','SourceViewArgsTest.exe'),
                      ('constructor_eye','SourceEyeArgsTest.exe'),('renderer_pair','TwoViewRendererShadowTest.exe'),
                      ('submit_pair','SplitSourceViewsTest.exe')):
        if getattr(args,flag,False):test(name)
    if getattr(args,'submit_pair_seconds',0) or getattr(args,'continuous_session',False):test('SubmittedViewLifetimeTest.exe')
    if getattr(args,'shadow_state',False):capture('--state')
    return suite


def continuous_suite():
    return capture_suite(SimpleNamespace(**dict.fromkeys(('gpu_diagnostics','view_union_mask_fix','frame_link_diagnostics',
        'color_copy_diagnostics','viewport_diagnostics','family_diagnostics','shadow_view','shadow_constructor',
        'constructor_eye','submit_pair','continuous_session','shadow_state'),True)))


def artifacts(dll,suite):
    dll=Path(dll).resolve()
    return {str(path).casefold():digest(path) for path in sorted({dll,*[dll.with_name(row[0]) for row in suite]})}


def run_suite(dll,suite,run):
    dll=Path(dll).resolve();before=artifacts(dll,suite)
    for name,arguments,isolated in suite:
        if isolated:
            # Never let the frame-link self-test overwrite the live game's
            # priming status file when legacy activation runs its own suite.
            with tempfile.TemporaryDirectory(prefix='gears-frame-link-selftest-') as directory:
                target=Path(directory);copied=Path(shutil.copy2(dll,target/dll.name))
                executable=Path(shutil.copy2(dll.with_name(name),target/name))
                if digest(copied)!=before[str(dll).casefold()] or digest(executable)!=before[str(dll.with_name(name)).casefold()]:
                    raise RuntimeError('Self-test artifact copy differs')
                run([str(executable),*[str(copied) if value=='@dll' else value for value in arguments]],check=True)
        else:run([str(dll.with_name(name)),*[str(dll) if value=='@dll' else value for value in arguments]],check=True)
    if artifacts(dll,suite)!=before:raise RuntimeError('Self-test artifacts changed during suite')
    return before


def owner_key(owner):return {key:owner[key] for key in ('pid','creation_time')}


def make_proof(dll,suite,hashes,owner,nonce):
    return dict(schema=1,kind='owned_startup_selftests',owner=owner_key(owner),nonce=nonce,
        dll=str(Path(dll).resolve()).casefold(),suite=suite,suite_sha256=hashlib.sha256(encoded(suite)).hexdigest(),artifacts=hashes)


def validate_proof(proof,dll,owner,game,nonce):
    suite=continuous_suite()
    if (not isinstance(proof,dict) or proof.get('schema')!=1 or proof.get('kind')!='owned_startup_selftests' or
            proof.get('owner')!=owner_key(owner) or proof.get('game_identity')!=game or proof.get('nonce')!=nonce or
            not isinstance(nonce,str) or len(nonce)!=32 or any(ch not in '0123456789abcdef' for ch in nonce) or
            proof.get('dll')!=str(Path(dll).resolve()).casefold() or proof.get('suite')!=suite or
            proof.get('suite_sha256')!=hashlib.sha256(encoded(suite)).hexdigest()):
        raise RuntimeError('Owned startup self-test proof identity/suite mismatch')
    if proof.get('artifacts')!=artifacts(dll,suite):raise RuntimeError('Owned startup self-test artifact hashes differ')


def consume_proof(path,expected_digest,**expected):
    path=Path(path);raw=path.read_bytes()
    if hashlib.sha256(raw).hexdigest()!=expected_digest:raise RuntimeError('Owned startup self-test proof digest differs')
    proof=json.loads(raw);validate_proof(proof,**expected)
    path.unlink() # A consumed invocation cannot be reused, even by the same owner.
    return proof
