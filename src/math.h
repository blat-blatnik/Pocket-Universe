#ifndef MATH_H
#define MATH_H

#include <stdint.h>
#include <math.h>

#define PI 3.141592741f

// 2D vector
typedef struct vec2 {
	float x, y;
} vec2;

// 3D vector
typedef struct vec3 {
	float x, y, z;
} vec3;

// 4D vector
typedef struct vec4 {
	float x, y, z, w;
} vec4;

// Converts from HSV to RGB color-space.
// All inputs expected to be in [0..1].
vec3 HSV(float h, float s, float v);

// We use a PCG random number generator. https://www.pcg-random.org/index.html
typedef uint64_t RNG;

// Seed a new random number generator.
RNG seedRNG(uint64_t seed);

// Generate a uniform unsigned integer in [0, UINT_MAX].
uint32_t randu(RNG *rng);

// Generate a uniform integer in [min, max).
int randi(RNG *rng, int min, int max);

// Generate a uniform float in [min, max).
float randUniform(RNG *rng, float min, float max);

// Generate a gaussian float in [min, max).
float randGaussian(RNG *rng, float mean, float stddev);

#endif