#version 430

// This shader does 2 things. First, it calculates the
// offset into the particle buffer at which each tile stores
// its particle list. Additionally, it clears the capacity
// and the size of the tile lists to 0. We only need the
// capacity of the lists in order to calculate all of the
// offsets, so once that is done we no longer need them until
// the next timestep. The sizes need to be reset to 0 so that
// we can properly sort the particles into tiles later.

layout (local_size_x=64) in;

struct TileList {
	int offset;
	int capacity;
	int size;
};

layout(std430, binding=0) restrict buffer TILE_LISTS {
	TileList tileLists[];
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

// Each thread processes a local block of 1/64 of all tile lists.
// The threads first process the local block by calculating the
// local capacity sum, and offsets of the tiles in this 1/64 tile
// fraction and storing these in the thread-shared cache. One of
// the threads then calculates a cumulative sum of the cached offsets.
// Each thread then calculates the global offset of the local fraction
// of tiles.

shared int blockSum[gl_WorkGroupSize.x];
shared int blockOffset[gl_WorkGroupSize.x];

void main() {

	// Assign a fraction of all tiles to each thread.
	// We want to distribute the work as much as possible, so
	// each thread gets the same number of tiles + the last 
	// couple of threads might get an extra tile of the total
	// number of threads doesn't evenly divide the number of tiles.

	int totalTiles = numTiles.x * numTiles.y;	
	int workSize = (totalTiles + int(gl_LocalInvocationID.x)) / int(gl_WorkGroupSize.x);
	int workOffset =
		(totalTiles / int(gl_WorkGroupSize.x)) * int(gl_LocalInvocationID.x) +
		max(int(gl_LocalInvocationID.x) - int(gl_WorkGroupSize.x) + totalTiles % int(gl_WorkGroupSize.x), 0);
	
	int id = int(gl_LocalInvocationID.x);
	int blockStart = workOffset;
	int blockEnd = workOffset + workSize;
	
	// Calculate the capacity sum and offsets for the whole local block.
	blockSum[id] = 0;
	for (int t = blockStart; t < blockEnd; ++t) {
		blockSum[id] += tileLists[t].capacity;
		tileLists[t].size = 0;
	}	
	memoryBarrierShared();
	barrier();
	
	// One thread does a cumulative sum of the block offsets.
	
	if (id == 0) {
		blockOffset[0] = 0;
		for (int b = 1; b < gl_WorkGroupSize.x; ++b)
			blockOffset[b] = blockOffset[b - 1] + blockSum[b - 1];
	}
	memoryBarrierShared();
	barrier();
	
	// Calculate the global offset for the local block and reset the capacity to 0.
	
	tileLists[blockStart].offset = blockOffset[id];
	for (int t = blockStart + 1; t < blockEnd; ++t) {
		tileLists[t].offset = tileLists[t - 1].offset + tileLists[t - 1].capacity;
		tileLists[t - 1].capacity = 0;
	}
	tileLists[blockEnd - 1].capacity = 0;
}