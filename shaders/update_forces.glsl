#version 430

// Calculate the forces enacted on each particle and then update
// the velocity of the particles based on that force.
// This shader is run once for each tile and only processes the
// forces and velocities for the particles in that tile only.

layout (local_size_x=256) in;

struct TileList {
	int offset;
	int capacity;
	int size;
};

struct Particle {
	vec2 pos;
	vec2 vel;
	int type;
};

struct ParticleType {
	vec3 color;
};

struct ParticleInteraction {
	float attraction;
	float minRadius;
	float maxRadius;
};

layout(std140, binding=10) uniform UNIFORMS {
	ivec2 numTiles;
	float invTileSize;
	float deltaTime;
	vec2 size;
	vec2 center;
	float friction;
	float particleRadius;
	bool wrap;
};

layout(std430, binding=0) coherent restrict buffer TILE_LISTS {
	TileList tileLists[];
};

layout(std430, binding=1) restrict buffer NEW_PARTICLES {
	Particle particles[];
};

layout(std430, binding=3) restrict readonly buffer PARTICLE_TYPES {
	ParticleType particleTypes[];
};

layout(std430, binding=4) restrict readonly buffer PARTICLE_INTERACTIONS {
	ParticleInteraction interactions[];
};

vec2 calcForce(vec2 ppos, vec2 qpos, ParticleInteraction interaction) {
	vec2 dpos = qpos - ppos;
	if (wrap) {
		dpos += size * vec2(lessThan(dpos, -center));
		dpos -= size * vec2(greaterThan(dpos, center));
	}

	float r2 = dot(dpos, dpos);
	float minr = interaction.minRadius;
	float maxr = interaction.maxRadius;
	if (r2 > maxr * maxr || r2 < 0.001) {
		return vec2(0);
	}

	float r = sqrt(r2);
	if (r > minr) {
		return dpos / r * interaction.attraction * (min(abs(r - minr), abs(r - maxr)));
	} else {
		return -dpos * (minr - r) / (r * (0.5 + minr * r));
	}
}

shared TileList tileCache[3][3];
shared vec2 qPosCache[gl_WorkGroupSize.x];
shared int qTypeCache[gl_WorkGroupSize.x];
shared int numParticleTypes;

void main() {

	ivec2 tilePos = ivec2(gl_WorkGroupID.x, gl_WorkGroupID.y);
	int tileID = tilePos.y * numTiles.x + tilePos.x;
	tileLists[tileID].capacity = 0;

	if (gl_LocalInvocationID.x == 0)
		numParticleTypes = particleTypes.length();

	if (gl_LocalInvocationID.x < 9) {
		// 9 threads will load the data for the neighboring tiles in a 3x3
		// block and store it into the shared memory cache so that we don't
		// have to keep loading these from global memory.
		int dx = int(gl_LocalInvocationID.x) % 3;
		int dy = int(gl_LocalInvocationID.x) / 3;
		ivec2 neighborPos = tilePos + ivec2(dx - 1, dy - 1);
		// Wrap the tile position.
		neighborPos += numTiles * ivec2(lessThan(neighborPos, ivec2(0)));
		neighborPos -= numTiles * ivec2(greaterThanEqual(neighborPos, numTiles));
		int neighborID = clamp(neighborPos.y * numTiles.x + neighborPos.x, 0, tileLists.length() - 1);
		tileCache[dy][dx] = tileLists[neighborID];
	}

	memoryBarrierShared();
	barrier();

	// Assign a fraction of all particles to each thread.
	// We want to distribute the work as much as possible, so
	// each thread gets the same number of particles + the last 
	// couple of threads might get an extra particles if the total
	// number of threads doesn't evenly divide the number of particles.

	TileList tile = tileCache[1][1];
	int workSize = (tile.size + int(gl_LocalInvocationID.x)) / int(gl_WorkGroupSize.x);
	int workOffset = tile.offset + 
		(tile.size / int(gl_WorkGroupSize.x)) * int(gl_LocalInvocationID.x) +
		max(int(gl_LocalInvocationID.x) - int(gl_WorkGroupSize.x) + tile.size % int(gl_WorkGroupSize.x), 0);
	
	// Loop through all the 3x3 neighboring tiles and calculate the forces for each interaction.

	for (int dy = 0; dy < 3; ++dy) {
		for (int dx = 0; dx < 3; ++dx) {

			TileList neighbor = tileCache[dy][dx];

			// In order to avoid loading particles from the neighboring tile over and over from global memory
			// each thread loads a single particle from the neighboring tile into a local cache. Only the interactions
			// with the cached particles are processed, and then the next batch of neighboring particles is cached again.
			// There is a tradeoff here, because processing the neighbor particles in blocks like this means
			// we have to load the actual particles we are working with more often.

			// Loop until all neighboring particles were processed
			for (int qBase = 0; qBase < neighbor.size + int(gl_WorkGroupSize.x); qBase += int(gl_WorkGroupSize.x)) {

				// Load the neighboring particles into the cache.
				int qIdx = qBase + int(gl_LocalInvocationID.x);
				if (qIdx < neighbor.size) {
				
					Particle q = particles[neighbor.offset + qIdx];
					qPosCache[gl_LocalInvocationID.x] = q.pos;
					qTypeCache[gl_LocalInvocationID.x] = q.type;
				}
				memoryBarrierShared();
				barrier();

				// Calculate the particle interactions with the cached neighboring particles.
				int qidMax = min(int(gl_WorkGroupSize.x), neighbor.size - qBase);
				for (int address = workOffset; address < workOffset + workSize; ++address) {

					Particle p = particles[address];
					int pOffset = p.type * numParticleTypes;
					vec2 f = vec2(0);

					for (int qid = 0; qid < qidMax; ++qid) {
						ParticleInteraction interaction = interactions[pOffset + qTypeCache[qid]];
						f += calcForce(p.pos, qPosCache[qid], interaction);	
					}

					particles[address].vel += deltaTime * f;
				}
			}
		}
	}
}
