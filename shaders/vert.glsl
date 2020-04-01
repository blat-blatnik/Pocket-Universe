#version 430

struct ParticleType {
	vec3 color;
};

layout(location = 0) in vec2 inPos;
layout(location = 1) in vec2 inOffset;
layout(location = 2) in int  inType;

out vec4 vertColor;

layout(std430, binding=3) readonly buffer PARTICLE_TYPES {
	ParticleType particleTypes[];
};

layout(std140, binding=10) uniform UNIFORMS {
	ivec2 numTiles;
	float invTileSize;
	float deltaTime;
	vec2  size;
	vec2  center;
	float friction;
	float particleRadius;
	bool  wrap;
};

void main() {
	// Add some transparency based on distance from center.
	float alpha = 1.0 - dot(inPos, inPos);
	vertColor = vec4(particleTypes[inType].color, alpha);

	// Output normalized device coordinates.
	vec2 pos = inPos * particleRadius + inOffset;
	pos = (pos / center) - 1.0;
	gl_Position = vec4(pos, 0.0, 1.0);
}