#ifndef RENDER_INC_H
#define RENDER_INC_H

#include "render.h"

#if OS_LINUX
#define GLAD_GL_IMPLEMENTATION
// #include "../third_party/glad/gl.h"
#include <SDL2/SDL.h>
// #include <SDL2/SDL_opengl.h>
#include "opengl/render_opengl.h"
#endif

#endif // RENDER_INC_H
