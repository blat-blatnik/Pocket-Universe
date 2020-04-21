![](./screenshots/particles2.gif)

# Pocket-Universe

A particle simulation running in parallel on the GPU using compute shaders. In this system the particles can attract and repel each other in a small radius and it looks astounding and very life-like when running in real time. I optimized it as much as I could. The current system can simulate around 100'000-200'000 particles in real-time (30fps) on a modern GPU. 

![](/screenshots/1.png)

## Particle Game of Life

This project was inspired by [CodeParade](https://www.youtube.com/channel/UCrv269YwJzuZL3dH5PCgxUw)'s [Particle Life](https://youtu.be/Z_zmZ23grXE) simulation. In this simulation the world consists of a number of differently colored particles. These particles can can be attracted - or repelled - by particles of different colors. For example, _blue_ particles might be attracted to _red_ particles, while _red_ particles might be repelled by _green_ particles, etc. A major difference between Particle Life and the [Game of Life](https://en.wikipedia.org/wiki/Conway%27s_Game_of_Life) is that in Particle Life, the particles can occupy any position in space, not just integer grid positions.

Each particle only attracts or repells other particles that are within some _maximum radius_. If two particles come really really close together they will always start strongly repelling in order to avoid occupying the same space. There is also _friction_ in the system, meaning the particles lose a proportion of their velocity each second - because of this the particles tend to fall into mutually stable arrangements with other particles.

If you want more details you should check out [the video](https://youtu.be/Z_zmZ23grXE) by [CodeParade](https://www.youtube.com/channel/UCrv269YwJzuZL3dH5PCgxUw). My goal with this project was to optimize this simulation so that it could run a very large number of particles. I did so by implementing Particle Life in compute shaders running massively in parallel on the GPU, rendered in real-time.

![](/screenshots/4.png)

## Optimizations

Since each particle can interact with every other particle the simulation algorithm is inherently an O(n<sup>2</sup>) algorithm, where _n_ denotes the number of particle in the simulation. In fact this simulation is very similar to [N-body simulations](https://en.wikipedia.org/wiki/N-body_simulation), except in 2D. A naive implementation of the Particle Life simulation would be something like this:

```python
for p in particles:
  for q in particles:
    p.velocity += interact(p, q)

for p in particles:
  p.position += p.velocity
```

This naive implementation runs very poorly. On a CPU this can barely simulate 1'000 particles in real-time (single-threaded), and on a GPU it cannot simulate more than around 10'000 particles.  However, we can do much better.

### Tiling

we can use the fact that particles only interact with particles within a strictly defined _maximum radius_, and divide the simulated world up into _tiles_. We can then sort the particles into these tiles based on their position in the world, and only interact them with other particles from neighboring tiles. As long as we make the size of the tiles equal to the maximum interaction distance, the simulation will still end up being 100% correct. An implementation using this approach would look something like this:

```python
for tile in tiles:
  clear(tile)

for p in particles:
  tile = tiles[floor(p.position / tiles.count)]
  add(p, tile.particles)
  
for p in particles:
  tile = tiles[floor(p.position / tiles.count)]
  for n in neighbors(tile)
    for q in n.particles
      p.velocity += interact(p, q)

for p in particles:
  p.position += p.velocity
```

This reduces the algorithmic complexity of the simulation from O(n<sup>2</sup>) to O(nt), where _t_ denotes the largest number of particles that belongs to any tile. Since the particles tend to stay somewhat spread out _t_ will usually be way smaller than _n_, this is a big performance win. An implementation on the CPU can now simulate 8'000 particles (single-threaded), and a GPU implementation can simulate around 40'000 particles.

![](/screenshots/2.png)

After each timestep, we want to render the particles to the screen, and so a major downside of the CPU implementation is that we will eventually have to send over the particle positions to the GPU _every timestep_. Even if this isn't a concern right now, it will eventually become the bottleneck as more and more particle positions have to be sent over. For this reason we should focus optimizations on the GPU implementation only.

While the psudocode above nicely _outlines_ the tiling algorithm, it hides one major concern, which is that `tile.particles` cannot simply be implemented as a dynamic array if we want to run on the GPU. GPU's have no support for such things. A very naive approach to solving this would be to make every tile's list big enough to fit _all_ particles. However this would obviously leave a big dent in VRAM. We can do something more clever.

### Radix sort

We can use a parallel variation of [radix sort](https://en.wikipedia.org/wiki/Radix_sort) in order to sort the tiles based on their tile position, we can have all the tile lists be backed by a single array that is exactly big enough to hold all of the particles. This way our memory complexity stays O(n), instead of exploding to O(nt) which would happen if all tiles had a big enough array to hold all particles. Another benefit of this approach is that it exploits the GPU's _cache_ much better, as particles in the same tile remain close in memory. This will turn out to be a big win.

The radix sort is performed in three steps. First, we determine how many particles will go into each tile. Then, allocate a portion of the array to each tile so that we know where the lists for each tile stop and end. And then finally, we add each particle to its associated tile list. In pseudocode this would look something like the following:

```python
for tile in tiles:
  clear(tile)

for p in particles:
  tile = tiles[floor(p.position / tiles.count)]
  tile.capacity += 1

tiles[0].offset = 0
for i in [1 .. tiles.count - 1]:
  tiles[i].offset = tiles[i - 1].offset + tiles[i - 1].capacity

for p in particles:
  tile = tiles[floor(p.position / tiles.count)]
  tiledparticles[tile.offset + tile.size] = p
  tile.size += 1
  
for tile in tiles:
  for p in tile.particles:
      for n in neighbors(tile)
        for particle q in n.particles
          p.velocity += interact(p, q)
          
for p in particles:
  p.position += p.velocity
```

The GPU implementation following the above algorithm can simulate roughly 80'000 particles in real-time. We improved our memory access patterns by implementing the additional steps above, however we have also reached a point where scheduling the compute shaders becomes a large bottleneck.

### Unification

Many of the above steps have to do relatively little work compared to the step where the particle interactions are finally calculated, but that step can't run until all of the previous steps are finished, so we end up waiting a lot before we can do the real work. At this point we can realize some of the 6 steps above can be combined into 4 like so:

```python
tiles[0].size = 0
tiles[0].offset = 0
for i in [1 .. tiles.count - 1]:
  tiles[i].offset = tiles[i - 1].offset + tiles[i - 1].capacity
  tiles[i].size = 0

for p in particles:
  tile = tiles[floor(p.position / tiles.count)]
  tiledparticles[tile.offset + tile.size] = p
  tile.size += 1

for tile in tiles:
  for p in tile.particles:
      for n in neighbors(tile)
        for q in n.particles
          p.velocity += interact(p, q)
          
for p in particles:
  p.position += p.velocity
  tile = tiles[floor(p.position / tiles.count)]
  tile.capacity += 1
```

These 4 steps are equivalent to the above 6, however they require the tile capacities to already be computed before running the first timestep, so this work has to be done on the CPU before the first timestep is simulated on the GPU. Each of the 4 steps is performed by 1 of the 4 compute shaders. This is the final step in the optimization. Using this algorithm we can finally simulate 100'000 particles in real-time.

![](/screenshots/8.png)

### Leftover details

The above sections only mention large optimizations that gave a significant performance improvement however many smaller but interesting optimizations are not covered. For example, instead of having each thread of a compute shader workgroup fetch the same value from memory, this value can be fetched by only 1 thread, and then cached for use by the others. This greatly reduces memory contention and resulted in a 20% performance boost when applied over all of the shaders.

Some implementation details are also left out of the above, such as how the `tiledparticles` list doesn't just hold a reference to particles from the `particles` array, but rather the particle array is [double-buffered](https://en.wikipedia.org/wiki/Multiple_buffering). You can find more details in the source code.

## Benchmarks

3 benchmarks of the final simulation code were run on 4 different computers and 7 different graphics cards. In the benchmarks I measured the time taken to simulate and draw 1,000 timesteps of a simulation with 10'000, 50'000, 100'000, and 200'000 particles. The RNG seed `42` was used to generate every universe from the benchmark for consistency. Vsync was turned off, and window event processing was ignored during the benchmark runs. Laptop machines were plugged in and charged through the simulation, and all other programs were closed.

### GPU specs

The following GPUs were used in the benchmarks. The clock speeds reported here were measured during executation of the benchmark using [GPU-Z](https://www.techpowerup.com/gpuz/).

| model                      | machine-type | cores | GPU-clock [MHz] | memory-clock [MHz] |
| -------------------------- | :----------: | ----: | --------------: | -----------------: |
| NVIDIA GeForce GTX 1080 Ti | desktop      | 3584  | 2075            | 5643               |
| NVIDIA GeForce GTX 1050    | laptop       |  640  | 1721            | 3504               |
| NVIDIA GeForce 940MX       | laptop       |  384  | 1176            | 2000               |
| Intel HD Graphics 620      | laptop       |   24  | 1050            | 2400               |
| Intel UHD Graphics 620     | laptop       |   24  | 1050            | 2400               |
| Intel HD Graphics 630      | laptop       |   24  | 1000            | 2400               |
| NVIDIA GeForce MX110       | laptop       |  256  | 1005            | 2505               |

### Data: particle count

For this benchmark, all machines ran the benchmarks once at their highest clock speeds which are reported above. The numbers in the cells report the amount of time taken to simulate 1'000 timesteps.

| model               | 10'000 [sec] | 50'000 [sec] | 100'000 [sec] | 200'000 [sec] |
| ------------------- | -----------: | -----------: | ------------: | ------------: |
| GeForce GTX 1080 Ti |        0.293 |        2.913 |         9.377 |         26.63 |
| GeForce GTX 1050    |        2.500 |        10.79 |         33.51 |         119.9 |
| GeForce 940MX       |        3.747 |        41.02 |         152.4 |         600.8 |
| HD Graphics 620     |        6.345 |        42.80 |         150.2 |         580.9 |
| UHD Graphics 620    |        2.301 |        12.41 |         151.8 |         597.1 |
| HD Graphics 630     |        5.991 |        47.10 |         165.3 |         641.7 |
| GeForce MX110       |        4.727 |        51.84 |         186.6 |         713.9 |

The quadratic nature of the particle interaction algorithm can clearly be seen from the data - doubling the particle count generally tends to increase the time taken to complete the benchmark by 4x.

### Data: core clock-speed

For this benchmark, only the machine with the GTX 1050 graphics card was used, and the clock-speed of the card was changed. The VRAM memory clock speed was 3504MHz.

| clock-speed [MHz] | underclock [MHz] | 10'000 [sec] | 50'000 [sec] | 100'000 [sec] | 200'000 [sec] |
| ----------------: | ---------------: | -----------: | -----------: | ------------: | ------------: |
|              1733 |               -0 |        2.501 |        10.79 |         33.51 |         119.9 |
|              1632 |             -100 |        2.290 |        11.56 |         36.61 |         128.4 |
|              1531 |             -200 |        2.294 |        12.62 |         38.87 |         137.1 |
|              1417 |             -300 |        2.496 |        12.44 |         40.52 |         146.6 |

As is to be expected, the performance seems to scale close to linearly with clock-speed. This can be seen in the 200'000 particle case where the clock speed was lowered by 17% and the performance decreased by 22%.

### Data: video-memory clock-speed

For this benchmark, only the machine with the GTX 1050 graphics card was used, and the video memory clock-speed was changed. The core clock speed was 1721MHz.

| clock-speed [MHz] | underclock [MHz] | 10'000 [sec] | 50'000 [sec] | 100'000 [sec] | 200'000 [sec] |
| ----------------: | ---------------: | -----------: | -----------: | ------------: | ------------: |
|              3504 |               -0 |        2.501 |        10.79 |         33.51 |         119.9 |
|              3354 |             -150 |        3.252 |        12.54 |         39.21 |         156.8 |
|              3204 |             -300 |        3.218 |        12.76 |         39.23 |         157.4 |
|              3054 |             -450 |        3.236 |        13.16 |         38.58 |         157.4 | 

Interestingly enough even a small drop in memory clock drastically lowered the performance in all cases except with 10'000 particles. Even more curiously lowering the memory clock further did not significantly affect performance. I'm not exactly sure why this is the case. It could indicate a problem with the benchmark - or with the algorithm - but this is something I have to look into further. 

## Requirements

1. C99 compiler
2. [GLFW](https://www.glfw.org/) window opening library
3. OpenGL 4.3 capable GPU

## How to compile..

#### .. with Visual Studio

A complete Visual Studio solution is provided in the [/bin](`/bin`) directory. Open it up and run.

#### .. with GCC or clang

Make sure to [install GLFW](https://www.glfw.org/download.html) through your package manager, or use an appropriate GLFW static library provided in the [/lib](`/lib`) directory. You need to link against GLFW. 

```bash
$ gcc -std=c99 -O2 *.c -lm -lglfw
```

```bash
$ clang -std=c99 -O2 *.c -lm -lglfw
```

## How to run

Place the [`/shaders`](/shaders) directory **in the same directory as the executable** and simply run the executable. A command-line prompt will then appear and the application ask you how many particles to simulate. The list of controls will also be printed on the command line. 

**Do not** try to simulate more particles than your GPU can reasonably handle because your driver might hang, crashing your whole computer. Refer to the given benchmarks as a reference point.

![](/screenshots/9.png)

If you couldn't or didn't compile from source for whatever reason, pre-compiled executables are provided in the [`/bin`](/bin) directory. One is for [windows](/bin/Pocket%20Universe.exe), and the other is for [linux](/bin/Pocket%20Universe.out). Make sure you also place the [`/shaders`](/shaders) directory in the same directory as the executable when running.

### Controls

| key   | function                     |
| :---: | ---------------------------- |
| `ESC` | close the simulation         |
| `H`   | print out the controls       |
| `W`   | toggle universe wrap-around  |
| `V`   | toggle vsync                 |
| `TAB` | print simulation parameters  |
| `B`   | randomize balanced           |
| `C`   | randomize chaos              |
| `D`   | randomize diversity          |
| `F`   | randomize frictionless       |
| `G`   | randomize gliders            |
| `O`   | randomize homogeneity        |
| `L`   | randomize large clusters     |
| `M`   | randomize medium clusters    |
| `S`   | randomize small clusters     |
| `Q`   | randomize quiescence         |
