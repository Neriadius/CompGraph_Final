# Snow Globe

Computer Graphics final project, topic 8: a glass snow globe with a tiny winter world inside.
Shake it with a key or the mouse and the snow flies up, then slowly settles. C++17, OpenGL 3.3
core profile, freeglut, GLAD, GLM.

## Team

| Member | Part | What they did |
|--------|------|---------------|
| Sharshiken Ali | Globe, glass, rendering pipeline, camera integration | Transparent glass sphere with specular highlights that works from outside and inside (both faces drawn, with the draw order and blending that keep the interior and the snow visible through it); shake input (Space / left mouse -> `shakeIntensity`); camera integration so the camera can move freely; cubemap reflections on the glass, also from inside |
| Ibrahim Nygyman | Snow and blizzard | Snow particle system: flakes fly up on a shake and fall slowly; a hard shake turns into a blizzard that settles over time; flakes stay inside the sphere, do not fall through the ground, hut or trees, and look right when the camera is among them |
| Alikhan Nurzhan | Interior scene | Snowy terrain and ground, trees and hut (imported models), the character sitting by the campfire with a simple animation, the campfire with a flickering flame; warm window lights and the fire light handed to the shared lighting; detail from every side for the free camera |
| Daryn Museyev | Base, materials, lighting, audio, integration | Wooden base with a texture and the texture loading everyone reuses; the shared lighting shader for the interior (night mood, fire light, window lights); music box sound; final integration, testing in both camera modes, README and report |

## Features

Topic core
- Miniature scene inside a transparent glass sphere with specular highlights.
- Snow particles (10 000 flakes, one draw call) that fly up when the globe is shaken and slowly fall,
  resting on the ground, the hut roof and the pine branches.
- Wooden base with a mipmapped wood texture.

Topic stretch goals
- Reflections on the glass from a cubemap (procedural night sky), correct from inside as well.
- Warm window lights on the hut at night (L) and a flickering campfire light.
- Music box sound: the winding key clicks on every shake, the tune starts after the third shake.

Wow moment
- A hard shake turns the globe into a blizzard (turbulence and swirl) that slowly settles.

More
- Start menu: the globe blurred behind the title, a Start button that clears the blur smoothly,
  and the team names.
- Controls list on screen from the start, in the FPS counter's pixel font (H hides / shows it);
  FPS counter in the window title and on screen (;).
- Day / night (I) with a dawn tint; snowy mountain terrain with baked shadows; campfire with
  flame, sparks and glow; chimney smoke; a sitting character with a two-pose morph animation.
- Brass winding key that turns while the globe is shaken; curved brass nameplate on the base.
- Orbit and free camera (frame-rate independent, works inside the globe), perspective / parallel.
- 4x MSAA.

## Build (MSYS2 UCRT64)

Open the **MSYS2 UCRT64** terminal:

    pacman -S --needed git make mingw-w64-ucrt-x86_64-gcc mingw-w64-ucrt-x86_64-freeglut
    git clone https://github.com/Neriadius/CompGraph_Final.git
    cd CompGraph_Final
    make run

`make` builds `snowglobe.exe` (freeglut is linked statically when the static library is
installed, so the exe also runs outside MSYS2), `make run` builds and starts it, `make clean`
removes the build files. The program finds `shaders/`, `textures/`, `models/` and `audio/` next to the exe.

## Controls

    MENU           click Start, or press Enter / Space
    SPACE / left mouse   shake the globe (the winding key turns; the 3rd shake starts the music box)
    L              hut lights on / off (windows, lantern, chimney smoke fade)
    F              campfire on / off (flame, sparks and fire light fade)
    I              day / night (sunrise / sunset over 3 seconds)
    H              controls list on / off (shown from the start)
    ;              FPS counter on / off (window title and overlay)
    C              toggle ORBIT / FREE camera
    P              parallel / perspective
    ORBIT: arrows = angle / height, +/- = zoom
    FREE : W A S D move, Q / E down / up,
           right-mouse drag or arrows = look, +/- = speed
    ESC            quit

## Layout
    main.cpp       window, GLUT callbacks, frame order (scene -> glass back -> snow -> glass front -> menu / HUD)
    src/           shaderClass, VAO, VBO, EBO (OpenGL wrappers), Camera (orbit / free), Shake.h
                   Globe + Cubemap      glass sphere passes, procedural night-sky cubemap
                   Snow                 snow particles and blizzard
                   Scene                wooden base, interior, winding key, nameplate, shared lights
                   Texture + Image      PNG loading (own decoder), mipmaps
                   WindingKey, Nameplate, MusicBox (Windows MCI), FpsCounter
                   Menu, UiRenderer, UiFont, BlurPass   start menu, SDF text, menu blur
                   ControlsOverlay, PixelFont           in-game controls list, 5x7 HUD font (also FpsCounter)
    shaders/       GLSL files (.vert / .frag)
    textures/      images (PNG), loaded by src/Texture; ui_font.png + .txt = menu font atlas
    audio/         music_box.mp3 (tune), winding_key.mp3 (key sound); played by src/MusicBox
    models/        hut, trees, snowman, typhoeus (OBJ + MTL + PNG, globe units)
    glad/, glm/, libs/   libraries (libs/: tiny_obj_loader.h, stb_image.h)
    tools/         Python scripts: model conversion, snow height maps, font atlas

    Interior:
    src/Interior          owns the interior: update, lights, toggles, drawing, model placement
    src/InteriorLayout.h  positions of everything + terrain height map (mountains) + height functions
                          (shared with the snow)
    src/ModelHeights.h    GENERATED top surfaces of the hut / pine models (tools/heights.py)
    src/InteriorModels    palette, snow ground, fire pit, seats (procedural)
    src/HutProps.cpp      lantern, woodpile, chopping stump, snow banks around the hut model
    src/Model             OBJ model loader (tinyobjloader + stb_image), morph animation
    src/Campfire          flame tongues, sparks, glow, flicker
    src/Smoke             chimney smoke
    src/MeshBuilder       primitives (box, cylinder/cone, sphere, torus, grid) -> one VAO
    src/SceneLights       interior lights for any shader (hand-off to the shared lighting), day / night values
    shaders/interior.frag Blinn-Phong, moon + 6 point lights, textures + cut-out, procedural bark/rings/stone/snow
    shaders/interior.vert procedural mesh;  shaders/model.vert  OBJ models (two-pose morph)
    shaders/flame.* embers.* glow.* smoke.*

## Interior scene
- Terrain: snowy mountains around the back of the globe and a flat meadow in front. Gaussian
  peaks made craggy by ridged noise (domain-warped), computed once into a 200 x 200 height map
  that both the ground mesh and the snow use. The shader puts snow on gentle slopes and high up
  and bare, layered rock on steep faces; mountain shadows for the moon and the sun are baked into
  the ground vertices (ray march over the height map). Trees on slopes are sunk by the slope.
- A trodden path from the door to the fire and a scorched ring around it.
- Day / night (I): the key light swings from the moon to the sun with a warm dawn tint half way;
  ambient, background colour, glass reflection (day sky), wooden base and brass key light,
  chimney smoke colour and the lamps all follow `daylight` (0..1) in `SceneLights`.
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

## AI Use

AI Assistance has been utilised procedurally generated materials for brass key, mountains, campfire and snow ground, as well as model for interior mountain. Additionally, AI has been used to assisst in integrating sounds (music box, brass key turning), debugging smooth camera motion, and writing utility python scripts in /tools.

## Credits

Third-party models, textures, fonts, libraries, code and tutorials are listed in [CREDITS.md](CREDITS.md).
