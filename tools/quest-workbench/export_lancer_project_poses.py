"""Export four saved LEFT project poses, offline; never a new capture or live input."""
from pathlib import Path
import argparse,copy,json,math
import numpy as np
import export_lancer_magazine_volume as geometry
import adopt_moving_lancer_captures as moving


def export(project: Path, expected_sha256: str, parts_asset: Path):
    data,digest=geometry.project_data(project,expected_sha256)
    pivot_xr=json.loads(parts_asset.read_text("utf-8"))["starter_pivot_metres"]
    pivot=moving.legacy.A.T@np.asarray(geometry.vector(pivot_xr,3))*100
    rows={}
    for index in range(1,5):
        part=data["parts"][index]
        hands=[h for h in part["hands"] if h.get("side")=="left"]
        if len(hands)!=1:raise ValueError("Expected exactly one saved left pose")
        hand=hands[0]
        for field,count in [("finger_correction_degrees",15)]:
            if len(hand[field])!=count:raise ValueError("Invalid correction count")
            for correction in hand[field]:
                if math.hypot(*geometry.vector(correction,3))>180:raise ValueError("Finger correction out of range")
        if math.hypot(*geometry.vector(hand["wrist_offset_metres"],3))>.3:raise ValueError("Wrist offset out of range")
        if math.hypot(*geometry.vector(hand["wrist_correction_degrees"],3))>180:raise ValueError("Wrist correction out of range")
        selected=copy.deepcopy(data)
        selected["capture"]={"side":0,"part":index,"id":"saved-project-"+part["id"],"name":"Saved project pose; not a new capture"}
        row=moving.convert_selected(selected,data["model_content_fnv64"],pivot)
        row.pop("capture_id");row.pop("name")
        row["source_time_ns"]=hand["source_time_ns"]
        rows[part["id"]]=row
    return {"schema":1,"source_project":str(project),"source_sha256":digest,"provenance":"Saved project authoring, including corrections; no capture event manufactured","right_hand_changed":False,"applied":False,"game_acceptance":False,"poses":rows}

if __name__=="__main__":
    p=argparse.ArgumentParser(description=__doc__);p.add_argument("--project",type=Path,required=True);p.add_argument("--expect-sha256",required=True);p.add_argument("--parts-asset",type=Path,default=Path(__file__).parent/"assets/Lancer.parts.json");p.add_argument("--output",type=Path,required=True);a=p.parse_args()
    result=export(a.project,a.expect_sha256,a.parts_asset)
    with a.output.open("x",encoding="utf-8") as f:json.dump(result,f,ensure_ascii=False,indent=2)
    print("Exported four saved left poses; not applied")
