"""Expand the existing Engine/WarfareGame offline script corpus; reuse UI exports."""
import csv
import hashlib
import json
from pathlib import Path
import re
import subprocess

ROOT=Path('J:/gearsVr')
OUT=ROOT/'analysis/re-kb/scripts'


def sha(path):
    with path.open('rb') as stream:
        return hashlib.file_digest(stream,'sha256').hexdigest()


def main():
    OUT.mkdir(parents=True,exist_ok=True)
    previous=json.loads((ROOT/'analysis/ui-v1-script-audit/manifest.json').read_text(encoding='utf-8'))
    pattern="(?!(?:"+previous['selection_regex']+"))^(?:Function|ScriptStruct|Enum|Const)'"
    report={'authority':'Offline UELib exports from hash-verified decompressed package copies',
            'selection_regex':pattern,'limitations':previous['limitations']+[ 
                'Expanded beyond UI; currently only Engine and WarfareGame packages.',
                'Objects with errors remain explicit; native declarations are not native implementations.'],
            'packages':[]}
    for old in previous['packages']:
        source=Path(old['source']); source_hash=sha(source)
        if source_hash!=old['source_sha256']:
            raise ValueError('package source changed: '+str(source))
        folder=OUT/old['name']; folder.mkdir(exist_ok=True)
        receipt=folder/'completed.json'
        if receipt.exists():
            if json.loads(receipt.read_text())!={'sha256':source_hash,'pattern':pattern}:
                raise ValueError('existing script export profile differs')
        else:
            with (folder/'export.log').open('w',encoding='utf-8') as log:
                result=subprocess.run([str(ROOT/'tools/package-inspector/Inspect.exe'),str(source),str(folder),pattern],
                    stdout=log,stderr=subprocess.STDOUT,timeout=240)
            if result.returncode:
                raise RuntimeError('Inspect failed; see '+str(folder/'export.log'))
            receipt.write_text(json.dumps({'sha256':source_hash,'pattern':pattern}),encoding='utf-8')
        reused={e['path']:e for e in old['entries']}
        entries=[]
        with (folder/'objects.tsv').open(encoding='utf-8-sig',newline='') as stream:
            for row in csv.DictReader(stream,delimiter='\t'):
                if row['Class'] not in ('Function','ScriptStruct','Enum','Const'):
                    continue
                prior=reused.get(row['Path'])
                path=Path(prior['file']) if prior else folder/(re.sub(r'[^a-zA-Z0-9_.-]','_',row['Path'])+'.uc')
                entry={'index':int(row['Index']),'path':row['Path'],'class':row['Class'],
                       'file':str(path),'exported':path.is_file(),'reused_ui_export':bool(prior)}
                if path.is_file():
                    data=path.read_bytes(); digest=hashlib.sha256(data).hexdigest()
                    if prior and digest!=prior['sha256']:
                        raise ValueError('existing UI export changed: '+str(path))
                    text=data.decode('utf-8-sig')
                    entry.update(sha256=digest,bytes=len(data),native_declaration=bool(re.search(
                        r'^\s*(?!//)[^\n]*\bnative\b[^\n]*\bfunction\b',text,re.M)),
                        has_unresolved_native_tokens=bool(re.search(r'__NFUN_\d+__',text)),
                        has_unresolved_type_marker='@NULL' in text)
                entries.append(entry)
        package={'name':old['name'],'source':str(source),'source_sha256':source_hash,
                 'entries':entries,'selected_objects':len(entries),
                 'exported_objects':sum(e['exported'] for e in entries),
                 'reused_ui_exports':sum(e['reused_ui_export'] for e in entries),
                 'errors_file':str(folder/'errors.txt')}
        report['packages'].append(package)
        print(json.dumps({k:v for k,v in package.items() if k not in ('entries','source','source_sha256')}),flush=True)
    (OUT/'manifest.json').write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf-8')


if __name__=='__main__':
    main()
