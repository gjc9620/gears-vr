"""Observe first rejected existing service gate, before its normal cancel."""
from pathlib import Path
import difflib,hashlib,json,shutil,subprocess
ROOT=Path(__file__).resolve().parents[2];AREA=Path(__file__).resolve().parent
BASE=AREA/'source-loss-r2';TARGET=AREA/'service-loss-r1'
def main():
    if TARGET.exists():raise RuntimeError('Preserve named candidate')
    for name in ('playground','vr-common','third-party','native-capture'):
        shutil.copytree(BASE/'source/tools'/name,TARGET/'source/tools'/name)
    rel='tools/playground/weapon_hammerburst_burst_event.inl';before=(BASE/'source'/rel).read_text()
    old='''    if(tx::cancel_requested||vrfire::mode.load(std::memory_order_acquire)!=vrfire::running||
       !weaponhand::client.armed()||(owns_pending()&&common::pending_fire.event.release_required(weaponhand::clock_ns(nullptr)))){
        tx::revoke();release_completed(nullptr);return true;
    }'''
    new='''    unsigned service_rejection{};
    const auto service_failed=[&](unsigned gate_number,bool gate_failed)noexcept {
        if(gate_failed&&!service_rejection)service_rejection=gate_number;return gate_failed;
    };
    if(service_failed(1,tx::cancel_requested)||service_failed(2,vrfire::mode.load(std::memory_order_acquire)!=vrfire::running)||
       service_failed(3,!weaponhand::client.armed())||service_failed(4,owns_pending()&&common::pending_fire.event.release_required(weaponhand::clock_ns(nullptr)))){
#include "weapon_hammerburst_service_loss_observation.inl"
        tx::revoke();release_completed(nullptr);return true;
    }'''
    assert before.count(old)==1;after=before.replace(old,new);(TARGET/'source'/rel).write_text(after)
    helper='tools/playground/weapon_hammerburst_service_loss_observation.inl';shutil.copy2(ROOT/helper,TARGET/'source'/helper)
    cmake='tools/playground/CMakeLists.txt';old_cmake=(BASE/'source'/cmake).read_text();new_cmake=old_cmake+'\n# Bounded private service-gate observation only.\ntarget_compile_definitions(GearsPlayground PRIVATE GEARS_PLAYGROUND_HB_SERVICE_LOSS_OBSERVE=1)\n';(TARGET/'source'/cmake).write_text(new_cmake)
    patch=''.join(difflib.unified_diff(before.splitlines(True),after.splitlines(True),fromfile='a/'+rel,tofile='b/'+rel))
    (ROOT/'docs/hammerburst-service-loss-observation.patch').write_text(patch)
    flags=json.loads((BASE/'prepared.json').read_text());configure=list(flags['configure_command']);build=list(flags['build_command'])
    configure[configure.index('-S')+1]=str(TARGET/'source/tools/playground');configure[configure.index('-B')+1]=str(TARGET/'build')
    build[build.index('--build')+1]=str(TARGET/'build')
    for label,command in (('configure',configure),('build',build)):
        with (TARGET/(label+'.log')).open('wb') as log:subprocess.run(command,check=True,stdout=log,stderr=subprocess.STDOUT,creationflags=subprocess.CREATE_NO_WINDOW)
    dll=TARGET/'build/Release/GearsPlayground6DofUnified3.dll'
    proof=dict(base=str(BASE),full_playground_compiled=True,runtime_exercised=False,gameplay_accepted=False,
        observation_only=True,maximum_rows=8,same_gate_evaluation_order=True,extra_clock_reads=False,
        release_or_lifetime_policy_changed=False,native_rebuilt=False,xr_rebuilt=False,
        dll=str(dll),dll_sha256=hashlib.sha256(dll.read_bytes()).hexdigest(),configure_command=configure,build_command=build)
    (TARGET/'prepared.json').write_text(json.dumps(proof,indent=2))
    (TARGET/'source-verification.json').write_text(json.dumps({n:hashlib.sha256((TARGET/'source'/n).read_bytes()).hexdigest() for n in (rel,helper,cmake)},indent=2))
    print(json.dumps({k:proof[k] for k in ('full_playground_compiled','dll_sha256','gameplay_accepted')}),flush=True)
if __name__=='__main__':main()
