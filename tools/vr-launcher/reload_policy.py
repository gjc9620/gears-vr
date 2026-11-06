"""Weapon reward/UI timing; physical mechanics and ammo are unaffected."""
import configparser
import math
from pathlib import Path


def read_policy(path, weapon='Lancer'):
    path=Path(path)
    parser=configparser.ConfigParser(interpolation=None)
    if path.exists():
        with path.open(encoding='utf-8-sig') as stream:parser.read_file(stream)
    weapons=('Lancer','Snub','Hammerburst')
    if weapon not in weapons or set(parser.sections())-set(weapons) or parser.defaults():
        raise ValueError('6dof-reload.ini: supported sections are Lancer, Snub and Hammerburst')
    for section in parser.sections():
        if set(parser[section])-{'perfect_reload','tactical_seconds','empty_seconds'}:
            raise ValueError('6dof-reload.ini: unknown '+section+' setting')
    # Existing owner files need no rewrite. Optional weapon sections override
    # Lancer's values individually; omitted sections inherit the same timing.
    values=dict(parser['Lancer']) if parser.has_section('Lancer') else {}
    if parser.has_section(weapon):values.update(parser[weapon])
    mode=values.get('perfect_reload','vr').strip().lower()
    if mode not in ('off','original','vr'):
        raise ValueError('perfect_reload must be vr, original or off')
    def milliseconds(name,default):
        seconds=float(values.get(name,default))
        if not math.isfinite(seconds) or not .1<=seconds<=60:
            raise ValueError(name+' must be between 0.1 and 60 seconds')
        return round(seconds*1000)
    tactical=milliseconds('tactical_seconds',4.)
    empty=milliseconds('empty_seconds',4.)
    return dict(mode=mode,tactical_ms=tactical,empty_ms=empty,
                packed=tactical|(empty<<16)|(('off','original','vr').index(mode)<<32),path=str(path))
