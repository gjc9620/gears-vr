from pathlib import Path
import hashlib,json
H=Path(__file__).resolve().parent
snapshot=H/'rea-snapshot.json';data=json.loads(snapshot.read_text())
records=data['evidence_bundle']['records']
wanted={'0x180172160','0x1801275a0','0x180188440','0x1800a9f10','0x1801ab130','0x18022cef0'}
selected=[r for r in records if any(str(v).lower() in wanted for v in r.get('parameters',{}).values())]
assert len(selected)>=7,len(selected)
(H/'selected-evidence.json').write_text(json.dumps(selected,indent=2)+'\n',encoding='utf8')
record=dict(binary_sha256='22051aa75312344a824a5f7e29c6375fec3a90df50cbeff6152f0e6c3afddd48',
    snapshot_sha256=hashlib.sha256(snapshot.read_bytes()).hexdigest(),
    evidence_ids=[r['evidence_id'] for r in selected],provider_closed=True,game_executed=False,
    observations=[
        'Public Scene vtable 0x23ca98 +0x2f0 ->0x172160. Internal Scene vtable 0x22ccb8 +0x248 ->0xa9f10.',
        'Public 0x172160 body has 201 bytes through RET 0x172228. Internal EAX is passed as tenth argument to 0x1275a0; that bridge tail-jumps to 0x188440 and public returns final EAX.',
        '0xa9f10 obtains candidate list through 0x1ab130; argument9 selects exact virtual +0x120 test versus candidates-only path. Returned entries are public shapes from internal shape virtual +0x40, eight bytes each.',
        'Arguments3/4 select object types and caller capacity; argument5 supplies shape-pointer buffer, argument6 synchronous callback object, argument7 groups, argument8 mask, argument9 accurate predicate.',
        'Callback vtable[0](reporter,uint32_count,shape_pointer_array) can be invoked repeatedly, returns bool continuation. Internal default batch64 when capacity0; caller buffer avoids original allocation path.',
        '0x188440 walks fallback scene actors and calls their internal +0x248. It can advance caller buffer by prior returned count and reduce available capacity; callback data need not equal original base.',
        'If remaining capacity is exhausted fallback may pass null buffer/capacity0, invoking its own original buffer allocation. Raw callback count, early stop, truncation and buffer provenance need explicit handling.'
    ],inferences=[
        'This is the capsule shape-enumeration candidate, inferred from same broadphase and shape method as the established capsule-any route. It can support self-aware endpoint observation after its exact ABI and transitive consumers are certified.',
        'Existing 64-byte sweep Hit collector must not be reused: this route emits 8-byte pointers and fallback callbacks may use subspans or internal buffers.'
    ],unknowns=[
        'No original enumerating call executed; no current C0 integration or ownership closure on these results yet.',
        'Complete collision type/scale/bone coverage, internal narrowphase correctness and world/transform fence remain unproven.',
        'Exact batch budget and all fallback scene count semantics need full assembly review and callback tests before production invocation.',
        'No endpoint occupancy certificate or movement permission issued; zero-axis Capsule and Sphere sweep remain prohibited.'
    ])
(H/'findings.json').write_text(json.dumps(record,indent=2)+'\n',encoding='utf8')
print(json.dumps(dict(selected_records=len(selected),provider_closed=True,movement_authorized=False)))
