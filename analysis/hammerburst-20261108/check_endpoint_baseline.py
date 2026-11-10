"""Reuse actual observer fixtures; add rejection outside5mm on all axes."""
from pathlib import Path
A=Path(__file__).resolve().parent
script=(A/'check_pickup_baseline_observer.py').read_text().replace("P=A/'pickup-baseline-r1'","P=A/'endpoint-baseline-r1'")
anchor="target=P/'source/tools/playground/hammerburst_pickup_baseline_observer_test.cpp';target.write_text(source)"
assert script.count(anchor)==1
insertion=r'''
point=' f.stereo.source=gears_vr::StereoSource::MetaOperatorSimulated;'
assert source.count(point)==1
source=source.replace(point,point+r"""
 for(auto position:{gears_vr::Vector{.236,-.115,-.17},gears_vr::Vector{.23,-.109,-.17},gears_vr::Vector{.23,-.115,-.164}}){
  f.controllers.hands[1].grip.position=position;
  {HammerburstInventoryTiming t(f);t.mark(1);t.mark(2);t.mark(3);t.mark(4);}
 }
 assert(clock_reads==92&&HammerburstInventoryTiming::shoulder_rows==0);++cases;
 f.controllers.hands[1].grip.position={.23,-.115,-.17};
""")
'''
script=script.replace(anchor,insertion+'\n'+anchor).replace('cases=9,exact_observer=True','cases=10,exact_observer=True')
exec(compile(script,str(A/'check_pickup_baseline_observer.py'),'exec'),{'__file__':str(A/'check_pickup_baseline_observer.py'),'__name__':'__main__'})
