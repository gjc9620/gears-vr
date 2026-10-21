"""Retry only failed classes after loading their constant/enum child objects."""
import json
from pathlib import Path
import re
import subprocess
from export_scripts import ROOT,sha


def main():
    original=json.loads((ROOT/'analysis/re-kb/classes-v2/manifest.json').read_text(encoding='utf-8'))
    output=ROOT/'analysis/re-kb/class-recovery';output.mkdir(exist_ok=True)
    tool=ROOT/'tools/package-inspector/RecoverClasses.exe'
    report={'authority':'UELib class recovery with constants/enums deserialized before class formatting',
        'limitations':original['limitations'],'packages':[]}
    for package in original['packages']:
        missing=[e for e in package['entries'] if not e['exported']]
        if not missing:continue
        source=Path(package['source'])
        if sha(source)!=package['source_sha256']:raise ValueError('source hash changed')
        folder=output/package['name'];folder.mkdir(exist_ok=True)
        selection=folder/'selection.txt';selection.write_text('\n'.join(e['path'] for e in missing),encoding='utf-8-sig')
        profile={'source_sha256':package['source_sha256'],'helper_sha256':sha(tool),'selection_sha256':sha(selection)}
        receipt=folder/'completed.json'
        if receipt.exists():
            if json.loads(receipt.read_text())!=profile:raise ValueError('recovery profile changed')
        else:
            with (folder/'export.log').open('w') as log:
                result=subprocess.run([str(tool),str(source),str(folder),str(selection)],
                    stdout=log,stderr=subprocess.STDOUT,timeout=120)
            if result.returncode:raise RuntimeError('recovery helper failed; inspect log')
            receipt.write_text(json.dumps(profile))
        entries=[]
        for previous in missing:
            path=folder/(re.sub(r'[^a-zA-Z0-9_.-]','_',previous['path'])+'.uc')
            e={**previous,'file':str(path),'exported':path.is_file(),'recovery_profile':str(receipt),
                'previous_failure_log':package['errors_file']}
            if path.is_file():
                text=path.read_text(encoding='utf-8-sig')
                e.update(sha256=sha(path),has_unresolved_native_tokens=bool(re.search(r'__NFUN_\d+__',text)),
                    has_unresolved_type_marker='@NULL' in text)
            entries.append(e)
        report['packages'].append({'name':package['name'],'source':str(source),
            'source_sha256':package['source_sha256'],'entries':entries,'errors_file':str(folder/'errors.txt')})
        print(json.dumps({'package':package['name'],'attempted':len(entries),'recovered':sum(e['exported'] for e in entries)}))
    (output/'manifest.json').write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf-8')


if __name__=='__main__':main()
