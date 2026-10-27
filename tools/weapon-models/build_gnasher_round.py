"""Build an original, closed Gnasher round prototype from parametric geometry.

The original game resource checked for this role is an *open spent shell*. This
new asset does not copy its vertices or textures. Its provisional envelope
(33.7 mm diameter, 113.5 mm length) is based only on that source's measured
bounds. Final grip, chamber fit, scale and appearance require game/VR review.
"""
import argparse
import json
import math
from pathlib import Path
import struct


ROOT = Path(__file__).resolve().parents[2]
SEGMENTS = 32
MATERIALS = [
    ('Brass base', (0.66, 0.43, 0.18), 0.72, 0.34),
    ('Red hull', (0.49, 0.055, 0.07), 0.02, 0.43),
    ('Folded crimp', (0.38, 0.035, 0.05), 0.02, 0.54),
    ('Primer', (0.22, 0.2, 0.17), 0.5, 0.48),
]


def sub(a, b):
    return tuple(x - y for x, y in zip(a, b))


def cross(a, b):
    return (a[1] * b[2] - a[2] * b[1],
            a[2] * b[0] - a[0] * b[2],
            a[0] * b[1] - a[1] * b[0])


def dot(a, b):
    return sum(x * y for x, y in zip(a, b))


def unit(v):
    norm = math.sqrt(dot(v, v))
    if norm < 1e-10 or not math.isfinite(norm):
        raise ValueError('Degenerate generated geometry')
    return tuple(x / norm for x in v)


def point(radius, angle, height):
    return (radius * math.cos(angle), radius * math.sin(angle), height)


class Primitive:
    def __init__(self):
        self.positions = []
        self.normals = []
        self.indices = []

    def triangle(self, a, b, c):
        normal = unit(cross(sub(b, a), sub(c, a)))
        start = len(self.positions)
        self.positions.extend((a, b, c))
        self.normals.extend((normal, normal, normal))
        self.indices.extend((start, start + 1, start + 2))

    def quad(self, a, b, c, d):
        self.triangle(a, b, c)
        self.triangle(a, c, d)


def lathe(mesh, profile):
    for (radius0, height0), (radius1, height1) in zip(profile, profile[1:]):
        for index in range(SEGMENTS):
            a = 2 * math.pi * index / SEGMENTS
            b = 2 * math.pi * (index + 1) / SEGMENTS
            mesh.quad(point(radius0, a, height0), point(radius0, b, height0),
                      point(radius1, b, height1), point(radius1, a, height1))


def disc(mesh, radius, height, upward):
    center = (0.0, 0.0, height)
    for index in range(SEGMENTS):
        a = point(radius, 2 * math.pi * index / SEGMENTS, height)
        b = point(radius, 2 * math.pi * (index + 1) / SEGMENTS, height)
        mesh.triangle(center, a, b) if upward else mesh.triangle(center, b, a)


def build_geometry():
    meshes = [Primitive() for _ in MATERIALS]
    brass, hull, crimp, primer = meshes
    # Bottom flange, narrower metal cup, and a slight shoulder under the hull.
    lathe(brass, [(0.01685, 0.0), (0.01685, 0.0025),
                  (0.01565, 0.0036), (0.01565, 0.0205)])
    disc(brass, 0.01685, 0.0, upward=False)
    lathe(hull, [(0.01565, 0.0205), (0.01565, 0.1005),
                 (0.01535, 0.1050)])
    # A sealed eight-fold crimp. The changing height makes the folds readable
    # close to the hand without baking a texture or reusing the spent shell.
    center = (0.0, 0.0, 0.1090)
    for index in range(SEGMENTS):
        angle0 = 2 * math.pi * index / SEGMENTS
        angle1 = 2 * math.pi * (index + 1) / SEGMENTS
        ridge0 = 0.00055 * math.cos(8 * angle0)
        ridge1 = 0.00055 * math.cos(8 * angle1)
        outer0 = point(0.01535, angle0, 0.1050)
        outer1 = point(0.01535, angle1, 0.1050)
        fold0 = point(0.0105, angle0, 0.11287 + ridge0)
        fold1 = point(0.0105, angle1, 0.11287 + ridge1)
        inner0 = point(0.0031, angle0, 0.1102 + ridge0 * 0.45)
        inner1 = point(0.0031, angle1, 0.1102 + ridge1 * 0.45)
        crimp.quad(outer0, outer1, fold1, fold0)
        crimp.quad(fold0, fold1, inner1, inner0)
        crimp.triangle(inner0, inner1, center)
    disc(primer, 0.0046, -0.00008, upward=False)
    return meshes


def pad4(raw, fill=b'\0'):
    return raw + fill * (-len(raw) % 4)


def glb_bytes(meshes):
    binary = bytearray()
    views = []
    accessors = []
    primitives = []

    def append_view(raw, target):
        offset = len(binary)
        binary.extend(raw)
        binary.extend(b'\0' * (-len(binary) % 4))
        views.append({'buffer': 0, 'byteOffset': offset,
                      'byteLength': len(raw), 'target': target})
        return len(views) - 1

    for material, mesh in enumerate(meshes):
        if not mesh.positions or len(mesh.positions) != len(mesh.normals):
            raise ValueError('Empty or malformed primitive')
        if max(mesh.indices) >= len(mesh.positions) or len(mesh.positions) >= 65536:
            raise ValueError('Invalid generated indices')
        positions = [value for vertex in mesh.positions for value in vertex]
        normals = [value for vertex in mesh.normals for value in vertex]
        p_view = append_view(struct.pack('<' + 'f' * len(positions), *positions), 34962)
        n_view = append_view(struct.pack('<' + 'f' * len(normals), *normals), 34962)
        i_view = append_view(struct.pack('<' + 'H' * len(mesh.indices), *mesh.indices), 34963)
        mins = [min(vertex[axis] for vertex in mesh.positions) for axis in range(3)]
        maxs = [max(vertex[axis] for vertex in mesh.positions) for axis in range(3)]
        p_accessor = len(accessors)
        accessors.append({'bufferView': p_view, 'componentType': 5126,
                          'count': len(mesh.positions), 'type': 'VEC3',
                          'min': mins, 'max': maxs})
        n_accessor = len(accessors)
        accessors.append({'bufferView': n_view, 'componentType': 5126,
                          'count': len(mesh.normals), 'type': 'VEC3'})
        i_accessor = len(accessors)
        accessors.append({'bufferView': i_view, 'componentType': 5123,
                          'count': len(mesh.indices), 'type': 'SCALAR'})
        primitives.append({'attributes': {'POSITION': p_accessor, 'NORMAL': n_accessor},
                           'indices': i_accessor, 'material': material})
    materials = [
        {'name': name, 'pbrMetallicRoughness': {
            'baseColorFactor': [*color, 1.0], 'metallicFactor': metallic,
            'roughnessFactor': roughness}}
        for name, color, metallic, roughness in MATERIALS]
    gltf = {'asset': {'version': '2.0', 'generator': 'GearsVR original parametric round'},
            'scene': 0, 'scenes': [{'nodes': [0]}],
            'nodes': [{'name': 'Gnasher complete round prototype', 'mesh': 0}],
            'meshes': [{'primitives': primitives}], 'materials': materials,
            'buffers': [{'byteLength': len(binary)}],
            'bufferViews': views, 'accessors': accessors,
            'extras': {'units': 'metres', 'model': 'original procedural prototype',
                       'referenceEnvelopeMm': [33.7, 113.5],
                       'sourceMeshCopied': False, 'chamberFitVerified': False}}
    json_chunk = pad4(json.dumps(gltf, separators=(',', ':'), sort_keys=True).encode('utf-8'), b' ')
    bin_chunk = pad4(bytes(binary))
    total = 28 + len(json_chunk) + len(bin_chunk)
    return (struct.pack('<4sII', b'glTF', 2, total) +
            struct.pack('<I4s', len(json_chunk), b'JSON') + json_chunk +
            struct.pack('<I4s', len(bin_chunk), b'BIN\0') + bin_chunk)


def preview(meshes, destination):
    from PIL import Image, ImageDraw

    if not destination.resolve().is_relative_to((ROOT / 'analysis').resolve()):
        raise ValueError('Preview must stay under ignored analysis/')
    canvas = Image.new('RGB', (1000, 600), (23, 29, 36))
    draw = ImageDraw.Draw(canvas)
    configurations = [((0.9, -1.8, 1.15), (250, 300), 'Oblique view'),
                      ((0.0, -1.0, 0.0), (735, 300), 'Side view')]
    for direction, center, title in configurations:
        eye = unit(direction)
        reference = (0.0, 0.0, 1.0)
        right = unit(cross(reference, eye))
        up = unit(cross(eye, right))
        triangles = []
        for material, mesh in enumerate(meshes):
            color = MATERIALS[material][1]
            for start in range(0, len(mesh.positions), 3):
                verts = mesh.positions[start:start + 3]
                normal = mesh.normals[start]
                if dot(normal, eye) <= 0:
                    continue
                coords = []
                depth = 0.0
                for vertex in verts:
                    point3 = sub(vertex, (0.0, 0.0, 0.056))
                    coords.append((center[0] + dot(point3, right) * 3400,
                                   center[1] - dot(point3, up) * 3400))
                    depth += dot(point3, eye)
                lighting = 0.38 + 0.62 * max(0.0, dot(normal, unit((0.45, -0.55, 0.70))))
                rgb = tuple(max(0, min(255, round(channel * lighting * 255)))
                            for channel in color)
                triangles.append((depth / 3, coords, rgb))
        for _, coords, color in sorted(triangles):
            draw.polygon(coords, fill=color)
        draw.text((center[0] - 150, 35), title, fill=(225, 230, 236))
    draw.text((24, 570), 'New procedural round prototype  |  33.7 mm x 113.5 mm envelope  |  scale and fit pending',
              fill=(188, 196, 207))
    destination.parent.mkdir(parents=True, exist_ok=True)
    canvas.save(destination)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path,
                        default=ROOT / 'assets/weapon-6dof/gnasher-round.glb')
    parser.add_argument('--preview', type=Path)
    parser.add_argument('--check', action='store_true')
    args = parser.parse_args()
    meshes = build_geometry()
    raw = glb_bytes(meshes)
    if args.check:
        if args.output.read_bytes() != raw:
            raise ValueError('Committed Gnasher round differs from procedural source')
    else:
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_bytes(raw)
    if args.preview:
        preview(meshes, args.preview)
    print(f'{len(raw)} bytes, {sum(len(mesh.indices) // 3 for mesh in meshes)} triangles')


if __name__ == '__main__':
    main()
