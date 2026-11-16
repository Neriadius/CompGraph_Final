// Implementations of the two single-header libraries used to load models (Person 3).
// Each header is compiled once, here.
//   tinyobjloader  - MIT,           https://github.com/tinyobjloader/tinyobjloader
//   stb_image      - public domain, https://github.com/nothings/stb

// Third-party code: keep its warnings out of our build output.
#if defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"
#endif

#define TINYOBJLOADER_IMPLEMENTATION
#include "../libs/tiny_obj_loader.h"

#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_PNG
#define STBI_ONLY_JPEG
#include "../libs/stb_image.h"

#if defined(__GNUC__)
#pragma GCC diagnostic pop
#endif
