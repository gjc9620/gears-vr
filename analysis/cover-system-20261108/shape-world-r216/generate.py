from pathlib import Path
import json,hashlib
root=Path.cwd();folder=root/'analysis/cover-system-20261108/shape-world-r216'
bundle=json.loads((folder/'rea-evidence.json').read_text());old=json.loads((folder.parent/'shape-parameters-r215/rea-evidence.json').read_text());records=bundle['records']+old['records']
sha='22051aa75312344a824a5f7e29c6375fec3a90df50cbeff6152f0e6c3afddd48';assert all(r['subject']['digest']['sha256']==sha for r in records)
addresses=[0x4e50,0x4e70,0x1209f0,0x133fe0,0x178ff0,0x13e7e0];rows=[]
output=['#pragma once','#include "cover_physx_query_source.h"','namespace gears_cover_physx_shape_world_code {','template<class Read> bool current(std::uintptr_t image_base,Read& read){']
for rva in addresses:
 address=hex(0x180000000+rva);function=next(r for r in records if r['operation']=='analyze_function' and r['normalized_result']['procedure']['address']==address)
 body=function['normalized_result']['procedure']['body'];assert body['available'] and not body['non_contiguous'] and len(body['ranges'])==1
 record=next(r for r in records if r['operation']=='read_bytes' and r['parameters']['address']==address and r['parameters']['length']==body['total_bytes'])
 data=bytes.fromhex(record['normalized_result']['bytes_hex']);assert record['normalized_result']['complete'] and len(data)==body['total_bytes']
 rows.append(dict(address=address,size=len(data),sha256=hashlib.sha256(data).hexdigest(),function_evidence=function['evidence_id'],bytes_evidence=record['evidence_id']))
 output.append('    { constexpr std::array<unsigned char,%d> expected{%s};std::array<unsigned char,%d> actual{};if(!gears_cover_physx_query_source::pose::bytes(read,actual.data(),image_base+0x%x,actual.size())||actual!=expected)return false;}'%(len(data),','.join(hex(v) for v in data),len(data),rva))
record=next(r for r in bundle['records'] if r['operation']=='read_bytes' and r['parameters']['address']=='0x1802266c0');assert record['normalized_result']['bytes_hex']=='0000803f'
output+=['    { std::array<unsigned char,4> actual{};constexpr std::array<unsigned char,4> one{{0,0,128,63}};if(!gears_cover_physx_query_source::pose::bytes(read,actual.data(),image_base+0x2266c0,4)||actual!=one)return false;}','    return true;','}','}']
(root/'tools/native-capture/cover_physx_shape_world_code.h').write_text('\n'.join(output)+'\n',encoding='utf-8')
(folder/'getter-certificate.json').write_text(json.dumps(dict(artifact_sha256=sha,records=len(bundle['records']),code=rows,one=record['evidence_id'],
 parent_bundle_sha256=hashlib.sha256((folder.parent/'shape-parameters-r215/rea-evidence.json').read_bytes()).hexdigest(),
 bundle_sha256=hashlib.sha256((folder/'rea-evidence.json').read_bytes()).hexdigest()),indent=2)+'\n',encoding='utf-8')
print(json.dumps({'functions':len(rows),'bytes':sum(r['size'] for r in rows),'one_bytes':4}))
