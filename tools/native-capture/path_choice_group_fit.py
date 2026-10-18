"""Pure first-baseline inputs for the native shared-fit SafePanel Y candidate.

No process writes and no Python replacement for the native shared fit policy.
Membership is the existing complete13-node parent graph, not a name heuristic.
"""
import math
import struct


def inputs(sample, panel, target, require):
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
    # docs155/Native88 established the actual UI viewport domain separately.
    # RenderBounds describes the inset panel, not the full viewport. Validate
    # ALL percentage-viewport faces against that domain; never infer a screen
    # size by adding arbitrary offset coordinates (nor by the panel width).
    width, height = 1920., 1080.
    members=panel['layout_details']['position']['members'];fraction=members['Value']['values']
    require(len(fraction)==4 and struct.pack('<4f',*fraction).hex()==members['Value']['raw'] and
            members['ScaleType']['raw']=='04040404' and all(math.isfinite(x) for x in fraction) and
            0<=fraction[0]<fraction[2]<=1 and 0<=fraction[1]<fraction[3]<=1 and
            all(abs(p[i]-fraction[i]*(width if i%2==0 else height))<=.001 for i in range(4)),
            'panel faces disagree with certified UI coordinate domain')
    values, reads = [], [panel_read]
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
    return fields, reads
