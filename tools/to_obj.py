"""glTF -> OBJ + MTL + PNG for the snow globe (Person 3).

Geometry is written in globe units: base at y = 0, centred on x/z, facing +Z,
so the C++ side only places it (translate + yaw). UVs are flipped to the OBJ
convention (origin bottom-left); textures are converted to PNG and downscaled.
"""
import os
import re

import numpy as np
from PIL import Image


def safe(name):
    return re.sub(r'[^A-Za-z0-9_]+', '_', name).strip('_') or 'mat'


def save_texture(src, out_dir, max_size, keep_alpha):
    im = Image.open(src)
    has_alpha = im.mode in ('RGBA', 'LA') or (im.mode == 'P' and 'transparency' in im.info)
    im = im.convert('RGBA' if (has_alpha and keep_alpha) else 'RGB')
    if max(im.size) > max_size:
        s = max_size / max(im.size)
        im = im.resize((max(1, int(im.width * s)), max(1, int(im.height * s))), Image.LANCZOS)
    name = os.path.splitext(os.path.basename(src))[0] + '.png'
    im.save(os.path.join(out_dir, name), optimize=True)
    return name, im.mode == 'RGBA'


def write_obj(path_obj, groups, materials, header=''):
    """groups: list of (material_key, pos(N,3), nrm(N,3), uv(N,2), idx(M,3))."""
    mtl_name = os.path.splitext(os.path.basename(path_obj))[0] + '.mtl'
    lines = [f'# {header}', f'mtllib {mtl_name}'] if header else [f'mtllib {mtl_name}']
    base = 1
    for key, pos, nrm, uv, idx in groups:
        lines.append(f'o {key}')
        lines.extend(f'v {p[0]:.5f} {p[1]:.5f} {p[2]:.5f}' for p in pos)
        lines.extend(f'vt {t[0]:.4f} {1.0 - t[1]:.4f}' for t in uv)       # OBJ: v goes up
        lines.extend(f'vn {n[0]:.4f} {n[1]:.4f} {n[2]:.4f}' for n in nrm)
        lines.append(f'usemtl {key}')
        for f in idx + base:
            lines.append(f'f {f[0]}/{f[0]}/{f[0]} {f[1]}/{f[1]}/{f[1]} {f[2]}/{f[2]}/{f[2]}')
        base += len(pos)
    with open(path_obj, 'w', newline='\n') as fh:
        fh.write('\n'.join(lines) + '\n')

    ml = []
    for key, m in materials.items():
        c = m['color']
        ml += [f'newmtl {key}', f'Kd {c[0]:.4f} {c[1]:.4f} {c[2]:.4f}', 'Ks 0.1 0.1 0.1', 'Ns 16', 'd 1']
        if m.get('Ke'):
            ml.append('Ke {:.3f} {:.3f} {:.3f}'.format(*m['Ke']))
        if m.get('map'):
            ml.append(f"map_Kd {m['map']}")
            if m.get('cutout'):
                ml.append(f"map_d {m['map']}")          # alpha channel = cut-out mask
        ml.append('')
    with open(os.path.join(os.path.dirname(path_obj), mtl_name), 'w', newline='\n') as fh:
        fh.write('\n'.join(ml))


def normalise(prims, height=None, width=None, yaw_deg=0.0, center=None):
    """Rotate about Y, then scale/translate so the base is at y=0 and x/z are centred."""
    a = np.radians(yaw_deg)
    R = np.array([[np.cos(a), 0, np.sin(a)], [0, 1, 0], [-np.sin(a), 0, np.cos(a)]])
    P = np.concatenate([p['pos'] for p in prims]) @ R.T
    lo, hi = P.min(0), P.max(0)
    size = hi - lo
    s = height / size[1] if height else width / max(size[0], size[2])
    c = np.array([(lo[0] + hi[0]) / 2, lo[1], (lo[2] + hi[2]) / 2]) if center is None else (np.asarray(center) @ R.T)
    c[1] = lo[1]
    return R, s, c


def convert(g, prims, out_dir, name, R, s, c, tex_max=512, extra_mat=None, header=''):
    os.makedirs(out_dir, exist_ok=True)
    groups, materials, tex_done = [], {}, {}
    for p in prims:
        m = g.material(p['material'])
        key = safe(m['name'])
        if key not in materials:
            entry = dict(color=m['color'][:3])
            if m['texture']:
                if m['texture'] not in tex_done:
                    tex_done[m['texture']] = save_texture(m['texture'], out_dir, tex_max, m['alpha'] != 'OPAQUE')
                entry['map'], has_alpha = tex_done[m['texture']]
                entry['cutout'] = has_alpha and m['alpha'] != 'OPAQUE'
            if extra_mat and key in extra_mat:
                entry.update(extra_mat[key])
            materials[key] = entry
        pos = ((p['pos'] @ R.T) - c) * s
        nrm = p['nrm'] @ R.T if p['nrm'] is not None else np.tile([0, 1, 0], (len(pos), 1))
        groups.append((key, pos, nrm, p['uv'], p['idx']))
    # merge primitives that share a material (fewer draw calls)
    merged = {}
    for key, pos, nrm, uv, idx in groups:
        if key in merged:
            P, N, U, I = merged[key]
            merged[key] = (np.vstack([P, pos]), np.vstack([N, nrm]), np.vstack([U, uv]), np.vstack([I, idx + len(P)]))
        else:
            merged[key] = (pos, nrm, uv, idx)
    out = [(k, *v) for k, v in merged.items()]
    path = os.path.join(out_dir, name + '.obj')
    write_obj(path, out, materials, header)
    P = np.concatenate([o[1] for o in out])
    tris = sum(len(o[4]) for o in out)
    print(f'{path}: {tris} triangles, {len(materials)} materials, size {np.round(P.max(0) - P.min(0), 3)}')
    return out
