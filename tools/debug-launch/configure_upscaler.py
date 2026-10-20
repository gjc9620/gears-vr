"""Reversible DLSS-only diagnostic setting; no changes while the game is running."""
import argparse
import hashlib
import json
import pathlib
import re
import subprocess

ROOT=pathlib.Path(__file__).resolve().parents[2]
STATE=ROOT/'analysis/upscaler-config'


def locate():
    folder=pathlib.Path.home()/'AppData/Local/Coalition/WarGame/Config'
    files=list(folder.glob('*EngineUserSettings.ini'))
    if len(files)!=1:raise RuntimeError('Expected exactly one user-settings file')
    return files[0].resolve()


def decode(raw):
    if raw.startswith(b'\xff\xfe'):return raw.decode('utf-16'), 'utf-16'
    return raw.decode('utf-8-sig'), 'utf-8-sig' if raw.startswith(b'\xef\xbb\xbf') else 'utf-8'


def value(text,key):
    matches=re.findall(r'^'+re.escape(key)+r'=(.*?)(?:\r?$)',text,re.M|re.I)
    if len(matches)!=1:raise RuntimeError(f'Expected one {key} setting')
    return matches[0]


def replace(text,old,new):
    if value(text,'DLSS').lower()!=old.lower():raise RuntimeError('DLSS changed independently; refusing overwrite')
    result,count=re.subn(r'^(DLSS=)[^\r\n]*',lambda m:m[1]+new,text,flags=re.M|re.I)
    if count!=1:raise RuntimeError('Unexpected edit count')
    return result


def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('action',choices=('status','disable','restore'))
    a=p.parse_args();target=locate();raw=target.read_bytes();text,encoding=decode(raw)
    if a.action=='status':
        print(json.dumps({'path':str(target),'dlss':value(text,'DLSS'),'quality':value(text,'AADefaultQualityMode'),
            'fsr2':value(text,'FSR2'),'fsr3':value(text,'FSR3'),'mfsr':value(text,'MFSR')},indent=2));return
    running=subprocess.run(['powershell','-NoProfile','-NonInteractive','-Command',
        "if(Get-Process -Name 'GOWDE-Steam' -ErrorAction SilentlyContinue){exit 1}else{exit 0}"],capture_output=True)
    if running.returncode:raise RuntimeError('Exit the game normally before changing settings')
    manifest=STATE/'active.json'
    if a.action=='disable':
        if manifest.exists():raise RuntimeError('An existing diagnostic edit must be restored first')
        if any(value(text,key).lower()!='false' for key in ('FSR2','FSR3','MFSR')):
            raise RuntimeError('This diagnostic requires other upscalers already off')
        patched=replace(text,'true','False').encode(encoding)
        STATE.mkdir(parents=True,exist_ok=True)
        backup=STATE/(hashlib.sha256(raw).hexdigest()+'.ini');backup.write_bytes(raw)
        report={'path':str(target),'backup':str(backup),'original_sha256':hashlib.sha256(raw).hexdigest(),
            'diagnostic_sha256':hashlib.sha256(patched).hexdigest(),'original_value':value(text,'DLSS')}
        manifest.write_text(json.dumps(report,indent=2)+'\n')
        target.write_bytes(patched)
        if target.read_bytes()!=patched:raise RuntimeError('Setting write verification failed')
        print(json.dumps({'dlss':'False','backup':str(backup),'changed_keys':['DLSS']}))
    else:
        report=json.loads(manifest.read_text())
        if pathlib.Path(report['path'])!=target:raise RuntimeError('Backup belongs to a different settings file')
        saved=pathlib.Path(report['backup']).read_bytes()
        if hashlib.sha256(saved).hexdigest()!=report['original_sha256']:raise RuntimeError('Backup hash differs')
        restored=replace(text,'false',report['original_value']).encode(encoding)
        target.write_bytes(restored)
        if target.read_bytes()!=restored:raise RuntimeError('Restore verification failed')
        manifest.replace(STATE/(report['original_sha256']+'.restored.json'))
        print(json.dumps({'dlss':report['original_value'],'restored':True,
            'exact_original_bytes':restored==saved,'other_current_settings_preserved':True}))


if __name__=='__main__':main()
