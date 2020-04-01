#include "shader.h"
#include <stdlib.h>

static char *readEntireFile(const char *filename) {
	FILE *f = fopen(filename, "rb");
	if (!f) return NULL;

	fseek(f, 0, SEEK_END);
	size_t fsize = (size_t)ftell(f);
	fseek(f, 0, SEEK_SET);

	char *string = (char*)malloc(fsize + 1);
	if (!string) return NULL;
	
	fread(string, 1, fsize, f);
	fclose(f);
	string[fsize] = 0;
	return string;
}

static GLuint loadShaderComponent(GLenum type, const char *sourceFile) {
	char* source = readEntireFile(sourceFile);
	if (!source) {
		fprintf(stderr, "ERROR: failed to read shader file %s\n", sourceFile);
		return 0;
	}

	const GLchar *glSource = (GLchar *)source;
	GLuint shader = glCreateShader(type);
	glShaderSource(shader, 1, &glSource, NULL);
	glCompileShader(shader);
	free(source);

	GLint compileOk;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &compileOk);
	if (!compileOk) {
	#ifndef NDEBUG
		GLint logLength;
		glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logLength);
		char *log = (char *)malloc((size_t)logLength);
		glGetShaderInfoLog(shader, logLength, NULL, (GLchar *)log);
		fprintf(stderr, "GLSL compile ERROR: %s\n", log);
		free(log);
	#endif
		glDeleteShader(shader);
		return 0;
	}

	return shader;
}

static GLboolean linkShaderProgram(Shader program) {
	glLinkProgram(program);
	GLint linkOk;
	glGetProgramiv(program, GL_LINK_STATUS, &linkOk);
	if (!linkOk) {
	#ifndef NDEBUG
		GLint logLength;
		glGetProgramiv(program, GL_INFO_LOG_LENGTH, &logLength);
		char *log = (char *)malloc((size_t)logLength);
		glGetProgramInfoLog(program, logLength, NULL, (GLchar *)log);
		fprintf(stderr, "GLSL link ERROR: %s\n", log);
		free(log);
	#endif
		return GL_FALSE;
	}

	return GL_TRUE;
}

Shader loadShader(const char *vertSourceFile, const char *fragSourceFile) {
	GLuint vert = loadShaderComponent(GL_VERTEX_SHADER, vertSourceFile);
	GLuint frag = loadShaderComponent(GL_FRAGMENT_SHADER, fragSourceFile);

	if (vert == 0 || frag == 0) {
		glDeleteShader(vert);
		glDeleteShader(frag);
		return 0;
	}

	Shader program = glCreateProgram();
	glAttachShader(program, vert);
	glAttachShader(program, frag);

	GLboolean linkOk = linkShaderProgram(program);
	if (!linkOk) {
		glDeleteProgram(program);
		return 0;
	}

	glDetachShader(program, vert);
	glDetachShader(program, frag);
	glDeleteShader(vert);
	glDeleteShader(frag);
	return program;
}

ComputeShader loadComputeShader(const char *sourceFile) {
	GLuint compute = loadShaderComponent(GL_COMPUTE_SHADER, sourceFile);
	
	if (compute == 0) {
		glDeleteShader(compute);
		return 0;
	}

	ComputeShader program = glCreateProgram();
	glAttachShader(program, compute);

	GLboolean linkOk = linkShaderProgram(program);
	if (!linkOk) {
		glDeleteProgram(program);
		return 0;
	}

	glDetachShader(program, compute);
	glDeleteShader(compute);
	return program;
}