"""Extend exact observer fixtures with released-shoulder marker rejection/caps."""
from pathlib import Path
import ast,hashlib,json,subprocess
A=Path(__file__).resolve().parent;P=A/'pickup-baseline-r1';COMMON=P/'source/tools/vr-common'
tree=ast.parse((A/'check_inventory_timing.py').read_text())
source=next(ast.literal_eval(n.value) for n in tree.body if isinstance(n,ast.Assign) and any(isinstance(x,ast.Name) and x.id=='source' for x in n.targets))
source=source.replace('FILE* log_file{};',r'''
namespace gears_equipment_input {enum class Status{baseline,quiet,other};const char* status_name(Status s){return s==Status::baseline?"baseline":s==Status::quiet?"quiet":"other";}}
struct Evidence{std::uint64_t sequence{};std::int64_t sync_ns{};gears_equipment_input::Status status{gears_equipment_input::Status::baseline};}input_evidence;
FILE* log_file{};''')
needle=' std::rewind(log_file);wchar_t line[2048]{};unsigned rows{};'
assert source.count(needle)==1
source=source.replace(needle,r'''
 f.controllers.hands[1].squeeze=0;
 f.controllers.hands[1].grip.position={.23,-.115,-.17};
 {HammerburstInventoryTiming t(f);t.mark(1);t.mark(2);t.mark(3);t.mark(4);}
 assert(clock_reads==92);++cases; // RuntimeTracked cannot produce the private marker.
 f.stereo.source=gears_vr::StereoSource::MetaOperatorSimulated;
 input_evidence.sequence=99;input_evidence.sync_ns=500;
 {HammerburstInventoryTiming t(f);t.mark(1);t.mark(2);t.mark(3);t.mark(4);}
 input_evidence.sequence=2;input_evidence.sync_ns=999;
 {HammerburstInventoryTiming t(f);t.mark(1);t.mark(2);t.mark(3);t.mark(4);}
 input_evidence.sync_ns=500;input_evidence.status=gears_equipment_input::Status::other;
 {HammerburstInventoryTiming t(f);t.mark(1);t.mark(2);t.mark(3);t.mark(4);}
 input_evidence.status=gears_equipment_input::Status::baseline;
 {HammerburstInventoryTiming t(f);t.mark(1);t.mark(2);t.mark(3);}
 ++cases;
 for(unsigned i=0;i<12;++i){HammerburstInventoryTiming t(f);t.mark(1);t.mark(2);t.mark(3);t.mark(4);}
 assert(HammerburstInventoryTiming::shoulder_rows==16&&clock_reads==187&&withdrawal_reads==32);
 {HammerburstInventoryTiming t(f);t.mark(1);}
 assert(clock_reads==187&&withdrawal_reads==32);++cases;
 std::rewind(log_file);wchar_t line[2048]{};unsigned rows{},up_markers{};
''')
source=source.replace('++rows;}','if(std::wcsstr(line,L"hammerburst_pickup_up "))++up_markers;++rows;}')
source=source.replace('assert(rows==16);','assert(rows==44&&up_markers==12);')
source=source.replace('HB exact inventory timing observer','HB exact shoulder baseline observer')
target=P/'source/tools/playground/hammerburst_pickup_baseline_observer_test.cpp';target.write_text(source)
cmd=P/'pickup-baseline-observer-test.cmd';exe=P/'pickup-baseline-observer-test.exe'
cmd.write_text('@echo off\ncall "E:/vs/VC/Auxiliary/Build/vcvars64.bat" >nul\nif errorlevel 1 exit /b %errorlevel%\n'+
 f'cl /nologo /O2 /std:c++17 /W4 /WX /UNDEBUG /EHsc /I"{COMMON}" "{target}" /Fo"{P / "baseline-observer-test.obj"}" /Fe"{exe}"\n'+
 f'if errorlevel 1 exit /b %errorlevel%\n"{exe}"\nexit /b %errorlevel%\n')
with (P/'baseline-observer-test.log').open('wb') as log:
 result=subprocess.run(['cmd','/c',str(cmd)],stdout=log,stderr=subprocess.STDOUT,creationflags=subprocess.CREATE_NO_WINDOW)
print((P/'baseline-observer-test.log').read_text(errors='replace'));result.check_returncode()
(P/'baseline-observer-test-result.json').write_text(json.dumps(dict(passed=True,cases=9,exact_observer=True,
 source_input_time_status_match_required=True,maximum_shoulder_up_rows=16,game_execution=False,
 source_sha256=hashlib.sha256(target.read_bytes()).hexdigest()),indent=2))
