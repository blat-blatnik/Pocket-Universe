# Pocket-Universe

A particle simulation running in parallel on the GPU using compute shaders. In this particle system the particles can attract and repel each other in a small radius and it looks astounding and very life-like when running in real time. I optimized it as much as I could. The current system can simulate around 100,000-200,000 particles in real-time (30fps) on a modern GPU. 

![](/screenshots/1.png)

## Particle Game of Life

This project was inspired by [CodeParade](https://www.youtube.com/channel/UCrv269YwJzuZL3dH5PCgxUw)'s [Particle Life](https://youtu.be/Z_zmZ23grXE) simulation. In this simulation the world consists of a number of differently colored particles. These particles can can be attracted - or repelled - by particles of different colors. For example, _blue_ particles might be attracted to _red_ particles, while _red_ particles might be repelled by _green_ particles, etc. A major difference between Particle Life and the [Game of Life](https://en.wikipedia.org/wiki/Conway%27s_Game_of_Life) is that in Particle Life, the particles can occupy any position in space, not just integer grid positions.

Each particle only attracts or repells other particles which are withing some _maximum radius_. If two particles come really really close together they will always start strongly repelling in order to avoid occupying the same space. There is also _friction_ in the system, meaning the particles lose a proportion of their velocity each second - because of this the particles tend to fall into mutually stable arrangements with other particles.

If you want more details you should check out [the video](https://youtu.be/Z_zmZ23grXE) by [CodeParade](https://www.youtube.com/channel/UCrv269YwJzuZL3dH5PCgxUw) which goes into more detail. My goal with this project was to optimize this simulation so that it could run a very large number of particles. I did so by implementing Particle Life in compute shaders running massively in parallel on the GPU.

![](/screenshots/4.png)

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
$ gcc -std=c99 -o2 *.c -lm -lglfw
```

```bash
$ clang -std=c99 -o2 *.c -lm -lglfw
```

## How to run

Simply run the compiled executable. A command-line prompt will then appear and the application ask you how many particles to simulate. The list of controls will also be printed on the command line. Note that you need to have the [`/shaders`](/shaders) directory **in the same directory as the executable** when running.

**Do not** try to simulate more particles than your GPU can reasonably handle because your driver might hang, crashing your whole computer. Refer to the given benchmarks as a reference point.

If for you couldn't or didn't compile for any reason, pre-compiled executables are provided in the [`/bin`](/bin) directory. One is for [windows](/bin/Pocket%20Universe.exe), and the other is for [linux](/bin/Pocket%20Universe.out). Make sure you also place the [`/shaders`](/shaders) directory in the same directory as the executable when running.

### Controls

| key   | function                     |
| :---: | ---------------------------- |
| `ESC` | close the simulation         |
| `H`   | print this help message      |
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
