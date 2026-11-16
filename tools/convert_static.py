"""Convert the hut, tree pack and snowman glTF downloads to OBJ in globe units (Person 3).

usage: python3 tools/convert_static.py <downloads dir> <models dir>
  <downloads dir>/hut/hut.gltf                                   (from snowy-wooden-hut/source/hut.zip)
  <downloads dir>/snow_pine_tree_and_bush_pack_lowpoly/scene.gltf
  <downloads dir>/fnaf_hw_snowman/scene.gltf
Prints the measured points (window light, chimney top...) used in src/InteriorLayout.h.
"""
import json
import os
import sys

import numpy as np

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from gltf_io import Gltf
from to_obj import normalise, convert

SRC, OUT = sys.argv[1], sys.argv[2]
info = {}

# ---- hut: door on +Z, window on +X ----
g = Gltf(f'{SRC}/hut/hut.gltf')
prims = list(g.primitives())
R, s, c = normalise(prims, width=0.52)
convert(g, prims, f'{OUT}/hut', 'hut', R, s, c, tex_max=512,
        extra_mat={'mWindowGlass': dict(color=[0.12, 0.13, 0.16], Ke=[1.0, 0.74, 0.40])},
        header='Snowy Wooden Hut by gecko0307 (CC BY 4.0), converted for the snow globe')
W = g.world()
def tf(p): return list(np.round(((np.asarray(p) @ R.T) - c) * s, 4))
info['hut'] = dict(scale=s,
    window_light=tf(W[[i for i, n in enumerate(g.nodes) if n.get('name') == 'Point'][0]][:3, 3]),
    door_light=tf(W[[i for i, n in enumerate(g.nodes) if n.get('name') == 'Point.001'][0]][:3, 3]),
    window_glass_center=tf([2.83, 2.32, 0.0]), chimney_top=tf([0.0, 6.19, -1.835]),
    door_center_bottom=tf([0.0, 0.24, 3.05]), door_height=round(2.28 * s, 4))

# ---- trees ----
g = Gltf(f'{SRC}/snow_pine_tree_and_bush_pack_lowpoly/scene.gltf')
prims = list(g.primitives())
def pick(prefix): return [p for p in prims if g.nodes[g.parent[p['node']]].get('name') == prefix]
for name, node, h in [('pine', 'SnowTree1', 0.42), ('pine_tall', 'SnowTree2', 0.50),
                      ('shrub', 'SnowTree1.001', 0.13), ('bush', 'BushBare1', 0.16)]:
    sel = pick(node)
    R, s, c = normalise(sel, height=h)
    convert(g, sel, f'{OUT}/trees', name, R, s, c, tex_max=512,
            extra_mat={'SnowBranchCircle': dict(color=[1.9, 2.0, 2.0])},     # dark texture: brighten for night
            header=f'{node} from Snow Pine Tree and Bush Pack LOWPOLY by EFX (CC BY 4.0)')

# ---- snowman ----
g = Gltf(f'{SRC}/fnaf_hw_snowman/scene.gltf')
prims = list(g.primitives())
R, s, c = normalise(prims, height=0.17)
convert(g, prims, f'{OUT}/snowman', 'snowman', R, s, c, tex_max=1024,
        header='FNaF HW Snowman by Captain Allen (CC BY 4.0)')
print(json.dumps(info, indent=1))
