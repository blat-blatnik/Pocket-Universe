#version 430

// This compute shader updates the positions of each particle
// and also sorts the particles into the tiles for the next frame
// by updating the tile capacities.

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

layout(std430, binding=0) coherent restrict buffer TILE_LISTS {
	TileList tileLists[];
};

layout(std430, binding=1) restrict buffer NEW_PARTICLES {
	Particle particles[];
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

void updateParticle(inout Particle p) {
	p.pos += p.vel * deltaTime;
	p.vel *= pow(1.0 - friction, deltaTime);

	if (wrap) {
		p.pos -= size * ivec2(greaterThanEqual(p.pos, size));
		p.pos += size * ivec2(lessThan(p.pos, vec2(0)));
	} else {
		float particleDiamater = 2.0 * particleRadius;
		vec2 minPos = vec2(particleDiamater);
		vec2 maxPos = size - vec2(particleDiamater);
		bvec2 less = lessThanEqual(p.pos, minPos);
		bvec2 greater = greaterThanEqual(p.pos, maxPos);
		bvec2 mask = bvec2(ivec2(less) | ivec2(greater));
		p.vel *= mix(vec2(1.0), vec2(-1.0), mask);
		p.pos = clamp(p.pos, minPos, maxPos);
	}
}

void main() {

	// Each global thread ID corresponds to a single particle.
	int id = int(gl_GlobalInvocationID.x);
	if (id >= particles.length())
		return;
		
	Particle p = particles[id];
	updateParticle(p);
	particles[id] = p;
	
	// Get which tile this particle belongs to.
	ivec2 tilePos = ivec2(p.pos * invTileSize);
	int tileID = clamp(tilePos.y * numTiles.x + tilePos.x, 0, tileLists.length() - 1);
	atomicAdd(tileLists[tileID].capacity, 1);
	memoryBarrier(); // <<--- is this necessary for atomics and coherent buffer???
}