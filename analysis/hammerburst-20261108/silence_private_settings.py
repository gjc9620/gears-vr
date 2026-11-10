"""Zero only the known audio categories in a fresh own private settings copy.

Does not change user settings, system/session volume, engine audio functions,
the shared launcher, or an instance which has already been started.
"""
from pathlib import Path
import hashlib,json,re

ROOT=Path(__file__).resolve().parents[2]
KEYS=('MusicVolume','FXVolume','DialogueVolume')


def prepare(selected):
    selected=Path(selected).resolve()
    if not selected.name.startswith('hb-') or selected.parent!=ROOT/'analysis/instances':raise ValueError('Expected own exact private instance')
    if (selected/'launcher-session.json').exists() or (selected/'isolated-worker.json').exists():raise ValueError('Never edit settings of a consumed instance')
    proof=json.loads((selected/'instance.json').read_text())
    if Path(proof['root']).resolve()!=selected:raise ValueError('Instance proof differs')
    directory=selected/'LocalAppData/Coalition/WarGame/Config'
    pending=[]
    for file in directory.glob('*EngineUserSettings.ini'):
        if not file.resolve().is_relative_to(selected):raise ValueError('Settings escape private root')
        raw=file.read_bytes();text=raw.decode('utf-8');section='';counts={k:0 for k in KEYS};lines=[]
        for line in text.splitlines(keepends=True):
            match=re.match(r'^\s*\[([^]]+)\]',line)
            if match:section=match.group(1)
            if section=='XAudio2.XAudio2Device':
                for key in KEYS:
                    if re.match(r'^\s*'+key+r'\s*=',line):
                        counts[key]+=1;eol='\r\n' if line.endswith('\r\n') else '\n' if line.endswith('\n') else ''
                        line=line.split('=',1)[0]+'=0'+eol;break
            lines.append(line)
        if any(value!=1 for value in counts.values()):raise ValueError('Expected exact existing sound controls: '+str(file))
        pending.append((file,raw,''.join(lines).encode('utf-8')))
    if not pending:raise ValueError('No private settings')
    config=json.loads((selected/'config.json').read_text())
    requested={Path(v).resolve() for v in config['game_settings_paths'].values()}
    if not requested.issubset({p.resolve() for p,_,_ in pending}):raise ValueError('Selected settings were not prepared')
    backup=selected/'sound-preparation';backup.mkdir(exist_ok=False)
    receipt=[]
    for file,raw,updated in pending:
        if file.read_bytes()!=raw:raise RuntimeError('Private settings changed before launch')
        (backup/(file.name+'.raw')).write_bytes(raw);file.write_bytes(updated)
        if file.read_bytes()!=updated:raise RuntimeError('Private sound settings write failed')
        receipt.append(dict(path=str(file),before_sha256=hashlib.sha256(raw).hexdigest(),
                            after_sha256=hashlib.sha256(updated).hexdigest(),configured_values={k:0 for k in KEYS}))
    result=dict(files=receipt,user_settings_changed=False,system_volume_changed=False,
                engine_audio_disabled=False,runtime_gain_verification_pending=True)
    (backup/'receipt.json').write_text(json.dumps(result,indent=2),encoding='utf-8')
    return result
