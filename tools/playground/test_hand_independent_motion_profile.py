"""Bounded extraction/stub CPU proof for the private independent-hand profile.

The compiled lifecycle functions and predicates are exact current source bytes.
Host restore/finish/end/component services are explicit stubs. Pure hand and
component journals and certified age/source policy are the actual headers.
This does not execute a game helper, tail writer, renderer, game or XR runtime.
"""
from pathlib import Path
import argparse
import datetime
import hashlib
import json
import os
import re
import subprocess
import sys


WORK = Path(__file__).resolve().parents[2]
DEFAULT_OUT = WORK / "analysis/hand-independent-motion-20261025bp" / (
    "lifecycle-cpu-" + datetime.datetime.now(datetime.timezone.utc).strftime("%Y%m%d-%H%M%S-%f")
)
STREAM = WORK / "tools/playground/vr_hand_ik_stream_trial.inl"
PHASE1 = WORK / "tools/playground/vr_hand_weapon_mesh_phase1.inl"
XR = WORK / "tools/playground/vr_hand_ik_xr_stream_trial.inl"
COMMON = WORK / "tools/vr-common"


def sha(raw):
    return hashlib.sha256(raw).hexdigest()


def pin(path):
    raw = path.read_bytes()
    return {"path": str(path), "bytes": len(raw), "sha256": sha(raw)}


HOST = r'''// Host dependencies below are controlled CPU seams, not game services.
#include <array>
#include <atomic>
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <cwchar>
#include "hand_ik_stream_lease.h"
#include "weapon_component_continuous_lease.h"
#include "hand_stream_ticket_policy.h"
#include "stereo_frame.h"
#ifdef NDEBUG
#error This fixture requires active assertions
#endif
namespace {
unsigned resume_calls{},component_apply_calls{},component_acquires{},component_updates{},restore_calls{},hand_commits{},hand_restores{};
unsigned end_calls{},last_end{},finish_calls{},publications{};
bool dual_draining{},hand_writes_relinquished{},healthy=true;
std::atomic<bool> stop_requested{},shutdown_requested{},safe_to_unload{true};
struct Trial {gears_hand_ik_stream::Lease lease;bool ended{},releasing{},xr_ready{true};} trial;
namespace component {gears_weapon::WeaponMeshComponentContinuousLease<unsigned> lease;}
namespace weaponrestore {inline bool healthy(){return ::healthy;}}
inline bool component_dirty(){return component::lease.dirty();}
inline bool component_faulted(){return component::lease.faulted();}
inline bool journals_dirty(){return trial.lease.dirty()||component_dirty();}
inline void publish_journals(){++publications;}
bool component_hand_only_ready();
bool component_forward_open();
namespace component_names {struct Scope {explicit Scope(bool){}~Scope(){}};}
namespace component_continuous {
#include "declarations.exact.inl"
}
inline void end(unsigned code,wchar_t const*){++end_calls;last_end=code;trial.ended=true;component_continuous::state.terminal=true;}
inline bool component_forward_open(){return !component_continuous::state.closed&&!component_continuous::state.terminal;}
[[maybe_unused]] bool component_has_interval(){return component::lease.has_snapshot();}
[[maybe_unused]] bool component_owned(){return !component::lease.faulted()&&!component::lease.revoked_mask();}
// Explicit external-service stub. Independent mode must never reach it.
[[maybe_unused]] bool component_resume_interval(){++resume_calls;return false;}
struct HandBytes {
    std::array<gears_hand_ik_stream::Value,4> values{};
    bool read(unsigned channel,gears_hand_ik_stream::Value& out){out=values[channel];return true;}
    bool write_target(unsigned channel,gears_hand_ik_stream::Target const& before,gears_hand_ik_stream::Target const& wanted){
        assert(values[channel].target==before);values[channel].target=wanted;return true;
    }
    bool set_strength(unsigned channel,float value){values[channel].strength={{value,value,0}};return true;}
} hand_bytes;
// Host restore seam exercises the real primary journal, not production tail/
// reflection/engine rollback. Only its host hand-only marker is finalized here.
inline bool restore(wchar_t const*){
    ++restore_calls;const bool owned=trial.lease.dirty();
    const bool clean=trial.lease.restore(hand_bytes);
    if(clean){component_continuous::state.hand_only_journal=false;if(owned)++hand_restores;}
    return clean;
}
namespace component_continuous {
inline bool finish(){++finish_calls;return !journals_dirty();}
inline bool pause_pending(){return false;}
inline void suspend_interval(wchar_t const*,bool){assert(false&&"No owned component interval is supplied by this fixture");}
#include "normal_tick.exact.inl"
#include "post_tick_destructor.exact.inl"
}
#include "hand_only_ready.exact.inl"
unsigned configured_source(){
#include "source_selector.exact.inl"
    return continuity_source;
}
struct Plan {unsigned accept_mask{3},reset_binding_mask{};};
void select_mode(component_continuous::PostTick& continuous_tick,unsigned equipment_phase){
    const Plan plan{};const bool fresh=true,qualified=true;
    (void)plan;(void)equipment_phase;(void)fresh;(void)qualified;
#include "mode_selector.exact.inl"
}
struct Frame {};struct Snapshot {};
// Count this external dispatch seam. No real production component backend runs.
inline bool component_apply(Plan const&,Frame const&,Snapshot const&,unsigned,unsigned){
    ++component_apply_calls;if(component::lease.has_snapshot())++component_updates;else ++component_acquires;return true;
}
inline bool drain_journals(wchar_t const*){assert(false&&"No production drain is supplied");return false;}
void dispatch_component(component_continuous::PostTick const& continuous_tick){
    const Plan plan{};const Frame frame{};const Snapshot check{};const unsigned mask=3,lease=1;
    (void)continuous_tick;
#include "component_dispatch.exact.inl"
}
struct ComponentBytes {
    using Value=gears_weapon::WeaponMeshComponentValue;using Absolute=gears_weapon::MeshComponentAbsolute;
    using Bytes=gears_weapon::MeshComponentBytes12;Value value{};
    bool read(gears_weapon::WeaponMeshComponentState<unsigned>& out){out={7,value,true};return true;}
    bool set_absolute(unsigned,Value const&,Absolute const& wanted){value.absolute=wanted;return true;}
    bool set_translation(unsigned,Value const&,Bytes const& wanted){value.translation=wanted;return true;}
    bool set_rotation(unsigned,Value const&,Bytes const& wanted){value.rotation=wanted;return true;}
    template<class Permit>bool set_absolute(unsigned key,Value const& before,Absolute const& wanted,Permit& permit){return permit()&&set_absolute(key,before,wanted);}
    template<class Permit>bool set_translation(unsigned key,Value const& before,Bytes const& wanted,Permit& permit){return permit()&&set_translation(key,before,wanted);}
    template<class Permit>bool set_rotation(unsigned key,Value const& before,Bytes const& wanted,Permit& permit){return permit()&&set_rotation(key,before,wanted);}
};
[[maybe_unused]] void dirty_component(){
    ComponentBytes bytes;auto wanted=bytes.value;wanted.translation[0]=1;
    auto permit=[](unsigned,unsigned,auto const&,auto const&,unsigned){return true;};
    assert(component::lease.acquire(7,wanted,1u,permit,bytes)&&component_dirty()&&!component_faulted());
}
void suspended(){
    auto& state=component_continuous::state;
    state.interval=component_continuous::Interval::suspended;state.closed=true;
    state.drain_attempted=true;state.last_drain_result=true;state.clean_closes=17;state.last_clean_interval=8;
    component::lease.cancel_forward();
}
void no_component_forward(){assert(!resume_calls&&!component_apply_calls&&!component_acquires&&!component_updates);}
void commit_hand(unsigned ordinal){
    component_continuous::PostTick tick;select_mode(tick,1);
    assert(tick.mode==component_continuous::Mode::hand_only);dispatch_component(tick);no_component_forward();
    std::array<gears_hand_ik_stream::Target,4> wanted{};
    for(auto& target:wanted)target[0]=static_cast<unsigned char>(ordinal);
    assert(trial.lease.acquire(wanted,1.f,hand_bytes));
    auto& state=component_continuous::state;state.hand_only_journal=true;state.hand_only_started=true;
    tick.hand_committed=true;++hand_commits;
}
[[maybe_unused]] void waiting_case(){
    auto& state=component_continuous::state;
    assert(component_hand_only_ready()&&component_continuous::normal_tick());
    assert(state.interval==component_continuous::Interval::waiting&&!state.closed&&!end_calls);
    commit_hand(1);assert(component_continuous::normal_tick());
    assert(hand_commits==1&&hand_restores==1&&!journals_dirty()&&!end_calls);no_component_forward();
}
[[maybe_unused]] void suspended_twice(){
    suspended();auto& state=component_continuous::state;
    for(unsigned n=1;n<=2;++n){assert(component_hand_only_ready()&&component_continuous::normal_tick());commit_hand(n);}
    assert(component_continuous::normal_tick());
    assert(hand_commits==2&&hand_restores==2&&!trial.lease.dirty()&&!state.hand_only_journal&&!end_calls);
    assert(state.interval==component_continuous::Interval::suspended&&state.closed&&state.drain_attempted&&state.last_drain_result);
    assert(state.clean_closes==17&&state.last_clean_interval==8&&component::lease.forward_stopped());no_component_forward();
}
[[maybe_unused]] void rejection_case(char const* kind){
    const bool dirty=!std::strcmp(kind,"dirty"),revocation=!std::strcmp(kind,"revoked");
    if(dirty||revocation)dirty_component();
    suspended();auto& state=component_continuous::state;
    if(revocation)component::lease.relinquish();
    else if(!dirty)
    {state.last_drain_result=false;state.failure_known=true;state.cleanup_failed=true;state.first_failure=71;
        constexpr wchar_t reason[]=L"existing drain fault";std::wmemcpy(state.failure_reason.data(),reason,sizeof(reason)/sizeof(*reason));}
    const auto reason=state.failure_reason;const auto first=state.first_failure;const bool known=state.failure_known,cleanup=state.cleanup_failed;
    const auto dirty_mask=component::lease.dirty_mask(),revoked=component::lease.revoked_mask();const bool faulted=component_faulted();
    assert(!component_hand_only_ready()&&!component_continuous::normal_tick());
    assert(end_calls==1&&state.interval==component_continuous::Interval::suspended&&state.closed);
    assert(state.failure_reason==reason&&state.first_failure==first&&state.failure_known==known&&state.cleanup_failed==cleanup);
    assert(component::lease.dirty_mask()==dirty_mask&&component::lease.revoked_mask()==revoked&&component_faulted()==faulted);
    assert(!hand_commits&&!hand_restores);no_component_forward();
}
[[maybe_unused]] void off_branch(){
    auto& state=component_continuous::state;state.resume_stable=true;
    assert(component_continuous::normal_tick());
    {component_continuous::PostTick tick;select_mode(tick,1);assert(tick.mode==component_continuous::Mode::joint);
        dispatch_component(tick);tick.committed=true;}
    assert(component_acquires==1&&!resume_calls&&!end_calls);
}
[[maybe_unused]] void off_resume(){suspended();assert(!component_continuous::normal_tick());assert(resume_calls==1&&end_calls==1);}
void source_and_age(){
    static_assert(gears_hand_stream::maximum_age_ms==100);
    const auto source=configured_source();assert(source<=1);
    gears_hand_stream::Policy policy;assert(policy.enable_certified_calibration(700,source));
    gears_hand_stream::Context context{{11,12,13,14,15},7,3,1000,true,true};
    gears_hand_stream::Ticket ticket{{1,2,3,7},1,1,1000,12,13,15,100.,3};
    auto& cert=ticket.certificate;cert.native_started_qpc=700;cert.xr_publisher_pid=81;cert.xr_publisher_start=91;
    cert.stream_epoch=cert.reference_epoch=cert.reference_generation=cert.profile_generation=1;
    cert.tracking_epoch[0]=cert.tracking_epoch[1]=1;cert.source=source;
    cert.stereo_received_ns=cert.input_received_ns=1000000000;
    gears_hand_stream::Plan plan{};
    for(std::uint64_t n=1;n<=2;++n){ticket.serial=ticket.sequence=n;ticket.created_ms=1000+n;context.observed_ms=ticket.created_ms;
        plan=policy.observe_certified(context,&ticket,ticket.created_ms,1000000000);}
    assert(plan.accept_mask==3&&plan.bind_mask==3&&policy.commit_binding(plan,3));
    assert(gears_hand_continuity::certificate_recent(cert,1100000000));
    assert(!gears_hand_continuity::certificate_recent(cert,1100000001));
    auto stereo_old=cert;--stereo_old.stereo_received_ns;
    auto input_old=cert;--input_old.input_received_ns;
    assert(!gears_hand_continuity::certificate_recent(stereo_old,1100000000));
    assert(!gears_hand_continuity::certificate_recent(input_old,1100000000));
    assert(policy.authorize_certified(plan,context,1102,1100000000)==3);
    assert(!policy.authorize_certified(plan,context,1102,1100000001));
    assert(!policy.authorize_certified(plan,context,1103,1100000000));
    gears_hand_stream::Policy mismatch;assert(mismatch.enable_certified_calibration(700,source));cert.source=source^1;
    const auto refused=mismatch.observe_certified(context,&ticket,context.observed_ms,1000000000);
    assert(!refused.accept_mask&&mismatch.certified_failed());
}
}
int main(int argc,char** argv){
    assert(argc==2);const char* mode=argv[1];
    if(!std::strcmp(mode,"source_age"))source_and_age();
#if defined(GEARS_PLAYGROUND_HAND_INDEPENDENT_MOTION_TRIAL)
    else if(!std::strcmp(mode,"waiting"))waiting_case();
    else if(!std::strcmp(mode,"suspended_twice"))suspended_twice();
    else if(!std::strcmp(mode,"dirty")||!std::strcmp(mode,"failed_drain")||!std::strcmp(mode,"revoked"))rejection_case(mode);
#else
    else if(!std::strcmp(mode,"off_branch"))off_branch();
    else if(!std::strcmp(mode,"off_resume"))off_resume();
#endif
    else{std::fprintf(stderr,"unsupported mode: %s\n",mode);return 2;}
    std::printf("{\"mode\":\"%s\",\"source\":%u,\"passed\":true,\"hand_commits\":%u,\"hand_restores\":%u,\"restore_calls\":%u,\"resume_calls\":%u,\"component_apply_calls\":%u,\"component_acquires\":%u,\"component_updates\":%u,\"end_calls\":%u,\"last_end\":%u}\n",
        mode,configured_source(),hand_commits,hand_restores,restore_calls,resume_calls,component_apply_calls,component_acquires,component_updates,end_calls,last_end);
}
'''


def extract_sources(out):
    data = {path: path.read_bytes() for path in (STREAM, PHASE1, XR)}
    records = []

    def save(name, path, start, end):
        raw = data[path]
        fragment = raw[start:end]
        target = out / (name + ".exact.inl")
        target.write_bytes(fragment)
        if target.read_bytes() != raw[start:end]:
            raise AssertionError("Extraction changed source bytes: " + name)
        records.append({"name": name, "source": str(path), "source_sha256": sha(raw),
                        "byte_start_inclusive": start, "byte_end_exclusive": end,
                        "line_start": raw[:start].count(b"\n") + 1,
                        "line_end": raw[:end - 1].count(b"\n") + 1,
                        "bytes": len(fragment), "sha256": sha(fragment), "output": str(target),
                        "verbatim_source_range": True})

    def between(name, path, start_marker, end_marker):
        raw = data[path]
        if raw.count(start_marker) != 1:
            raise AssertionError("Nonunique start marker: " + name)
        start = raw.index(start_marker)
        end = raw.index(end_marker, start + len(start_marker))
        save(name, path, start, end)

    between("declarations", STREAM, b"// Adapter lifecycle only. These flags never replace a Plan, raw read or Permit.", b"struct Action {")
    between("normal_tick", STREAM, b"bool normal_tick(){", b"PostTick::~PostTick(){")
    between("post_tick_destructor", STREAM, b"PostTick::~PostTick(){", b"} // namespace component_continuous")
    between("hand_only_ready", PHASE1, b"bool component_hand_only_ready(){", b"bool component_resume_interval(){")
    selector = rb"^#if defined\(GEARS_PLAYGROUND_HAND_INDEPENDENT_MOTION_TRIAL\)\r?\n[ \t]+constexpr unsigned continuity_source=.*?^#endif\r?\n"
    matches = list(re.finditer(selector, data[XR], re.M | re.S))
    if len(matches) != 1:
        raise AssertionError("Expected one production source selector")
    save("source_selector", XR, matches[0].start(), matches[0].end())
    mode_start = rb"^#if defined\(GEARS_PLAYGROUND_HAND_INDEPENDENT_MOTION_TRIAL\)\r?\n[ \t]+constexpr bool joint_ready=false;"
    matches = list(re.finditer(mode_start, data[XR], re.M))
    if len(matches) != 1:
        raise AssertionError("Expected one production joint selector")
    start = matches[0].start()
    stop = re.search(rb"^#endif\r?\n[ \t]+if\(!plan.accept_mask\)return;", data[XR][start:], re.M)
    if stop is None:
        raise AssertionError("Missing complete mode selection boundary")
    save("mode_selector", XR, start, start + stop.start())
    dispatch = rb"^#if defined\(GEARS_PLAYGROUND_WEAPON_COMPONENT_PERSISTENT_SESSION_TRIAL\)\r?\n[ \t]+if\(continuous_tick.mode==component_continuous::Mode::joint\)\r?\n#endif\r?\n[ \t]+if\(!component_apply\(plan,frame,check,mask,lease\)\)\{.*?^    \}\r?\n"
    matches = list(re.finditer(dispatch, data[XR], re.M | re.S))
    if len(matches) != 1:
        raise AssertionError("Expected one exact component dispatch guard")
    save("component_dispatch", XR, matches[0].start(), matches[0].end())
    return records


def run(args):
    out = args.out_dir.resolve()
    if (out / "result.json").exists():
        raise FileExistsError("Preserve the existing result; select a fresh lifecycle output directory: " + str(out))
    out.mkdir(parents=True, exist_ok=True)
    paths = (STREAM, PHASE1, XR, Path(__file__).resolve(),
             COMMON / "hand_ik_stream_lease.h", COMMON / "weapon_component_continuous_lease.h",
             COMMON / "hand_stream_ticket_policy.h", COMMON / "hand_continuity_certificate.h")
    before = [pin(path) for path in paths]
    report = {"schema": 1, "passed": False, "scope": "verbatim production lifecycle/predicate with explicit host stubs",
              "limits": ["No production tail/engine restore helper, reflection, game body, renderer, game or XR runtime executed.",
                         "Host lifecycle services are stubs; real primary/pure journals do not prove the tail writer or game lifecycle.",
                         "Source0/1 policy checks are CPU evidence, not publisher/runtime compatibility."],
              "timestamp_utc": datetime.datetime.now(datetime.timezone.utc).isoformat(),
              "source_pins_before": before, "builds": [], "runs": []}
    try:
        report["extractions"] = extract_sources(out)
        cpp = out / "lifecycle-host.cpp"
        cpp.write_text(HOST, encoding="utf-8", newline="\n")
        report["host"] = pin(cpp)
        report["compiler"] = pin(args.cl)
        report["vcvars"] = pin(args.vcvars)
        strict = ["/nologo", "/std:c++17", "/W4", "/WX", "/EHsc", "/UNDEBUG", "/MT", "/O2", "/DNOMINMAX", "/DWIN32_LEAN_AND_MEAN"]
        report["strict_flags"] = strict
        base_defs = ["GEARS_PLAYGROUND_WEAPON_COMPONENT_CONTINUOUS_TRIAL", "GEARS_PLAYGROUND_WEAPON_COMPONENT_PERSISTENT_SESSION_TRIAL"]
        profiles = (("source0", 0, True), ("source1", 1, True), ("profile-off", 0, False))
        for profile, source, enabled in profiles:
            definitions = list(base_defs)
            if enabled:
                definitions += ["GEARS_PLAYGROUND_HAND_INDEPENDENT_MOTION_TRIAL", f"GEARS_PLAYGROUND_HAND_MOTION_EXPECTED_SOURCE={source}"]
            exe = out / (profile + ".exe")
            command = [str(args.cl), *strict, *("/D" + value for value in definitions),
                       "/I" + str(COMMON), "/sourceDependencies", str(out / (profile + ".dependencies.json")),
                       "/Fo" + str(out / (profile + ".obj")), "/Fe" + str(exe), str(cpp)]
            command_text = subprocess.list2cmdline(command)
            batch = out / ("build-" + profile + ".cmd")
            lines = ["@echo off", "setlocal", 'call "' + str(args.vcvars) + '" >"' + str(out / (profile + ".environment.log")) + '" 2>&1',
                     'if not "%ERRORLEVEL%"=="0" exit /b %ERRORLEVEL%', 'set "CL="', 'set "_CL_="', command_text, "exit /b %ERRORLEVEL%"]
            batch.write_text("\r\n".join(lines) + "\r\n", encoding="utf-8")
            proc = subprocess.run([os.environ.get("COMSPEC", "cmd.exe"), "/d", "/c", str(batch)],
                                  capture_output=True, text=True, errors="replace", timeout=90,
                                  creationflags=getattr(subprocess, "CREATE_NO_WINDOW", 0))
            log = out / ("build-" + profile + ".log")
            log.write_text(proc.stdout + proc.stderr, encoding="utf-8")
            report["builds"].append({"profile": profile, "definitions": definitions, "command": command_text,
                                     "batch": str(batch), "log": str(log), "exit_code": proc.returncode})
            if proc.returncode:
                raise RuntimeError(f"Strict build failed: {profile}; see {log}")
            report["builds"][-1]["executable"] = pin(exe)
            modes = ("waiting", "suspended_twice", "dirty", "failed_drain", "revoked", "source_age") if enabled else ("off_branch", "off_resume", "source_age")
            for mode in modes:
                proc = subprocess.run([str(exe), mode], capture_output=True, text=True, errors="replace", timeout=15,
                                      creationflags=getattr(subprocess, "CREATE_NO_WINDOW", 0))
                log = out / ("run-" + profile + "-" + mode + ".log")
                log.write_text(proc.stdout + proc.stderr, encoding="utf-8")
                row = {"profile": profile, "mode": mode, "command": [str(exe), mode], "process_isolated": True,
                       "exit_code": proc.returncode, "log": str(log)}
                report["runs"].append(row)
                if proc.returncode:
                    raise RuntimeError(f"CPU fixture failed: {profile}/{mode}; see {log}")
                row["receipt"] = json.loads(proc.stdout)
                if row["receipt"]["passed"] is not True or row["receipt"]["source"] != source:
                    raise AssertionError("Unexpected executable receipt")
        after = [pin(path) for path in paths]
        report["source_pins_after"] = after
        if before != after:
            raise AssertionError("Source changed during extraction/build/test")
        report["source_unchanged"] = True
        report["passed"] = True
    except BaseException as exc:
        report["error"] = f"{type(exc).__name__}: {exc}"
        raise
    finally:
        (out / "result.json").write_text(json.dumps(report, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")
    print(f"Passed: {len(report['builds'])} strict builds; {len(report['runs'])} isolated lifecycle/source-age CPU processes; {out / 'result.json'}")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--out-dir", type=Path, default=DEFAULT_OUT)
    parser.add_argument("--cl", type=Path, default=Path("E:/vs/VC/Tools/MSVC/14.51.36231/bin/Hostx64/x64/cl.exe"))
    parser.add_argument("--vcvars", type=Path, default=Path("E:/vs/VC/Auxiliary/Build/vcvars64.bat"))
    run(parser.parse_args())
