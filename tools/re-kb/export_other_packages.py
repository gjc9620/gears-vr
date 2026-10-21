"""Inventory remaining installed .u packages; export readable script objects offline."""
import csv
import hashlib
import json
from pathlib import Path
import re
import subprocess
from export_scripts import ROOT, sha


def main():
    cooked=ROOT/'Gears.of.War.Reloaded.v1.0.1.3816947-P2P/Gears of War Reloaded/WarGame/CookedPC'
    output=ROOT/'analysis/re-kb/other-packages-v2'
    output.mkdir(exist_ok=True)
    runtime=Path('C:/Users/11/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe')
    pattern="^(?:Function|ScriptStruct|Enum|Const)'"
    result={'authority':'Offline installed .u package inventory and UELib script export',
            'limitations':['Asset packages outside *.u are not decompiled.',
                'Native declarations and unresolved UELib tokens are not native implementations.',
                'Unsupported/decompression failures remain explicit; no game execution.'],
            'packages':[],'failures':[]}
    for packed in sorted(cooked.glob('*.u')):
        if packed.stem in ('Engine','WarfareGame'):
            continue
        folder=output/packed.stem; folder.mkdir(exist_ok=True)
        receipt=folder/'receipt.json'; packed_sha=sha(packed)
        try:
            if receipt.exists():
                old=json.loads(receipt.read_text())
                if old['packed_sha256']!=packed_sha or old['pattern']!=pattern:
                    raise ValueError('existing export profile changed')
                source=Path(old['source'])
                if sha(source)!=old['source_sha256']:
                    raise ValueError('analysis source changed')
            else:
                source=packed
                with (folder/'export.log').open('w',encoding='utf-8') as log:
                    args=[str(ROOT/'tools/package-inspector/Inspect.exe'),str(source),str(folder),pattern]
                    code=subprocess.run(args,stdout=log,stderr=subprocess.STDOUT,timeout=120).returncode
                    if code==3:
                        source=folder/packed.name
                        dec=subprocess.run([str(runtime),str(ROOT/'tools/package-inspector/decompress.py'),str(packed),str(source)],
                            stdout=log,stderr=subprocess.STDOUT,timeout=120)
                        if dec.returncode:
                            raise RuntimeError('unsupported compressed package; inspect export.log')
                        args[1]=str(source)
                        code=subprocess.run(args,stdout=log,stderr=subprocess.STDOUT,timeout=120).returncode
                    if code:
                        raise RuntimeError('Inspect returned '+str(code))
                receipt.write_text(json.dumps({'packed_sha256':packed_sha,'pattern':pattern,
                    'source':str(source),'source_sha256':sha(source)}),encoding='utf-8')
            entries=[]; objects=0
            with (folder/'objects.tsv').open(encoding='utf-8-sig',newline='') as stream:
                for row in csv.DictReader(stream,delimiter='\t'):
                    objects+=1
                    if row['Class'] not in ('Function','ScriptStruct','Enum','Const'):
                        continue
                    path=folder/(re.sub(r'[^a-zA-Z0-9_.-]','_',row['Path'])+'.uc')
                    item={'index':int(row['Index']),'path':row['Path'],'class':row['Class'],
                          'file':str(path),'exported':path.is_file()}
                    if path.is_file():
                        data=path.read_bytes(); text=data.decode('utf-8-sig')
                        item.update(sha256=hashlib.sha256(data).hexdigest(),bytes=len(data),
                            native_declaration=bool(re.search(r'^\s*(?!//)[^\n]*\bnative\b[^\n]*\bfunction\b',text,re.M)),
                            has_unresolved_native_tokens=bool(re.search(r'__NFUN_\d+__',text)),
                            has_unresolved_type_marker='@NULL' in text)
                    entries.append(item)
            result['packages'].append({'name':packed.stem,'source':str(source),'source_sha256':sha(source),
                'packed_source':str(packed),'packed_source_sha256':packed_sha,'objects':objects,
                'entries':entries,'errors_file':str(folder/'errors.txt')})
            print(json.dumps({'package':packed.stem,'objects':objects,'selected':len(entries),
                'exported':sum(e['exported'] for e in entries)}),flush=True)
        except (OSError,ValueError,RuntimeError,subprocess.TimeoutExpired) as exc:
            result['failures'].append({'package':packed.stem,'packed_sha256':packed_sha,'error':str(exc)})
            print(json.dumps(result['failures'][-1]),flush=True)
        (output/'manifest.json').write_text(json.dumps(result,ensure_ascii=False,indent=2),encoding='utf-8')


if __name__=='__main__':
    main()
