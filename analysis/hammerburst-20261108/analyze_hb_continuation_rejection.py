"""Read HB failure receipts only; no process, input, game or filesystem mutation."""
import re
RELEASE={1:"no_frame",2:"not_running",3:"not_committed",4:"trigger_channel_inactive",5:"trigger_still_down",6:"frame_not_active",7:"right_not_tracked",8:"game_read_failed",9:"game_unknown",10:"game_r2_unknown",11:"game_r2_held",12:"game_busy",13:"game_animation",14:"reload_blocks_fire",15:"equipment_not_current",16:"need_source_rejected"}
CONTINUATION={1:"no_frame",2:"not_running",3:"cancel_already_requested",4:"target_not_current",5:"right_not_tracked",6:"equipment_not_current",7:"need_source_rejected"}
SOURCE={1:"certificate_cancel_or_active_scope",2:"cohort_changed",3:"first_debt_trigger_qualification",4:"not_committed",5:"completion_observe_rejected"}
def read_receipts(text):
    out=[]
    for number,line in enumerate(text.splitlines(),1):
        phase=next((p for p in ("release","continuation") if line.startswith("native_hammerburst_"+p+"_rejected ")),None)
        if phase is None:continue
        pairs=re.findall(r"(\w+)=([^\s]+)",line)
        if len(dict(pairs))!=len(pairs):raise ValueError("Duplicate receipt field")
        fields=dict(pairs);required={"gate","source_stage","frame","count","first_debt","firing_debt","cancel"}
        if not required<=fields.keys():raise ValueError("Incomplete failure receipt")
        values={k:int(fields[k]) for k in required}
        gates=RELEASE if phase=="release" else CONTINUATION
        if values["gate"] not in gates:raise ValueError("Unknown gate; source version mismatch")
        if any(values[k] not in (0,1) for k in ("first_debt","firing_debt","cancel")) or not 0<=values["count"]<=6 or values["frame"]<0:raise ValueError("Invalid typed receipt")
        used=values["gate"]==(16 if phase=="release" else 7)
        stage=values["source_stage"] if used else None
        if used and stage not in SOURCE:raise ValueError("Unknown actual need_source rejection")
        out.append(dict(line=number,phase=phase,first_failed_gate=gates[values["gate"]],frame=values["frame"],source_stage_is_current=used,source_rejection=SOURCE[stage] if used else None,recorded_source_stage=values["source_stage"],count=values["count"],cancel_already_requested=bool(values["cancel"]),root_cause_proven=False))
    return dict(receipts=out,receipt_count=len(out),missing_receipt=not out,limits="Observed failed predicate only. Earlier cancellation, raw game/identity change and original engine cause remain unproven; no retry or timing recommendation.")
