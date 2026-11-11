from pathlib import Path
import json,hashlib
root=Path.cwd();folder=root/'analysis/cover-system-20261108/shape-parameters-r215'
bundle=json.loads((folder/'rea-evidence.json').read_text());records=bundle['records'];sha='22051aa75312344a824a5f7e29c6375fec3a90df50cbeff6152f0e6c3afddd48'
assert all(r['subject']['digest']['sha256']==sha for r in records)
addresses=[0xc410,0x12770,0x12760,0xc460,0xb1350,0xb1370,0x13e780,0x13fc90,0x13fc80,0x13e7a0]
rows=[];output=['#pragma once','#include "cover_physx_query_source.h"','namespace gears_cover_physx_shape_parameter_code {','template<class Read> bool current(std::uintptr_t image_base,Read& read){']
for rva in addresses:
 address=hex(0x180000000+rva);function=next(r for r in records if r['operation']=='analyze_function' and r['normalized_result']['procedure']['address']==address)
 body=function['normalized_result']['procedure']['body'];assert body['available'] and not body['non_contiguous'] and len(body['ranges'])==1
 record=next(r for r in records if r['operation']=='read_bytes' and r['parameters']['address']==address and r['parameters']['length']==body['total_bytes'])
 value=record['normalized_result'];data=bytes.fromhex(value['bytes_hex']);assert value['complete'] and len(data)==body['total_bytes']
 rows.append(dict(address=address,rva=hex(rva),size=len(data),bytes=data.hex(),sha256=hashlib.sha256(data).hexdigest(),function_evidence=function['evidence_id'],bytes_evidence=record['evidence_id']))
 output.append('    { constexpr std::array<unsigned char,%d> expected{%s};std::array<unsigned char,%d> actual{};if(!gears_cover_physx_query_source::pose::bytes(read,actual.data(),image_base+0x%x,actual.size())||actual!=expected)return false;}'%(len(data),','.join(hex(v) for v in data),len(data),rva))
output+=['    return true;','}','}']
(root/'tools/native-capture/cover_physx_shape_parameter_code.h').write_text('\n'.join(output)+'\n',encoding='utf-8')
(folder/'getter-certificate.json').write_text(json.dumps(dict(artifact_sha256=sha,bundle_sha256=hashlib.sha256((folder/'rea-evidence.json').read_bytes()).hexdigest(),records=len(records),getters=rows,
 scope='Exact original direct field getters/public forwarding stubs only. Stored position/quaternion and flags are observations, not a world pose or eligibility decision. Capsule12780 global-capsule getter may write cached pose and is never called by this reader.'),indent=2)+'\n',encoding='utf-8')
print(json.dumps({'getters':len(rows),'bytes':sum(r['size'] for r in rows),'records':len(records)}))
