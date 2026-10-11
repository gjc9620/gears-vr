"""Compare read-only export inventories and separate likely offset fixups.

Relocation classification is a byte-level heuristic, not an asset-format decoder.
"""
import argparse
import collections
import csv
import json
import pathlib
import struct


def inventory(path):
    with path.open(encoding='utf-8-sig',newline='') as stream:
        return {r['Path']:r for r in csv.DictReader(stream,delimiter='\t')}


def offset_fixups(old,new,a,b):
    delta=int(new['Offset'])-int(old['Offset'])
    if len(a)!=len(b) or not delta:return False
    differences={i for i,(x,y) in enumerate(zip(a,b)) if x!=y}
    if not differences:return False
    covered=set()
    for changed in differences:
        if changed in covered:continue
        for pos in range(max(0,changed-3),min(changed+1,len(a)-3)):
            before=struct.unpack_from('<I',a,pos)[0]
            after=struct.unpack_from('<I',b,pos)[0]
            if int(old['Offset'])<=before<=int(old['Offset'])+len(a) and after-before==delta:
                covered.update(range(pos,pos+4))
    return differences<=covered


def compare(root,package):
    original=inventory(root/('original-'+package+'.tsv'))
    modified=inventory(root/('mod-'+package+'.tsv'))
    result={'original_count':len(original),'mod_count':len(modified),
        'added':sorted(modified.keys()-original.keys()),'removed':sorted(original.keys()-modified.keys()),
        'changed':[], 'unchanged_class_exports':0,'unchanged_function_exports':0}
    with (root.parent/'packages/decompressed'/(package+'.u')).open('rb') as a, (root/'decompressed'/(package+'.u')).open('rb') as b:
        for path in sorted(original.keys()&modified.keys()):
            old,new=original[path],modified[path]
            if old['SHA256']==new['SHA256'] and old['ObjectFlags']==new['ObjectFlags']:
                if new['Class']=='Class':result['unchanged_class_exports']+=1
                if new['Class']=='Function':result['unchanged_function_exports']+=1
                continue
            record={'path':path,'class':new['Class'],'original':old,'mod':new,'consistent_with_offset_fixups':False}
            if new['Class'] in ('Texture2D','SoundNodeWave','StaticMesh','SkeletalMesh','TextureCube'):
                a.seek(int(old['Offset']));b.seek(int(new['Offset']))
                record['consistent_with_offset_fixups']=offset_fixups(old,new,a.read(int(old['Size'])),b.read(int(new['Size'])))
            result['changed'].append(record)
    return result


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--root',type=pathlib.Path,default=pathlib.Path('analysis/first-person'))
    parser.add_argument('--output',type=pathlib.Path,required=True)
    args=parser.parse_args()
    result={pkg:compare(args.root,pkg) for pkg in ('Engine','WarfareGame')}
    args.output.write_text(json.dumps(result,indent=2),encoding='utf-8')
    for pkg,r in result.items():
        print(json.dumps({'package':pkg,'exports':r['original_count'],'changed':len(r['changed']),
            'unchanged_classes':r['unchanged_class_exports'],'unchanged_functions':r['unchanged_function_exports'],
            'offset_fixup_candidates':sum(x['consistent_with_offset_fixups'] for x in r['changed']),
            'other_changed_classes':dict(collections.Counter(x['class'] for x in r['changed'] if not x['consistent_with_offset_fixups']))}))


if __name__=='__main__':main()
