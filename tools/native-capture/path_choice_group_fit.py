"""Pure first-baseline inputs for the native shared-fit SafePanel Y candidate.

No process writes and no Python replacement for the native shared fit policy.
Membership is the existing complete13-node parent graph, not a name heuristic.
"""
import math
import struct


def viewport_domain(sample, require):
    """Validate the collector's registered/reflected storage and raw coverage."""
    v=sample['viewport_domain'];prop=v['property'];field=prop['identity'];structure=v['struct_type']
    require(v['schema']==1 and v['kind']=='reflected_uiscene_viewport_domain' and
            v['read_only'] is True and v['consistent_reread'] is True and
            v['scene']==sample['scene_owner']['roles']['scene'] and
            v['scene']['address']==int(sample['top']['address'],0),'unverified viewport scene/domain')
    require(prop['type']=='StructProperty' and field['name']=='CurrentViewportSize' and field['number']==0 and
            prop['owner']['name']=='UIScene' and prop['owner']['number']==0 and
            field['outer']==prop['owner']['address'] and prop['array_dim']==1 and
            type(prop['element_size']) is int and 8<=prop['element_size']<=128 and
            type(prop['offset']) is int and 0<=prop['offset']<=0x10000-prop['element_size'],
            'unsupported viewport property proof')
    owners=[c['identity'] for c in sample['scene_owner']['classes'] if c['identity']['name']=='UIScene']
    require(len(owners)==1 and prop['owner']==owners[0],'viewport declaring class differs from owner snapshot')
    checks=v['source_checks'];require(type(checks) is list and 0<len(checks)<=2048,'viewport check budget')
    cells={};reads=[]
    for check in checks:
        address=check['address'];data=bytes.fromhex(check['raw'])
        require(type(address) is int and 0<address<=(1<<64)-len(data) and 0<len(data)<=1024,'invalid viewport raw span')
        for i,byte in enumerate(data):
            require(address+i not in cells or cells[address+i]==byte,'conflicting viewport raw spans')
            cells[address+i]=byte
        require(len(cells)<=32768,'viewport byte budget')
        reads.append(dict(address=address,raw=data))
    def raw_check(address,expected):
        require(all(cells.get(address+i)==byte for i,byte in enumerate(expected)),
                'missing or stale viewport metadata/value coverage')
    from path_choice_trial_config import LAYOUT
    raw_check(field['address']+0x70,struct.pack('<II',1,prop['element_size']))
    raw_check(field['address']+LAYOUT['property_offset_internal'],struct.pack('<I',prop['offset']))
    raw_check(field['address']+LAYOUT['struct_property_struct'],struct.pack('<Q',structure['address']))
    offsets=[]
    for name in ('X','Y'):
        member=v['members'][name];identity=member['identity'];at=member['offset']
        require(member['type']=='FloatProperty' and identity['name']==name and identity['number']==0 and
                identity['outer']==structure['address'] and member['array_dim']==1 and member['element_size']==4 and
                type(at) is int and 0<=at<=prop['element_size']-4,'unsupported viewport member proof')
        raw_check(identity['address']+0x70,struct.pack('<II',1,4))
        raw_check(identity['address']+LAYOUT['property_offset_internal'],struct.pack('<I',at));offsets.append(at)
    require(abs(offsets[0]-offsets[1])>=4,'overlapping viewport member proof')
    raw=bytes.fromhex(v['raw']);address=v['scene']['address']+prop['offset']
    require(v['value_address']==address and len(raw)==prop['element_size'],'invalid viewport value address/size')
    values=[struct.unpack_from('<f',raw,at)[0] for at in offsets]
    require(values==v['values_xy'] and all(math.isfinite(x) and x>0 for x in values),'invalid reflected viewport values')
    raw_check(address,raw)
    return values,reads


def inputs(sample, panel, target, require, *, immutable_reads=None):
    def f32(x): return struct.unpack('<f', struct.pack('<f', x))[0]
    def bounds(row):
        b = row['render_bounds']; raw = bytes.fromhex(b['raw']); values = b['values']
        require(b['order'] == ['left','top','right','bottom'] and len(raw) == 16 and
                len(values) == 4 and all(type(x) in (float,int) and math.isfinite(x) for x in values) and
                struct.pack('<4f', *values) == raw, 'invalid copied content RenderBounds')
        p = b['reflected']; require(p['type'] == 'FloatProperty' and p['array_dim'] == 4 and
            p['element_size'] == 4 and type(p['offset']) is int and 0 <= p['offset'] <= 0xfff0,
            'unsupported content RenderBounds field')
        ordered = [min(values[0],values[2]),min(values[1],values[3]),max(values[0],values[2]),max(values[1],values[3])]
        require(b['ordered_values'] == ordered and ordered[2] > ordered[0] and ordered[3] > ordered[1],
                'invalid ordered content bounds')
        return ordered, dict(address=int(row['address'],0)+p['offset'], raw=raw)
    tree = sample['tree']; root = sample['top']['address']; parent = panel['address']
    require(len(tree) == 13 and len({r['address'] for r in tree}) == 13, 'incomplete content inventory')
    children = [r for r in tree if r.get('parent') == parent]
    direct = [r for r in tree if r.get('parent') == root]
    require(len(children) == 10 and len(direct) == 2 and panel in direct and
            all(r['outer'] == parent and r['depth'] == 3 for r in children) and
            {r['address'] for r in tree} == {root, *[r['address'] for r in direct], *[r['address'] for r in children]},
            'unsupported content parent graph')
    # The other direct scene child is excluded because it is outside the panel
    # subtree. Its name, texture, dimensions and display text are never a filter.
    p, panel_read = bounds(panel)
    # Independently reflected UIScene storage supplies the domain. RenderBounds
    # describes the inset panel; all four percentage faces must agree, but none
    # is used to infer a screen/eye texture/window size.
    (width,height),domain_reads=viewport_domain(sample,require)
    members=panel['layout_details']['position']['members'];fraction=members['Value']['values']
    require(len(fraction)==4 and struct.pack('<4f',*fraction).hex()==members['Value']['raw'] and
            members['ScaleType']['raw']=='04040404' and all(math.isfinite(x) for x in fraction) and
            0<=fraction[0]<fraction[2]<=1 and 0<=fraction[1]<fraction[3]<=1 and
            all(abs(p[i]-fraction[i]*(width if i%2==0 else height))<=.001 for i in range(4)),
            'panel faces disagree with certified UI coordinate domain')
    values, reads = [], [panel_read,*domain_reads]
    for row in children:
        b, read = bounds(row); values.append(b); reads.append(read)
    source = [f32(min(b[0] for b in values)/width), f32(min(b[1] for b in values)/height),
              f32(max(b[2] for b in values)/width), f32(max(b[3] for b in values)/height)]
    require(type(target) in (tuple,list) and len(target) == 2 and
            all(type(x) in (float,int) and math.isfinite(x) for x in target), 'invalid target vertical region')
    top, bottom = map(f32,target)
    require(0 <= source[0] < source[2] <= 1 and 0 <= source[1] < source[3] <= 1 and
            0 <= top < bottom <= 1 and bottom-top >= source[3]-source[1] and
            0 < (top+bottom-source[1]-source[3])/2 <= 400/1080,
            'shared fit needs unsupported scaling, reverse or excessive translation')
    names = ('fit_left_bits','fit_top_bits','fit_right_bits','fit_bottom_bits','fit_target_top_bits','fit_target_bottom_bits')
    fields = dict(group_fit='vertical_v1')
    fields.update(zip(names, struct.unpack('<6I',struct.pack('<6f',*source,top,bottom))))
    if immutable_reads is not None:immutable_reads.extend(domain_reads)
    return fields, reads
