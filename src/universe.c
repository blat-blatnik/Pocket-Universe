#define GLAD_IMPLEMENTATION
#include "universe.h"
#include <stddef.h>
#include <time.h>

ParticleInteraction *getInteraction(Universe *u, int type1, int type2) {
	return &u->interactions[type1 * u->numParticleTypes + type2];
}

Universe createUniverse(int numParticleTypes, int numParticles, float width, float height) {

	Universe u;

	u.rng = seedRNG((uint64_t)time(NULL));
	u.numParticles = numParticles;
	u.numParticleTypes = numParticleTypes;
	u.particles = (Particle *)malloc(u.numParticles * sizeof(Particle));
	u.particleTypes = (ParticleType *)malloc(u.numParticleTypes * sizeof(ParticleType));
	u.interactions = (ParticleInteraction *)malloc(u.numParticleTypes * u.numParticleTypes * sizeof(ParticleInteraction));

	u.width = width;
	u.height = height;
	u.wrap = GL_TRUE;
	u.particleRadius = 5;
	u.meshDetail = 8;

	struct UniverseInternal *ui = &u.internal;

	// Load the shaders.
	ui->particleShader  = loadShader("shaders/vert.glsl", "shaders/frag.glsl");
	ui->setupTiles      = loadComputeShader("shaders/setup_tiles.glsl");
	ui->sortParticles   = loadComputeShader("shaders/sort_particles.glsl");
	ui->updateForces    = loadComputeShader("shaders/update_forces.glsl");
	ui->updatePositions = loadComputeShader("shaders/update_positions.glsl");

	// Generate and bind all of the GPU buffers.
	glGenBuffers(1, &ui->gpuUniforms);
	glGenBuffers(1, &ui->gpuTileLists);
	glGenBuffers(1, &ui->gpuNewParticles);
	glGenBuffers(1, &ui->gpuOldParticles);
	glGenBuffers(1, &ui->gpuParticleTypes);
	glGenBuffers(1, &ui->gpuInteractions);	
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, ui->gpuTileLists);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, ui->gpuNewParticles);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, ui->gpuOldParticles);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, ui->gpuParticleTypes);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, ui->gpuInteractions);
	glBindBufferBase(GL_UNIFORM_BUFFER, 10, ui->gpuUniforms);

	// Initialize circle mesh for the particles.
	const float twoPi = 2 * PI;
	const float limit = twoPi + 0.001f;
	int num_coords = u.meshDetail + 2;
	vec2 *coords = (vec2 *)malloc(num_coords * sizeof(vec2));
	// Generate a circular mesh with the given detail.
	coords[0].x = 0;
	coords[0].y = 0;
	int i = 1;
	for (float x = 0; x <= limit; x += twoPi / u.meshDetail) {
		coords[i].x = cosf(x);
		coords[i].y = sinf(x);
		++i;
	}
	glGenBuffers(1, &ui->particleVertexBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, ui->particleVertexBuffer);
	glBufferData(GL_ARRAY_BUFFER, num_coords * sizeof(vec2), coords, GL_STATIC_DRAW);
	free(coords);

	// Initialize VAOs.
	glGenVertexArrays(1, &ui->particleVertexArray1);
	glGenVertexArrays(1, &ui->particleVertexArray2);
	glBindVertexArray(ui->particleVertexArray1);
	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);
	glEnableVertexAttribArray(2);
	glBindBuffer(GL_ARRAY_BUFFER, ui->particleVertexBuffer);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), 0);
	glBindBuffer(GL_ARRAY_BUFFER, ui->gpuNewParticles);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Particle), (void *)offsetof(Particle, pos));
	glVertexAttribDivisor(1, 1);
	glVertexAttribIPointer(2, 1, GL_INT, sizeof(Particle), (void *)offsetof(Particle, type));
	glVertexAttribDivisor(2, 1);
	glBindVertexArray(ui->particleVertexArray2);
	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);
	glEnableVertexAttribArray(2);
	glBindBuffer(GL_ARRAY_BUFFER, ui->particleVertexBuffer);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), 0);
	glBindBuffer(GL_ARRAY_BUFFER, ui->gpuOldParticles);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Particle), (void *)offsetof(Particle, pos));
	glVertexAttribDivisor(1, 1);
	glVertexAttribIPointer(2, 1, GL_INT, sizeof(Particle), (void *)offsetof(Particle, type));
	glVertexAttribDivisor(2, 1);
	glBindVertexArray(0);

	return u;
}

void destroyUniverse(Universe *u) {
	free(u->particles);
	free(u->particleTypes);
	free(u->interactions);

	struct UniverseInternal *ui = &u->internal;

	glDeleteProgram(ui->particleShader);
	glDeleteProgram(ui->setupTiles);
	glDeleteProgram(ui->sortParticles);
	glDeleteProgram(ui->updateForces);
	glDeleteProgram(ui->updatePositions);

	glDeleteVertexArrays(1, &ui->particleVertexArray1);
	glDeleteVertexArrays(1, &ui->particleVertexArray2);
	glDeleteBuffers(1, &ui->particleVertexBuffer);
	glDeleteBuffers(1, &ui->gpuTileLists);
	glDeleteBuffers(1, &ui->gpuNewParticles);
	glDeleteBuffers(1, &ui->gpuOldParticles);
	glDeleteBuffers(1, &ui->gpuParticleTypes);
	glDeleteBuffers(1, &ui->gpuInteractions);
	glDeleteBuffers(1, &ui->gpuUniforms);

	glCheckErrors();
	memset(u, 0, sizeof(*u));
}

void updateBuffers(Universe *u) {

	struct UniverseInternal *ui = &u->internal;

	// Recalculate the tile sizes.

	float tileSize = 0;
	for (int i = 0; i < u->numParticleTypes; ++i)
		for (int j = 0; j < u->numParticleTypes; ++j)
			tileSize = fmaxf(tileSize, getInteraction(u, i, j)->maxRadius);

	ui->invTileSize = 1 / tileSize;
	ui->numTilesX = (int)ceilf(u->width / tileSize);
	ui->numTilesY = (int)ceilf(u->height / tileSize);
	int numTiles = ui->numTilesX * ui->numTilesY;

	struct TileList {
		int offset;
		int capacity;
		int size;
	} *tileLists = (struct TileList *)calloc(numTiles, sizeof(*tileLists));
	
	// On the initial run of the shaders the capacity of the tile lists needs to be calculated.
	// This is why we have to do it here. After the first timestep we no longer have to do this
	// here and the shaders will take care of it.

	for (int pID = 0; pID < u->numParticles; ++pID) {
		Particle p = u->particles[pID];
		int gridID = (int)(p.pos.y * ui->invTileSize) * ui->numTilesX + (int)(p.pos.x * ui->invTileSize);
		if (gridID < 0) gridID = 0;
		if (gridID >= numTiles) gridID = numTiles - 1;
		tileLists[gridID].capacity++;
	}

	glBindBuffer(GL_SHADER_STORAGE_BUFFER, ui->gpuTileLists);
	glBufferData(GL_SHADER_STORAGE_BUFFER, numTiles * sizeof(*tileLists), tileLists, GL_STREAM_COPY);
	free(tileLists);

	glBindBuffer(GL_SHADER_STORAGE_BUFFER, ui->gpuNewParticles);
	glBufferData(GL_SHADER_STORAGE_BUFFER, u->numParticles * sizeof(Particle), u->particles, GL_STREAM_COPY);

	glBindBuffer(GL_SHADER_STORAGE_BUFFER, ui->gpuOldParticles);
	glBufferData(GL_SHADER_STORAGE_BUFFER, u->numParticles * sizeof(Particle), u->particles, GL_STREAM_COPY);

	glBindBuffer(GL_SHADER_STORAGE_BUFFER, ui->gpuParticleTypes);
	glBufferData(GL_SHADER_STORAGE_BUFFER, u->numParticleTypes * sizeof(ParticleType), u->particleTypes, GL_STATIC_DRAW);
	
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, ui->gpuInteractions);
	glBufferData(GL_SHADER_STORAGE_BUFFER, u->numParticleTypes * u->numParticleTypes * sizeof(ParticleInteraction), u->interactions, GL_STATIC_DRAW);
}

void simulateTimestep(Universe *u) {

	struct UniverseInternal *ui = &u->internal;

	struct {
		int numTilesX;
		int numTilesY;
		float invTileSize;
		float deltaTime;
		float width;
		float height;
		float centerX;
		float centerY;
		float friction;
		float particleRadius;
		int wrap;
	} uniforms;

	uniforms.numTilesX = ui->numTilesX;
	uniforms.numTilesY = ui->numTilesY;
	uniforms.invTileSize = ui->invTileSize;
	uniforms.deltaTime = u->deltaTime;
	uniforms.width = u->width;
	uniforms.height = u->height;
	uniforms.centerX = u->width / 2;
	uniforms.centerY = u->height / 2;
	uniforms.friction = u->friction;
	uniforms.particleRadius = u->particleRadius;
	uniforms.wrap = u->wrap;
	glBindBuffer(GL_UNIFORM_BUFFER, ui->gpuUniforms);
	glBufferData(GL_UNIFORM_BUFFER, sizeof(uniforms), &uniforms, GL_STREAM_DRAW);

	// The particle data is actually double buffered on the GPU between timesteps.
	// During the shader pipeline the particles from the back buffer are copied over
	// to the front buffer sorted in order of which tile they belong to. This improves
	// the memory caching behavior when we calculate particle interactions. The code
	// below switches the front/new buffer from the previous timestep to be the back/old
	// buffer in this timestep. A similar thing has to happen with the VAO so that we
	// always render the particles in the front buffer.
	// As a side note, because we re-order the particles every timestep the particles
	// will appear to flicker when we render them. This is because they will be essentially
	// drawn in random order each frame and so one frame, particle 1 might completely cover
	// up particle 2, but the next frame particle 2 might cover up particle 1, which
	// causes them to flicker. This could be fixed by not using a double-buffer like we
	// do here and simply storing a list of indices for each tile, but that would
	// introduce a memory indirection and lower performance by a pretty significant factor
	// (I tried it).

	GpuBuffer temp = ui->gpuNewParticles;
	ui->gpuNewParticles = ui->gpuOldParticles;
	ui->gpuOldParticles = temp;
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, ui->gpuNewParticles);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, ui->gpuOldParticles);

	glUseProgram(ui->setupTiles);
	glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
	glDispatchCompute(1, 1, 1);

	glUseProgram(ui->sortParticles);
	glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
	glDispatchCompute((int)ceil(u->numParticles / (1.0 * 256.0)), 1, 1);

	glUseProgram(ui->updateForces);
	glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
	glDispatchCompute(ui->numTilesX, ui->numTilesY, 1);

	glUseProgram(ui->updatePositions);
	glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
	glDispatchCompute((int)ceil(u->numParticles / (1.0 * 256.0)), 1, 1);

	GLuint tempa = ui->particleVertexArray1;
	ui->particleVertexArray1 = ui->particleVertexArray2;
	ui->particleVertexArray2 = tempa;
}

void draw(Universe *u) {
	struct UniverseInternal *ui = &u->internal;

	// The code commented out below needs to be uncommented if
	// you are drawing the universe without simulating a timestep first.
	// So if you are doing something like:
	//
	//  while (..) {
	//    simulateTimestep(&u);
	//    ..
	//    draw(&u);
	//  }
	//
	// Then this should stay commented out.
	//
	//struct {
	//	int numTilesX;
	//	int numTilesY;
	//	float invTileSize;
	//	float deltaTime;
	//	float width;
	//	float height;
	//	float centerX;
	//	float centerY;
	//	float friction;
	//	float particleRadius;
	//	int wrap;
	//} uniforms;
	//
	//uniforms.numTilesX = ui->numTilesX;
	//uniforms.numTilesY = ui->numTilesY;
	//uniforms.invTileSize = ui->invTileSize;
	//uniforms.deltaTime = u->deltaTime;
	//uniforms.width = u->width;
	//uniforms.height = u->height;
	//uniforms.centerX = u->width / 2;
	//uniforms.centerY = u->height / 2;
	//uniforms.friction = u->friction;
	//uniforms.particleRadius = u->particleRadius;
	//uniforms.wrap = u->wrap;
	//glBindBuffer(GL_UNIFORM_BUFFER, ui->gpuUniforms);
	//glBufferData(GL_UNIFORM_BUFFER, sizeof(uniforms), &uniforms, GL_STREAM_DRAW);

	glUseProgram(ui->particleShader);
	glBindVertexArray(ui->particleVertexArray2);
	glMemoryBarrier(GL_VERTEX_ATTRIB_ARRAY_BARRIER_BIT);
	glDrawArraysInstanced(GL_TRIANGLE_FAN, 0, u->meshDetail + 2, (int)u->numParticles);
	glBindVertexArray(0);
}

void randomize(Universe *u, float attractionMean, float attractionStddev, float minRadius0, float minRadius1, float maxRadius0, float maxRadius1) {

	const float diamater = 2 * u->particleRadius;

	for (int i = 0; i < u->numParticleTypes; ++i)
		u->particleTypes[i].color = HSV((float)i / u->numParticleTypes, 1, (float)(i & 1) * 0.5f + 0.5f);

	for (int i = 0; i < u->numParticleTypes; ++i) {
		for (int j = 0; j < u->numParticleTypes; ++j) {		
			ParticleInteraction *interaction = getInteraction(u, i, j);
			
			if (i == j) {
				interaction->attraction = -fabsf(randGaussian(&u->rng, attractionMean, attractionStddev));
				interaction->minRadius = diamater;
			} else {
				interaction->attraction = randGaussian(&u->rng, attractionMean, attractionStddev);
				interaction->minRadius = fmaxf(randUniform(&u->rng, minRadius0, minRadius1), diamater);
			}
			
			interaction->maxRadius = fmaxf(randUniform(&u->rng, maxRadius0, maxRadius1), interaction->minRadius);

			// Keep radii symmetric
			getInteraction(u, j, i)->maxRadius = getInteraction(u, i, j)->maxRadius;
			getInteraction(u, j, i)->minRadius = getInteraction(u, i, j)->minRadius;

			interaction->attraction = 2 * interaction->attraction / (interaction->maxRadius - interaction->minRadius);
		}
	}

	for (int i = 0; i < u->numParticles; ++i) {
		Particle *p = &u->particles[i];
		p->type = randi(&u->rng, 0, u->numParticleTypes);
		p->pos.x = randUniform(&u->rng, 0, u->width);
		p->pos.y = randUniform(&u->rng, 0, u->height);
		p->vel.x = randGaussian(&u->rng, 0, 1);
		p->vel.y = randGaussian(&u->rng, 0, 1);
	}

	updateBuffers(u);
}

void printParams(Universe *u) {
	printf("Attract:\n");
	for (int i = 0; i < u->numParticleTypes; ++i) {
		for (int j = 0; j < u->numParticleTypes; ++j) {
			ParticleInteraction *interaction = getInteraction(u, i, j);
			printf("%g ", interaction->attraction / 2 * (interaction->minRadius - interaction->maxRadius));
		}
		printf("\n");
	}

	printf("MinR:\n");
	for (int i = 0; i < u->numParticleTypes; ++i) {
		for (int j = 0; j < u->numParticleTypes; ++j) {
			ParticleInteraction *interaction = getInteraction(u, i, j);
			printf("%g ", interaction->minRadius);
		}
		printf("\n");
	}

	printf("MaxR:\n");
	for (int i = 0; i < u->numParticleTypes; ++i) {
		for (int j = 0; j < u->numParticleTypes; ++j) {
			ParticleInteraction *interaction = getInteraction(u, i, j);
			printf("%g ", interaction->maxRadius);
		}
		printf("\n");
	}
}
