"""Export available class declarations/fields from the inventoried script packages."""
import csv
import json
from pathlib import Path
import re
import subprocess
from export_scripts import ROOT, sha


def main():
    output=ROOT/'analysis/re-kb/classes-v2'; output.mkdir(exist_ok=True)
    packages=[]
    for name in ('scripts','other-packages-v2'):
        packages.extend(json.loads((ROOT/f'analysis/re-kb/{name}/manifest.json').read_text(encoding='utf-8'))['packages'])
    report={'authority':'Offline UELib class exports from hash-verified script packages',
            'limitations':['Class text may contain unresolved types/tokens and embedded function text.',
                'Script fields are declarations, not verified native memory layouts.',
                'Generated files are not original source or proof of execution.'], 'packages':[]}
    for package in packages:
        source=Path(package['source'])
        if sha(source)!=package['source_sha256']:
            raise ValueError('package identity mismatch')
        folder=output/package['name']; folder.mkdir(exist_ok=True)
        receipt=folder/'completed.sha256'
        if receipt.exists():
            if receipt.read_text()!=package['source_sha256']:
                raise ValueError('class export source changed')
        else:
            with (folder/'export.log').open('w') as log:
                p=subprocess.run([str(ROOT/'tools/package-inspector/Inspect.exe'),str(source),str(folder),"^Class'"],
                    stdout=log,stderr=subprocess.STDOUT,timeout=180)
                if p.returncode:
                    raise RuntimeError('class export failed: '+package['name'])
            receipt.write_text(package['source_sha256'])
        entries=[]
        with (folder/'objects.tsv').open(encoding='utf-8-sig',newline='') as stream:
            for row in csv.DictReader(stream,delimiter='\t'):
                if row['Class']!='Class': continue
                path=folder/(re.sub(r'[^a-zA-Z0-9_.-]','_',row['Path'])+'.uc')
                e={'index':int(row['Index']),'path':row['Path'],'class':'Class','file':str(path),
                   'exported':path.is_file(),'native_declaration':False}
                if path.is_file():
                    e['sha256']=sha(path)
                    text=path.read_text(encoding='utf-8-sig')
                    e['has_unresolved_native_tokens']=bool(re.search(r'__NFUN_\d+__',text))
                    e['has_unresolved_type_marker']='@NULL' in text
                entries.append(e)
        report['packages'].append({'name':package['name'],'source':str(source),
            'source_sha256':package['source_sha256'],'entries':entries,'errors_file':str(folder/'errors.txt')})
        print(json.dumps({'package':package['name'],'classes':len(entries),'exported':sum(e['exported'] for e in entries)}),flush=True)
        (output/'manifest.json').write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf-8')


if __name__=='__main__': main()
