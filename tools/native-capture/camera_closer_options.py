"""Pure validation shared by the continuous runner and INI writer."""
import math

def camera_closer_metres(args):
    value=getattr(args,'camera_closer_metres',0.)
    if isinstance(value,bool) or not isinstance(value,(int,float)) or not math.isfinite(value) or not 0<=value<=.5:
        raise ValueError('camera_closer_metres must be finite and between 0 and 0.5')
    if value>0 and (not all(getattr(args,key,False) for key in ('anchor_observer','cinematic_camera')) or
                    getattr(args,'origin_trial',False)):
        raise ValueError('Camera closer requires cinematic/anchor guards and cannot use origin trial')
    return float(value)
