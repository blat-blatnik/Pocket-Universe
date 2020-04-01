# Pocket-Universe

A particle simulation running in parallel on the GPU using compute shaders. In this particle system the particles can attract and repel each other in a small radius and it looks astounding and very life-like when running in real time. I optimized it as much as I could. The current system can simulate around 100,000-200,000 particles in real-time (30fps) on a modern GPU. 

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

If for you couldn't or didn't compile for any reason, pre-compiled executables are provided in the [`/bin`](/bin) directory. One is for [windows](/bin/Pocket%20Universe.exe), and the other is for [linux](/bin/Pocket%20Universe.out). Make sure you also place the [`/shaders`](/shaders) directory in the same directory as the executable when running.

### Controls

key   | function
----------------
`ESC` | close the simulation
`H`   | print this help message
`W`   | toggle universe wrap-around
`V`   | toggle vsync
`TAB` | print simulation parameters
`B`   | balanced
`C`   | chaos
`D`   | diversity
`F`   | frictionless
`G`   | gliders
`O`   | homogeneity
`L`   | large clusters
`M`   | medium clusters
`S`   | small clusters
`Q`   | quiescence

![](/screenshots/1.png)

![](/screenshots/4.png)
