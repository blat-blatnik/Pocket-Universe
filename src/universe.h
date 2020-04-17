#ifndef UNIVERSE_H
#define UNIVERSE_H

#include "math.h"
#include "glad.h"
#include "shader.h"

typedef struct Particle {
	vec2 pos;  /* position */
	vec2 vel;  /* velocity */
	int  type; /* index into the particle type array */
	
	/* This struct needs to be aligned on a vec2 sized boundary on the GPU
	   because of std430 buffer layout rules. So we need to add some padding.
	   See: https://www.khronos.org/registry/OpenGL/specs/gl/glspec43.core.pdf#page=146 */
	
	int  padding[1];
} Particle;

typedef struct ParticleType {
	vec3 color;

	/* This struct needs to be aligned on a vec4 sized boundary on the GPU
	   because of std430 buffer layout rules. So we need to add some padding.
	   See: https://www.khronos.org/registry/OpenGL/specs/gl/glspec43.core.pdf#page=146 */

	int  padding[1];
} ParticleType;

typedef struct ParticleInteraction {
	float attraction; /* how strongly the particles attract each other (can be negative)  */
	float minRadius;  /* the minimum distance from which the particles attract each other */
	float maxRadius;  /* the maximum distance from which the particles attract each other */

	/* Note that if you want to manually set attraction it has to be corrected like so:
	   
	   attraction = 2 * attraction / (maxRadius - minRadius)
	   
	   this correction would normally be applied every frame but if you pre-calculate it
	   like this we can avoid doing an expensive floating-point division and save some perf. */

} ParticleInteraction;

typedef struct Universe {

	int numParticles;
	int numParticleTypes;
	Particle *particles;
	ParticleType *particleTypes;
	ParticleInteraction *interactions;
	float width;          /* should be positive */
	float height;         /* should be positive */
	float friction;       /* should be between 0 and 1 */
	float deltaTime;      /* should be positive or 0 */
	float particleRadius; /* should be positive or 0 */
	int wrap;             /* should be either 0 or 1 */
	int meshDetail;       /* should be positive */
	RNG rng;              /* you can set this with seedRNG() */

	/* This stores data which should not be modified - unless you know what you're doing.. */
	struct UniverseInternal {
		int numTilesX;
		int numTilesY;
		float invTileSize; /* stores the inverse of the tile size so we don't have to divide */

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

/* Create a new universe with the given characteristics. */
Universe createUniverse(int numParticleTypes, int numParticles, float width, float height);

/* Destroy all the resources used by the universe. */
void destroyUniverse(Universe *u);

/* Get a pointer to the interaction of one particle type with another. */
ParticleInteraction *getInteraction(Universe *u, int type1, int type2);

/* Randomize the universe with the given parameters.
   You can control the RNG used by setting the universes .rng field before calling randomize. */
void randomize(Universe *u, float attractionMean, float attractionStddev, float minRadius0, float minRadius1, float maxRadius0, float maxRadius1);

/* This function sends the universe data to the GPU and it has to be called 
   whenever particles, particle types, or interactions are changed. */
void updateBuffers(Universe *u);

/* Simulate a single timestep on the GPU. */
void simulateTimestep(Universe *u);

/* Render the universe. */
void draw(Universe *u);

/* Print the parameters of the universe for reproducability. */
void printParams(Universe *u);

#endif