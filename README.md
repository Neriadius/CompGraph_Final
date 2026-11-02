# Snow Globe

Glass snow globe with a tiny world inside. Shake it with a key or the mouse.

Current state: window + orbit/free camera + placeholder sphere.

## Build (MSYS2 UCRT64)

    pacman -S make mingw-w64-ucrt-x86_64-gcc mingw-w64-ucrt-x86_64-freeglut
    make run

## Controls
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
    glad/, glm/    libraries

## Plan
- Core: transparent sphere + specular, snow particles (shake -> fly up, slowly fall), textured wooden base
- Stretch: cubemap reflections, warm window lights at night, music box sound
- Wow: hard shake -> blizzard that settles
