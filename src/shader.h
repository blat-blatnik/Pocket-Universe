#ifndef SHADER_H
#define SHADER_H

#include "glad.h"
#include <stdio.h>

typedef GLuint Shader;
typedef GLuint ComputeShader;
typedef GLuint GpuBuffer;

// Load, compile, and link an OpenGL shader program from the given vertex and fragment shader source files.
Shader loadShader(const char *vertSourceFile, const char *fragSourceFile);

// Load, compile, and link an OpenGL compute shader program from the given source code file.
ComputeShader loadComputeShader(const char *sourceFile);

#ifndef NDEBUG
// Check if any OpenGL errors have occured in previous GL calls.
#define glCheckErrors()\
	do {\
		GLenum code = glGetError();\
		if (code != GL_NO_ERROR) {\
			const char *desc;\
			switch (code) {\
				case GL_INVALID_ENUM:      desc = "Invalid Enum";      break;\
				case GL_INVALID_VALUE:     desc = "Invalid Value";     break;\
				case GL_INVALID_OPERATION: desc = "Invalid Operation"; break;\
				case GL_STACK_OVERFLOW:    desc = "Stack Overflow";    break;\
				case GL_STACK_UNDERFLOW:   desc = "Stack Underflow";   break;\
				case GL_OUT_OF_MEMORY:     desc = "Out of Memory";     break;\
				case GL_INVALID_FRAMEBUFFER_OPERATION: desc = "Invalid Framebuffer Operation"; break;\
				default: desc = "???"; break;\
			}\
			fprintf(stderr, "OpenGL ERROR %s in %s:%d (%s)\n", desc, __FILE__, (int)__LINE__, __func__);\
		}\
	} while (0)
#else
#define glCheckErrors() do {} while(0)
#endif // NDEBUG

#endif