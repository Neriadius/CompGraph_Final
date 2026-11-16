# Snow Globe

Glass snow globe with a tiny world inside. Shake it with a key or the mouse.

Current state: glass globe, snow, wooden base with winding key, music box sound,
interior scene (log hut, pine trees, snowman, campfire, Typhoeus sitting by the fire).

## Build (MSYS2 UCRT64)

    pacman -S make mingw-w64-ucrt-x86_64-gcc mingw-w64-ucrt-x86_64-freeglut
    make run

## Controls
    SPACE / left mouse   shake the globe (the winding key turns; the 3rd shake starts the music box)
    L              hut lights on / off (windows, lantern, chimney smoke fade)
    F              campfire on / off (flame, sparks and fire light fade)
    C              toggle ORBIT / FREE camera
    P              parallel / perspective
    ORBIT: arrows = angle / height, +/- = zoom
    FREE : W A S D move, Q / E down / up,
           right-mouse drag or arrows = look, +/- = speed
    ESC            quit

## Layout
    main.cpp       window, callbacks, placeholder sphere
    src/           Camera, Shader, VAO, VBO, EBO
    shaders/       GLSL files
    textures/      images (PNG), loaded by src/Texture (uses src/Image)
    audio/         music_box.mp3 (tune), winding_key.mp3 (key sound); played by src/MusicBox
    glad/, glm/    libraries

    Interior (Person 3):
    src/Interior          owns the interior: update, lights, toggles, drawing, model placement
    src/InteriorLayout.h  positions of everything + height functions (shared with the snow)
    src/ModelHeights.h    GENERATED top surfaces of the hut / pine models (tools/heights.py)
    src/InteriorModels    palette, snow ground, fire pit, seats (procedural)
    src/HutProps.cpp      lantern, woodpile, chopping stump, snow banks around the hut model
    src/Model             OBJ model loader (tinyobjloader + stb_image), morph animation
    src/Campfire          flame tongues, sparks, glow, flicker
    src/Smoke             chimney smoke
    src/MeshBuilder       primitives (box, cylinder/cone, sphere, torus, grid) -> one VAO
    src/SceneLights       interior lights for any shader (hand-off to Person 4)
    shaders/interior.frag Blinn-Phong, moon + 6 point lights, textures + cut-out, procedural bark/rings/stone/snow
    shaders/interior.vert procedural mesh;  shaders/model.vert  OBJ models (two-pose morph)
    shaders/flame.* embers.* glow.* smoke.*
    models/               hut, trees, snowman, typhoeus (OBJ + MTL + PNG, globe units)
    libs/                 tiny_obj_loader.h, stb_image.h (single-header libraries)
    tools/                Python scripts that converted the Sketchfab glTF files to models/

## Interior scene (Person 3)
- Snow ground disc with drifts, a trodden path from the door to the fire and a scorched ring.
- Models (credited in CREDITS.md, converted from glTF to OBJ by `tools/`): snowy log hut, pine trees,
  shrubs and bare bushes, a snowman, and Typhoeus sitting on a log by the fire.
- The hut window glass glows with the hut lights (L); a warm point light sits outside the window and
  in the lantern by the door. Chimney smoke fades with the lights.
- Typhoeus was posed sitting from her skeleton (`tools/pose_typhoeus.py`: forward kinematics +
  linear blend skinning in Python). Two key poses with the same mesh are exported; `model.vert`
  blends them, so she watches the fire, now and then glances aside and lifts her hands, and breathes.
- Campfire: stone ring, crossed + leaning logs, glowing coals, animated flame tongues
  (vertex shader), rising sparks, soft glow. The flame, the coal glow and the fire light
  share one flicker function.
- Snow settles on the drifts, the hut roof and the pine branches: `SurfaceHeightAt` uses
  `layout::SurfaceOffset`, which samples height maps made from the models (transparent parts of
  the branch textures ignored).
- Placement hierarchy: globe (shake) -> hut -> lantern / smoke / window light,
  globe -> seat log -> Typhoeus, globe -> fire pit -> flame / sparks / glow / fire light.
- Procedural parts are one static mesh (1 draw call); sparks, glow and smoke have no vertex
  buffers (geometry comes from `gl_VertexID`).

Lights for other shaders: `scene.Lights()` returns `SceneLights` (world space, includes the shake):
moon direction/colour, hemisphere ambient and up to 6 point lights (slot 0 = campfire,
then the hut window and the lantern). `UploadSceneLights(program, lights)` sets the uniforms;
the GLSL block to paste is in `src/SceneLights.h`, a full example is `shaders/interior.frag`.

## Plan
- Core: transparent sphere + specular, snow particles (shake -> fly up, slowly fall), textured wooden base
- Stretch: cubemap reflections, warm window lights at night, music box sound
- Wow: hard shake -> blizzard that settles
