# Snow Globe

Glass snow globe with a tiny world inside. Shake it with a key or the mouse.

Current state: glass globe, snow, wooden base with winding key, music box sound,
interior scene (log hut, pine trees, campfire, character).

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
    src/Interior        owns the interior: update, lights, toggles, drawing
    src/InteriorLayout.h  positions and sizes of everything + height functions (shared with the snow)
    src/InteriorModels  palette, snow ground, pine trees, fire pit, seats
    src/HutModel.cpp    log cabin (windows, door, lantern, chimney, roof, woodpile)
    src/Character       sitting character, transformation hierarchy
    src/Campfire        flame tongues, sparks, glow, flicker
    src/Smoke           chimney smoke
    src/MeshBuilder     primitives (box, cylinder/cone, sphere, lathe, prism, torus, grid) -> one VAO
    src/SceneLights     interior lights for any shader (hand-off to Person 4)
    shaders/interior.*  Blinn-Phong, moon + 6 point lights, procedural bark/rings/planks/stone/snow/knit
    shaders/flame.* embers.* glow.* smoke.*

## Interior scene (Person 3)
- Snow ground disc with drifts, a trodden path from the door to the fire and a scorched ring.
- Six pine trees: three cone tiers with wavy hems, each wearing a snow cap.
- Log cabin built from cylinders: logs split around the openings, tree rings on the cut ends,
  lit windows on all four walls (shutters, snow on the sills), plank door with a wreath,
  lantern, round gable window, stone chimney with smoke, snowy roof with icicles,
  woodpile and chopping stump at the back.
- Campfire: stone ring, crossed + leaning logs, glowing coals, animated flame tongues
  (vertex shader), rising sparks, soft glow. The flame, the coal glow and the fire light
  share one flicker function.
- Character sitting on a log, facing the fire: hierarchy root -> torso -> head / upper arm -> forearm,
  breathing, looking around, rubbing the hands over the fire.
- Snow settles on the drifts, the roof and the pine branches: `SurfaceHeightAt` uses
  `layout::SurfaceOffset` (same numbers as the geometry).
- Everything is one static mesh (1 draw call) + 7 small character meshes + flame, sparks, glow, smoke
  (the last three have no vertex buffers: geometry comes from `gl_VertexID`).

Lights for other shaders: `scene.Lights()` returns `SceneLights` (world space, includes the shake):
moon direction/colour, hemisphere ambient and up to 6 point lights (slot 0 = campfire,
then the hut windows and the lantern). `UploadSceneLights(program, lights)` sets the uniforms;
the GLSL block to paste is in `src/SceneLights.h`, a full example is `shaders/interior.frag`.

## Plan
- Core: transparent sphere + specular, snow particles (shake -> fly up, slowly fall), textured wooden base
- Stretch: cubemap reflections, warm window lights at night, music box sound
- Wow: hard shake -> blizzard that settles
