#define GL_TYPES(M) /*
*/  M(GLchar, u8) /*
*/  M(GLenum, u32) /*
*/  M(GLint, i32) /*
*/  M(GLuint, u32) /*
*/  M(GLintptr, i64) /*
*/  M(GLsizei, i32) /*
*/  M(GLsizeiptr, i64) /*
*/  M(GLvoid, void) /*
*/  M(GLclampf, float) /*
*/  M(GLbitfield, u32) /*
*/  M(GLfloat, f32) /*
*/  M(GLboolean, bool) /*
*/ 

#define GL_FUNCS(M) /*
*/  M(glViewport, void, GLint x, GLint y, GLsizei width, GLsizei height ) /*
*/  M(glAttachShader, void, GLuint program, GLuint shader) /*
*/  M(glBindBuffer, void,GLenum target, GLuint buffer) /*
*/  M(glBindTexture, void, GLenum target, GLuint texture ) /*
*/  M(glBindVertexArray, void,GLuint array) /*
*/  M(glBlendFunc, void, GLenum sfactor, GLenum dfactor ) /*
*/  M(glBufferData, void, GLenum target, GLsizeiptr size, const void *data, GLenum usage) /*
*/  M(glBufferSubData, void, GLenum target, GLintptr offset, GLsizeiptr size, const void *data) /*
*/  M(glClear, void, GLbitfield mask ) /*
*/  M(glClearColor, void, GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha ) /*
*/  M(glCompileShader, void, GLuint shader) /*
*/  M(glCreateProgram, GLuint, void) /*
*/  M(glCreateShader, GLuint, GLenum type) /*
*  M(GLDEBUGPROC, void, GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar *message, const void *userParam) *
*/  M(glDebugMessageCallback, void, GLDEBUGPROC callback, const void *userParam) /*
*/  M(glDeleteBuffers, void, GLsizei n, const GLuint *buffers) /*
*/  M(glDeleteShader, void, GLuint shader) /*
*/  M(glDrawArraysInstanced, void, GLenum mode, GLint first, GLsizei count, GLsizei instancecount) /*
*/  M(glEnable, void, GLenum cap ) /*
*/  M(glEnableVertexAttribArray, void, GLuint index) /*
*/  M(glGenBuffers, void, GLsizei n, GLuint *buffers) /*
*/  M(glGenTextures, void, GLsizei n, GLuint *textures ) /*
*/  M(glGenVertexArrays, void, GLsizei n, GLuint *arrays) /*
*/  M(glGetProgramInfoLog, void, GLuint program, GLsizei bufSize, GLsizei *length, GLchar *infoLog) /*
*/  M(glGetProgramiv, void, GLuint program, GLenum pname, GLint *params) /*
*/  M(glGetShaderInfoLog, void, GLuint shader, GLsizei bufSize, GLsizei *length, GLchar *infoLog) /*
*/  M(glGetShaderiv, void, GLuint shader, GLenum pname, GLint *params) /*
*/  M(glGetUniformLocation, GLint, GLuint program, const GLchar *name) /*
*/  M(glLinkProgram, void, GLuint program) /*
*/  M(glShaderSource, void, GLuint shader, GLsizei count, const GLchar *const*string, const GLint *length) /*
*/  M(glTexImage2D, void, GLenum target, GLint level, GLint internalFormat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid *pixels) /*
*/  M(glTexParameteri, void, GLenum target, GLenum pname, GLint param ) /*
*/  M(glTexSubImage2D, void, GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *pixels) /*
*/  M(glUniform2f, void, GLint location, GLfloat v0, GLfloat v1) /*
*/  M(glUseProgram, void, GLuint program) /*
*/  M(glVertexAttribDivisor, void, GLuint index, GLuint divisor) /*
*/  M(glVertexAttribPointer, void, GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const void *pointer) /*
*/

#define GL_ARRAY_BUFFER 0x8892
#define GL_BLEND 0x0BE2
#define GL_CLAMP_TO_EDGE 0x812F
#define GL_COLOR_BUFFER_BIT 0x00004000
#define GL_COMPILE_STATUS 0x8B81
#define GL_DEBUG_OUTPUT 0x92E0
#define GL_DEBUG_OUTPUT_SYNCHRONOUS 0x8242
#define GL_DEBUG_SEVERITY_HIGH 0x9146
#define GL_DEBUG_SEVERITY_LOW 0x9148
#define GL_DEBUG_SEVERITY_MEDIUM 0x9147
#define GL_DEBUG_SEVERITY_NOTIFICATION 0x826B
#define GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR 0x824D
#define GL_DEBUG_TYPE_ERROR 0x824C
#define GL_DEBUG_TYPE_OTHER 0x8251
#define GL_DEBUG_TYPE_PERFORMANCE 0x8250
#define GL_DEBUG_TYPE_PORTABILITY 0x824F
#define GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR 0x824E
#define GL_DYNAMIC_DRAW 0x88E8
#define GL_FRAGMENT_SHADER 0x8B30
#define GL_LINEAR 0x2601
#define GL_LINK_STATUS 0x8B82
#define GL_ONE_MINUS_SRC_ALPHA 0x0303
#define GL_RED 0x1903
#define GL_RGBA 0x1908
#define GL_SRC_ALPHA 0x0302
#define GL_TEXTURE_2D 0x0DE1
#define GL_TEXTURE_MAG_FILTER 0x2800
#define GL_TEXTURE_MIN_FILTER 0x2801
#define GL_TEXTURE_WRAP_S 0x2802
#define GL_TEXTURE_WRAP_T 0x2803
#define GL_TRIANGLE_STRIP 0x0005
#define GL_UNSIGNED_BYTE 0x1401
#define GL_VERTEX_SHADER 0x8B31
#define GL_FALSE 0
#define GL_TRUE 1	
#define GL_BYTE	0x1400
#define GL_UNSIGNED_BYTE 0x1401
#define GL_SHORT 0x1402
#define GL_UNSIGNED_SHORT 0x1403
#define GL_INT 0x1404
#define GL_UNSIGNED_INT	0x1405
#define GL_FLOAT 0x1406
#define GL_DOUBLE 0x140A

#ifdef MACROS_H

#define DEFINE_TYPES(name, val) \
    typedef val name;

GL_TYPES(DEFINE_TYPES)

#define DEFINE_FUNC_TYPES(name, return_val, ...) \
    typedef return_val (__VARED_##name)(__VA_ARGS__);

typedef void (*GLDEBUGPROC)(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar *message, const void *userParam);
GL_FUNCS(DEFINE_FUNC_TYPES)

#define DEFINE_FUNC_GLOBALS(name, return_val, ...) \
    global __VARED_##name *name = 0;

GL_FUNCS(DEFINE_FUNC_GLOBALS)

#endif // MACROS_H
