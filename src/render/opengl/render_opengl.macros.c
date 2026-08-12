#include "render_opengl.macros.h"
#ifdef MACROS_C

#if OS_LINUX

#define DEFINE_LOAD_FUNC(name, return_type, ...) \
    return_type (*name##_ptr)(__VA_ARGS__) = dlsym(gl_handle, #name); \
    if (!name##_ptr) { fprintf(stderr, "ded at %s:%d\n", __FILE__, __LINE__); assert(0); } \
    else { name = *name##_ptr; }

void load_gl(void) {
    void *gl_handle = dlopen("libGL.so", RTLD_LAZY);
    GL_FUNCS(DEFINE_LOAD_FUNC)
    dlclose(gl_handle);
}

#endif

#endif // MACROS_C
