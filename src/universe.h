#ifndef UNIVERSE_H
#define UNIVERSE_H

#include "math.h"
#include "glad.h"
#include "shader.h"

typedef struct Particle {
	vec2 pos;  // position
	vec2 vel;  // velocity
	int  type; // index into the particle type array
	
	// This struct needs to be aligned on a vec2 sized boundary on the GPU
	// because of std430 buffer layout rules. So we need to add some padding.
	// See: https://www.khronos.org/registry/OpenGL/specs/gl/glspec43.core.pdf#page=146
	
	int  padding[1];
} Particle;

typedef struct ParticleType {
	vec3 color;

	// This struct needs to be aligned on a vec4 sized boundary on the GPU
	// because of std430 buffer layout rules. So we need to add some padding.
	// See: https://www.khronos.org/registry/OpenGL/specs/gl/glspec43.core.pdf#page=146

	int  padding[1];
} ParticleType;

typedef struct ParticleInteraction {
	float attraction; // how strongly the particles attract each other (can be negative)
	float minRadius;  // the minimum distance from which the particles attract each other
	float maxRadius;  // the maximum distance from which the particles attract each other
} ParticleInteraction;

typedef struct Universe {
	int numParticles;
	int numParticleTypes;
	Particle *particles;
	ParticleType *particleTypes;
	ParticleInteraction *interactions;
	float width;
	float height;
	float friction;
	float deltaTime;
	float particleRadius;
	int wrap;
	int meshDetail;
	RNG rng;

	struct UniverseInternal {
		int numTilesX;
		int numTilesY;
		float invTileSize;

		Shader particleShader;
		ComputeShader setupTiles;
		ComputeShader sortParticles;
		ComputeShader updateForces;
		ComputeShader updatePositions;

		GLuint particleVertexArray1;
		GLuint particleVertexArray2;
		GpuBuffer particleVertexBuffer;
		GpuBuffer gpuTileLists;
		GpuBuffer gpuNewParticles;
		GpuBuffer gpuOldParticles;
		GpuBuffer gpuParticleTypes;
		GpuBuffer gpuInteractions;
		GpuBuffer gpuUniforms;
	} internal;
} Universe;

Universe createUniverse(int numParticleTypes, int numParticles, float width, float height);
void destroyUniverse(Universe *u);
ParticleInteraction *getInteraction(Universe *u, int type1, int type2);
void randomize(Universe *u, float attractionMean, float attractionStddev, float minRadius0, float minRadius1, float maxRadius0, float maxRadius1);
void updateBuffers(Universe *u);
void simulateTimestep(Universe *u);
void draw(Universe *u);
void printParams(Universe *u);

#endif