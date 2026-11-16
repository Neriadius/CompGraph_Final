# Credits

## Models (Sketchfab, converted from glTF to OBJ + PNG with tools/; geometry rescaled, Typhoeus re-posed)

| Model | Author | Link | License | Used in |
|-------|--------|------|---------|---------|
| Snowy Wooden Hut | gecko0307 | https://sketchfab.com/3d-models/snowy-wooden-hut-857c02ab61834df8a8d125ccce6fec2a | CC BY 4.0 | models/hut |
| Snow Pine Tree and Bush Pack LOWPOLY | EFX | https://sketchfab.com/3d-models/snow-pine-tree-and-bush-pack-lowpoly-7afc4ca23dd84adea90dc0ba8506c500 | CC BY 4.0 | models/trees |
| FNaF HW Snowman | Captain Allen | https://sketchfab.com/3d-models/fnaf-hw-snowman-5246a3b846ae4b32baec02858bd3d7a5 | CC BY 4.0 | models/snowman |
| Typhoeus / Arknights Endfield | Abcxyz51 | https://sketchfab.com/3d-models/typhoeus-arknights-endfield-2f36d643349c430db585dc23461b5838 | CC BY 4.0 | models/typhoeus |

Typhoeus is a character from *Arknights: Endfield* (Hypergryph / GRYPHLINE); the snowman is from
*Five Nights at Freddy's: Help Wanted*. Both models are fan uploads, credited as their uploaders licensed them.

## Libraries

| Library | Author | Link | License | Used in |
|---------|--------|------|---------|---------|
| tinyobjloader (tiny_obj_loader.h) | Syoyo Fujita and contributors | https://github.com/tinyobjloader/tinyobjloader | MIT | libs/, src/Model.cpp |
| stb_image (stb_image.h) | Sean Barrett | https://github.com/nothings/stb | Public domain / MIT | libs/, src/Model.cpp |
| GLAD (OpenGL 3.3 core loader, generated) | David Herberth (Dav1dde) | https://github.com/Dav1dde/glad | MIT | glad/ |
| GLM (OpenGL Mathematics) | G-Truc Creation | https://github.com/g-truc/glm | MIT (or Happy Bunny License) | glm/, all matrix / vector code |
| freeglut (installed with MSYS2, not in the repo) | Pawel W. Olszta and the freeglut team | https://freeglut.sourceforge.net/ | X11 / MIT | window, input, main loop |

## Code and ideas

| What | Author | Link | License | Used in |
|------|--------|------|---------|---------|
| "Hash without Sine" hash functions | Dave Hoskins | https://www.shadertoy.com/view/4djSRW | MIT | shaders/interior.frag, shaders/flame.frag, shaders/smoke.frag |
| Blinn-Phong lighting, point light attenuation (idea) | Joey de Vries, LearnOpenGL | https://learnopengl.com/Lighting/Advanced-Lighting | CC BY-NC 4.0 | shaders/interior.frag |
| OpenGL tutorial series: Shader, VAO, VBO, EBO classes and the camera's Matrix / Inputs structure | Victor Gordan | https://github.com/VictorGordan/opengl-tutorials (videos: https://www.youtube.com/playlist?list=PLPaoO-vpZnumdcb4tZc4x5Q-v7CkrQ6M-) | No license stated in the repository | src/shaderClass, src/VAO, src/VBO, src/EBO, src/Camera |
| Signed distance field text (idea) | Chris Green, Valve, "Improved Alpha-Tested Magnification for Vector Textures and Special Effects" (SIGGRAPH 2007) | https://cdn.akamai.steamstatic.com/apps/valve/2007/SIGGRAPH2007_AlphaTestedMagnification.pdf | Paper (technique only) | tools/make_font_atlas.py, shaders/ui_text.frag |
| Rounded rectangle distance function | Inigo Quilez | https://iquilezles.org/articles/distfunctions2d/ | Formula from the article, no license stated | shaders/ui_rect.frag |
| Gaussian blur with linear sampling (5 taps for a 9-tap kernel, offsets and weights) | Daniel Rakos | https://www.rastergrid.com/blog/2010/09/efficient-gaussian-blur-with-linear-sampling/ | Article (technique only) | shaders/post_blur.frag |

The ground, campfire, seats, lantern, woodpile and stump are built in code from primitives.

## Texture sources

| Texture | Author | Link | License | Used in |
|---------|--------|------|---------|---------|
| Realistic Seamless Wood Texture (wood.png) | SpringySpringo | https://opengameart.org/content/realistic-seamless-wood-texture | CC0 | wooden base |
| Asterisk icon, white (snowflake.png) | Icons8 | https://icons8.com/icons/set/asterisk--white | Icons8 free license (requires a link to https://icons8.com) | snow flakes (src/Snow) |
| "Vinter Varme" nameplate (vinter_varme_plate.png) | Daryn Museyev (team member) | - | Own work | nameplate on the base (src/Nameplate) |

## Sounds

| Sound | Author | Link | License | Used in |
|-------|--------|------|---------|---------|
| Music box tune (audio/music_box.mp3) | Daryn Museyev (team member) | - | Own work | src/MusicBox |
| Winding key (audio/winding_key.mp3) | Daryn Museyev (team member) | - | Own work | src/MusicBox |

## Fonts (start menu)

Baked into `textures/ui_font.png` + `textures/ui_font.txt` by `tools/make_font_atlas.py`
(font files from the Fontsource npm packages `@fontsource/cormorant-garamond` and `@fontsource/inter`).

| Font | Author | Link | License | Used in |
|------|--------|------|---------|---------|
| Cormorant Garamond SemiBold | Christian Thalmann (The Cormorant Project Authors) | https://github.com/CatharsisFonts/Cormorant | SIL Open Font License 1.1 | menu title |
| Inter Medium and SemiBold | Rasmus Andersson (The Inter Project Authors) | https://github.com/rsms/inter | SIL Open Font License 1.1 | menu text |
