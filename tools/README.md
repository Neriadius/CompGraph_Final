# Model conversion tools (Person 3)

Python 3 + numpy + Pillow (`pip install numpy pillow`). They turn the Sketchfab glTF downloads
into the OBJ + MTL + PNG files in `models/` (globe units: base at y = 0, facing +Z) and make the
snow height maps. Only needed to redo the models; the program itself just reads `models/`.

    python3 tools/convert_static.py <downloads> models          # hut, trees, snowman
    python3 tools/pose_typhoeus.py <downloads>/typhoeus__arknights_endfield/scene.gltf models/typhoeus
    python3 tools/heights.py models src/ModelHeights.h           # after any model change

- `gltf_io.py`       minimal glTF reader: accessors, node transforms, skinning (linear blend skinning)
- `to_obj.py`        writes OBJ/MTL (UVs flipped to the OBJ convention), textures to PNG, downscaled
- `pose_typhoeus.py` sitting pose by forward kinematics: each bone is turned to a target
  direction (thighs forward, shins down, hands to the fire, tail draped behind the log); exports
  pose 0 and pose 1 with the same mesh for the morph animation in `shaders/model.vert`
- `heights.py`       casts rays down onto the hut and trees (ignores transparent texture pixels)
  and writes `src/ModelHeights.h`, used by `SurfaceHeightAt` so snow rests on roof and branches

## Menu font atlas

`make_font_atlas.py` (needs numpy, scipy and Pillow with raqm) bakes the menu fonts into a signed
distance field atlas: `textures/ui_font.png` (glyphs) and `textures/ui_font.txt` (glyph boxes,
advances, kerning), read by `src/UiFont`. Fonts: Cormorant Garamond SemiBold (title) and Inter
Medium / SemiBold (text), both SIL Open Font License, e.g. from `npm pack @fontsource/cormorant-garamond @fontsource/inter`.

    python3 tools/make_font_atlas.py cormorant-garamond-latin-600-normal.woff \
        inter-latin-500-normal.woff inter-latin-600-normal.woff textures/ui_font.png textures/ui_font.txt
