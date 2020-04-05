#include "math.h"
#include <stdint.h>
#include <limits.h>

// Construct a 3D vector from 3 floats.
static vec3 v3(float x, float y, float z) {
	vec3 v;
	v.x = x;
	v.y = y;
	v.z = z;
	return v;
}

vec3 HSV(float h, float s, float v) {
	int i = (int)(h * 6.0f);
	float f = h * 6.0f - i;
	float p = v * (1.0f - s);
	float q = v * (1.0f - f * s);
	float t = v * (1.0f - (1.0f - f) * s);
	switch (i % 6) {
		case 0:  return v3(v, t, p);
		case 1:  return v3(q, v, p);
		case 2:  return v3(p, v, t);
		case 3:  return v3(p, q, v);
		case 4:  return v3(t, p, v);
		default: return v3(v, p, q);
	}
}

uint32_t randu(RNG *rng) {
	uint64_t x = *rng;
	uint32_t count = (uint32_t)(x >> 61);
	*rng = x * 6364136223846793005u;
	x ^= x >> 22;
	return (uint32_t)(x >> (22 + count));
}

int randi(RNG *rng, int min, int max) {
	uint32_t x = randu(rng);
	uint64_t m = (uint64_t)x * (uint64_t)(max - min);
	return min + (int)(m >> 32);
}

float randUniform(RNG *rng, float min, float max) {
	float f = randu(rng) / (float)UINT_MAX;
	return min + f * (max - min);
}

float randGaussian(RNG *rng, float mean, float stddev) {
	float u, v, s;
	do {
		u = randUniform(rng, -1, 1);
		v = randUniform(rng, -1, 1);
		s = u * u + v * v;
	} while (s >= 1.0 || s == 0.0);
	s = sqrtf(-2 * logf(s) / s);
	return mean + stddev * u * s;
}

RNG seedRNG(uint64_t seed) {
	RNG rng = 2 * seed + 1;
	randu(&rng);
	return rng;
}
