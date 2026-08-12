/*
*/ typedef u8 GLchar; /*
*/ typedef u32 GLenum; /*
*/ typedef i32 GLint; /*
*/ typedef u32 GLuint; /*
*/ typedef i64 GLintptr; /*
*/ typedef i32 GLsizei; /*
*/ typedef i64 GLsizeiptr; /*
*/ typedef void GLvoid; /*
*/ typedef float GLclampf; /*
*/ typedef u32 GLbitfield; /*
*/ typedef f32 GLfloat; /*
*/ typedef bool GLboolean; /*
*/
typedef void (*GLDEBUGPROC)(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar *message, const void *userParam);
/*
*/ typedef void (__VARED_glViewport)(GLint x, GLint y, GLsizei width, GLsizei height); /*
*/ typedef void (__VARED_glAttachShader)(GLuint program, GLuint shader); /*
*/ typedef void (__VARED_glBindBuffer)(GLenum target, GLuint buffer); /*
*/ typedef void (__VARED_glBindTexture)(GLenum target, GLuint texture); /*
*/ typedef void (__VARED_glBindVertexArray)(GLuint array); /*
*/ typedef void (__VARED_glBlendFunc)(GLenum sfactor, GLenum dfactor); /*
*/ typedef void (__VARED_glBufferData)(GLenum target, GLsizeiptr size, const void *data, GLenum usage); /*
*/ typedef void (__VARED_glBufferSubData)(GLenum target, GLintptr offset, GLsizeiptr size, const void *data); /*
*/ typedef void (__VARED_glClear)(GLbitfield mask); /*
*/ typedef void (__VARED_glClearColor)(GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha); /*
*/ typedef void (__VARED_glCompileShader)(GLuint shader); /*
*/ typedef GLuint (__VARED_glCreateProgram)(void); /*
*/ typedef GLuint (__VARED_glCreateShader)(GLenum type); /*
*  M(GLDEBUGPROC, void, GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar *message, const void *userParam) *
*/ typedef void (__VARED_glDebugMessageCallback)(GLDEBUGPROC callback, const void *userParam); /*
*/ typedef void (__VARED_glDeleteBuffers)(GLsizei n, const GLuint *buffers); /*
*/ typedef void (__VARED_glDeleteShader)(GLuint shader); /*
*/ typedef void (__VARED_glDrawArraysInstanced)(GLenum mode, GLint first, GLsizei count, GLsizei instancecount); /*
*/ typedef void (__VARED_glEnable)(GLenum cap); /*
*/ typedef void (__VARED_glEnableVertexAttribArray)(GLuint index); /*
*/ typedef void (__VARED_glGenBuffers)(GLsizei n, GLuint *buffers); /*
*/ typedef void (__VARED_glGenTextures)(GLsizei n, GLuint *textures); /*
*/ typedef void (__VARED_glGenVertexArrays)(GLsizei n, GLuint *arrays); /*
*/ typedef void (__VARED_glGetProgramInfoLog)(GLuint program, GLsizei bufSize, GLsizei *length, GLchar *infoLog); /*
*/ typedef void (__VARED_glGetProgramiv)(GLuint program, GLenum pname, GLint *params); /*
*/ typedef void (__VARED_glGetShaderInfoLog)(GLuint shader, GLsizei bufSize, GLsizei *length, GLchar *infoLog); /*
*/ typedef void (__VARED_glGetShaderiv)(GLuint shader, GLenum pname, GLint *params); /*
*/ typedef GLint (__VARED_glGetUniformLocation)(GLuint program, const GLchar *name); /*
*/ typedef void (__VARED_glLinkProgram)(GLuint program); /*
*/ typedef void (__VARED_glShaderSource)(GLuint shader, GLsizei count, const GLchar *const*string, const GLint *length); /*
*/ typedef void (__VARED_glTexImage2D)(GLenum target, GLint level, GLint internalFormat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid *pixels); /*
*/ typedef void (__VARED_glTexParameteri)(GLenum target, GLenum pname, GLint param); /*
*/ typedef void (__VARED_glTexSubImage2D)(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *pixels); /*
*/ typedef void (__VARED_glUniform2f)(GLint location, GLfloat v0, GLfloat v1); /*
*/ typedef void (__VARED_glUseProgram)(GLuint program); /*
*/ typedef void (__VARED_glVertexAttribDivisor)(GLuint index, GLuint divisor); /*
*/ typedef void (__VARED_glVertexAttribPointer)(GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const void *pointer); /*
*/
/*
*/ global __VARED_glViewport *glViewport = 0; /*
*/ global __VARED_glAttachShader *glAttachShader = 0; /*
*/ global __VARED_glBindBuffer *glBindBuffer = 0; /*
*/ global __VARED_glBindTexture *glBindTexture = 0; /*
*/ global __VARED_glBindVertexArray *glBindVertexArray = 0; /*
*/ global __VARED_glBlendFunc *glBlendFunc = 0; /*
*/ global __VARED_glBufferData *glBufferData = 0; /*
*/ global __VARED_glBufferSubData *glBufferSubData = 0; /*
*/ global __VARED_glClear *glClear = 0; /*
*/ global __VARED_glClearColor *glClearColor = 0; /*
*/ global __VARED_glCompileShader *glCompileShader = 0; /*
*/ global __VARED_glCreateProgram *glCreateProgram = 0; /*
*/ global __VARED_glCreateShader *glCreateShader = 0; /*
*  M(GLDEBUGPROC, void, GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar *message, const void *userParam) *
*/ global __VARED_glDebugMessageCallback *glDebugMessageCallback = 0; /*
*/ global __VARED_glDeleteBuffers *glDeleteBuffers = 0; /*
*/ global __VARED_glDeleteShader *glDeleteShader = 0; /*
*/ global __VARED_glDrawArraysInstanced *glDrawArraysInstanced = 0; /*
*/ global __VARED_glEnable *glEnable = 0; /*
*/ global __VARED_glEnableVertexAttribArray *glEnableVertexAttribArray = 0; /*
*/ global __VARED_glGenBuffers *glGenBuffers = 0; /*
*/ global __VARED_glGenTextures *glGenTextures = 0; /*
*/ global __VARED_glGenVertexArrays *glGenVertexArrays = 0; /*
*/ global __VARED_glGetProgramInfoLog *glGetProgramInfoLog = 0; /*
*/ global __VARED_glGetProgramiv *glGetProgramiv = 0; /*
*/ global __VARED_glGetShaderInfoLog *glGetShaderInfoLog = 0; /*
*/ global __VARED_glGetShaderiv *glGetShaderiv = 0; /*
*/ global __VARED_glGetUniformLocation *glGetUniformLocation = 0; /*
*/ global __VARED_glLinkProgram *glLinkProgram = 0; /*
*/ global __VARED_glShaderSource *glShaderSource = 0; /*
*/ global __VARED_glTexImage2D *glTexImage2D = 0; /*
*/ global __VARED_glTexParameteri *glTexParameteri = 0; /*
*/ global __VARED_glTexSubImage2D *glTexSubImage2D = 0; /*
*/ global __VARED_glUniform2f *glUniform2f = 0; /*
*/ global __VARED_glUseProgram *glUseProgram = 0; /*
*/ global __VARED_glVertexAttribDivisor *glVertexAttribDivisor = 0; /*
*/ global __VARED_glVertexAttribPointer *glVertexAttribPointer = 0; /*
*/
