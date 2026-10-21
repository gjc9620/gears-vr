"""Validated local armspan record for the opt-in 6DoF controller observer."""
import json
import math
import os
import pathlib
import re
import tempfile

NATIVE_LINE=re.compile(
    r'^controller_armspan_observer measured=(?P<measured>[01]) progress=\S+ '
    r'armspan_metres=(?P<span>\S+) hands_below_head_metres=(?P<height>\S+) saved=0$')


def validate(value):
    if (not isinstance(value,dict) or set(value)!={'schema','armspan_metres','hands_below_head_metres'} or
            type(value['schema']) is not int or value['schema']!=1):
        raise ValueError('Invalid armspan calibration schema')
    for key,lower,upper in (('armspan_metres',.9,2.4),('hands_below_head_metres',-.35,.1)):
        item=value[key]
        if isinstance(item,bool) or not isinstance(item,(int,float)) or not math.isfinite(item) or not lower<=item<=upper:
            raise ValueError('Invalid armspan calibration '+key)
    return {'schema':1,'armspan_metres':float(value['armspan_metres']),
            'hands_below_head_metres':float(value['hands_below_head_metres'])}


def load(path):
    path=pathlib.Path(path)
    if not path.exists():return None
    return validate(json.loads(path.read_text(encoding='utf-8')))


def measured_from_native(text):
    found=[]
    for line in text.splitlines():
        match=NATIVE_LINE.fullmatch(line)
        if match and match['measured']=='1':
            found.append(validate({'schema':1,'armspan_metres':float(match['span']),
                                   'hands_below_head_metres':float(match['height'])}))
    if len(found)>1:raise ValueError('Ambiguous native armspan measurement')
    return found[0] if found else None


def save(path,value):
    value=validate(value);path=pathlib.Path(path);path.parent.mkdir(parents=True,exist_ok=True)
    temporary=None
    try:
        with tempfile.NamedTemporaryFile('w',encoding='utf-8',dir=path.parent,
                                         prefix=path.name+'.',suffix='.tmp',delete=False) as file:
            temporary=pathlib.Path(file.name)
            json.dump(value,file,allow_nan=False,indent=2)
            file.write('\n')
        os.replace(temporary,path)
    finally:
        if temporary is not None:
            try:temporary.unlink(missing_ok=True)
            except OSError:pass
