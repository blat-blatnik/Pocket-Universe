#version 430

// This compute shader sorts all of the particles into tiles
// based on the position of the particle. The particles are moved
// from the "old" particle buffer (back-buffer) to the "new" 
// particle buffer (front-buffer).

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

layout(std430, binding=1) restrict writeonly buffer NEW_PARTICLES {
	Particle newParticles[];
};

layout(std430, binding=2) restrict readonly buffer OLD_PARTICLES {
	Particle oldParticles[];
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

void main() {

	// Each global thread ID corresponds to a single particle.
	int id = int(gl_GlobalInvocationID.x);
	if (id >= oldParticles.length())
		return;
		
	Particle p = oldParticles[id];
	
	// Get which tile this particle belongs to.
	ivec2 tilePos = ivec2(p.pos * invTileSize);
	int tileID = clamp(tilePos.y * numTiles.x + tilePos.x, 0, tileLists.length() - 1);
	
	// Place the particle in its tile.
	int address = atomicAdd(tileLists[tileID].size, 1);
	memoryBarrier(); // <<--- Is this necessary???
	newParticles[tileLists[tileID].offset + address] = p;
}