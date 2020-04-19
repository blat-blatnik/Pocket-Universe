/* This entire particle simulation idea was inspired by the "Particle Life" youtube video by CodeParade.
   His channel has some of the most interesting coding videos I've seen.
   Definitely check him out.

   Particle Life video: https://youtu.be/Z_zmZ23grXE
   CodeParade: https://www.youtube.com/channel/UCrv269YwJzuZL3dH5PCgxUw */

#include "universe.h"
#include "glfw3.h"
#include <stdlib.h>
#include <stdio.h>

/* Uncomment below to compile a benchmark executable */
/* #define BENCHMARK */

/* Request a dedicated GPU if avaliable.
   See: https://stackoverflow.com/a/39047129 */
#ifdef _MSC_VER
__declspec(dllexport) unsigned long NvOptimusEnablement = 1;
__declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;
#endif

static Universe universe;
static int vsyncIsOn = 1;

static void onGlfwError(int code, const char *desc) {
	fprintf(stderr, "GLFW error 0x%X: %s\n", code, desc);
}

/* Callback for when and OpenGL debug error occurs. */
static void onGlError(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar *message, const void *userParam) {
	const char* severityMessage =
		severity == GL_DEBUG_SEVERITY_HIGH         ? "error" :
		severity == GL_DEBUG_SEVERITY_MEDIUM       ? "warning" :
		severity == GL_DEBUG_SEVERITY_LOW          ? "warning" :
		severity == GL_DEBUG_SEVERITY_NOTIFICATION ? "info" : 
		"unknown";
	const char *sourceMessage =
		source == GL_DEBUG_SOURCE_SHADER_COMPILER  ? "GLSL compiler" :
		source == GL_DEBUG_SOURCE_API              ? "API" :
		source == GL_DEBUG_SOURCE_WINDOW_SYSTEM    ? "windows API" :
		source == GL_DEBUG_SOURCE_APPLICATION      ? "application" :
		source == GL_DEBUG_SOURCE_THIRD_PARTY      ? "third party" :
		"unknown";

	if (severity != GL_DEBUG_SEVERITY_NOTIFICATION) {
		fprintf(stderr, "OpenGL %s 0x%X: %s (source: %s)\n", severityMessage, (int)id, message, sourceMessage);
	}
}

/* Report a fatal application error and abort. */
static void fatalError(const char* message) {
	fprintf(stderr, "FATAL ERROR: %s .. aborting\n", message);
	abort();
}

/* Print the controls. */
static void printHelp() {
	printf(" ================ controls ================\n");
	printf("|| ESC               close the simulation ||\n");
	printf("|| H              print this help message ||\n");
	printf("|| W          toggle universe wrap-around ||\n");
	printf("|| V                         toggle vsync ||\n");
	printf("|| TAB        print simulation parameters ||\n");
	printf("||                                        ||\n");
	printf("|| ------------ randomization ----------- ||\n");
	printf("||                                        ||\n");
	printf("|| B                             balanced ||\n");
	printf("|| C                                chaos ||\n");
	printf("|| D                            diversity ||\n");
	printf("|| F                         frictionless ||\n");
	printf("|| G                              gliders ||\n");
	printf("|| O                          homogeneity ||\n");
	printf("|| L                       large clusters ||\n");
	printf("|| M                      medium clusters ||\n");
	printf("|| S                       small clusters ||\n");
	printf("|| Q                           quiescence ||\n");
	printf(" ==========================================\n");
}

/* This is called when a key is pressed/released. */
static void onKey(GLFWwindow *window, int key, int scancode, int action, int mods) {
	if (action != GLFW_PRESS)
		return;

	switch (key) {
		case GLFW_KEY_ESCAPE:
			glfwSetWindowShouldClose(window, GLFW_TRUE);
		break;
		case GLFW_KEY_H:
			printHelp();
		break;
		case GLFW_KEY_TAB:
			printParams(&universe);
		break;
		case GLFW_KEY_W:
			universe.wrap = !universe.wrap;
		break;
		case GLFW_KEY_V:
			vsyncIsOn = !vsyncIsOn;
			glfwSwapInterval(vsyncIsOn);
		break;
		case GLFW_KEY_B:
			universe.friction = 0.05f;
			randomize(&universe, -0.02f, 0.06f, 0.0f, 20.0f, 20.0f, 70.0f);
		break;
		case GLFW_KEY_C:
			universe.friction = 0.01f;
			randomize(&universe, 0.02f, 0.04f, 0.0f, 30.0f, 30.0f, 100.0f);
		break;
		case GLFW_KEY_D:
			universe.friction = 0.05f;
			randomize(&universe, -0.01f, 0.04f, 0.0f, 20.0f, 10.0f, 60.0f);
		break;
		case GLFW_KEY_F:
			universe.friction = 0.0f;
			randomize(&universe, 0.01f, 0.005f, 10.0f, 10.0f, 10.0f, 60.0f);
		break;
		case GLFW_KEY_G:
			universe.friction = 0.1f;
			randomize(&universe, 0.0f, 0.06f, 0.01f, 20.0f, 10.0f, 50.0f);
		break;
		case GLFW_KEY_O:
			universe.friction = 0.05f;
			randomize(&universe, 0.0f, 0.04f, 10.0f, 10.0f, 10.0f, 80.0f);
		break;
		case GLFW_KEY_L:
			universe.friction = 0.2f;
			randomize(&universe, 0.025f, 0.02f, 0.0f, 30.0f, 30.0f, 100.0f);
		break;
		case GLFW_KEY_M:
			universe.friction = 0.05f;
			randomize(&universe, 0.02f, 0.05f, 0.0f, 20.0f, 20.0f, 50.0f);
		break;
		case GLFW_KEY_Q:
			universe.friction = 0.2f;
			randomize(&universe, -0.02f, 0.1f, 10.0f, 20.0f, 20.0f, 60.0f);
		break;
		case GLFW_KEY_S:
			universe.friction = 0.01f;
			randomize(&universe, -0.005f, 0.01f, 10.0f, 10.0f, 20.0f, 50.0f);
		break;
		default: break;
	}
}

/* This is called when the mouse wheel is scrolled. */
static void onMouseWheel(GLFWwindow *window, double dx, double dy) {
	if (dy > 0.0)
		universe.deltaTime *= 1.1f;
	else if (dy < 0.0)
		universe.deltaTime /= 1.1f;
}

/* This is called when the window is resized. */
static void onFramebufferResize(GLFWwindow *window, int newWidth, int newHeight) {
	glViewport(0, 0, newWidth, newHeight);
}

int main(void) {

	/* Print the intro. */
	printf("\nwelcome to the..\n\n");
	printf(" ========== Pocket Universe ========== \n");
	printf("||     .     *           .      *    ||\n");
	printf("||  *      .     .   *      .     .  ||\n");
	printf("||        .   *       .       *  .   ||\n");
	printf("||.   *           *    . .           ||\n");
	printf("||     .   .  *   .   .          *   ||\n");
	printf("||  *    .      .           *        ||\n");
	printf("||    .       *      *  .      .  .  ||\n");
	printf("|| .     *        .         *        ||\n");
	printf("||         .    *       *        *   ||\n");
	printf(" ===================================== \n");
	printf("\n");
	printf("A particle simulation program.\n\n");
	printf("how many particles would you like to create? ");
	int numParticles, numParticleTypes;
	scanf("%d", &numParticles);
	printf("and how many varieties of particles? ");
	scanf("%d", &numParticleTypes);
	printf("\ninitializing ");

	/* Initialize GLFW. */
	glfwSetErrorCallback(onGlfwError);
	int glfwOk = glfwInit();
	if (!glfwOk) {
		fatalError("failed to initialize GLFW");
	}

	printf(".");

	/* The window should be hidden for now.. */
	glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
	/* We don't use depth and stencil buffers. */
	glfwWindowHint(GLFW_DEPTH_BITS, 0);
	glfwWindowHint(GLFW_STENCIL_BITS, 0);
	/* We need at least OpenGL 4.3 for shader storage buffers. */
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	/* We don't want to use deprecated functionality. */
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifndef NDEBUG
	glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GLFW_TRUE);
#endif

	/* Create a window and make its OpenGL context current. */
	GLFWwindow *window = glfwCreateWindow(1280, 720, "Pocket Universe", NULL, NULL);
	if (window == NULL) {
		fatalError("failed to open a window");
	}

	printf(".");
	glfwMakeContextCurrent(window);

	/* Load all OpenGL functions. */
	int gladOk = gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
	if (!gladOk) {
		fatalError("failed to load OpenGL functions");
	}
	printf(".");

	glfwSwapInterval(0);            /* Vsync */
	glEnable(GL_FRAMEBUFFER_SRGB); 	/* Gamma correction */
	glEnable(GL_BLEND);             /* Transparency */
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glClearColor(0, 0, 0, 1);

#ifndef NDEBUG
	glEnable(GL_DEBUG_OUTPUT);
	glDebugMessageCallback(onGlError, NULL);
	glCheckErrors();
#endif

	glfwSetKeyCallback(window, onKey);
	glfwSetScrollCallback(window, onMouseWheel);
	glfwSetFramebufferSizeCallback(window, onFramebufferResize);

	double timerFrequency = (double)glfwGetTimerFrequency();
	uint64_t t0 = glfwGetTimerValue();

	/* Set up the initial universe. */
	universe = createUniverse(numParticleTypes, numParticles, 1280, 720);
	universe.deltaTime = 1.0f;
	universe.friction = 0.05f;
	universe.wrap = GL_TRUE;
	universe.particleRadius = 5.0f;
#ifdef BENCHMARK
	universe.rng = seedRNG(42);
#endif
	randomize(&universe, -0.02f, 0.06f, 0.0f, 20.0f, 20.0f, 70.0f);
	printf(" done\n");

	const char *version = (const char *)glGetString(GL_VERSION);
	const char *renderer = (const char *)glGetString(GL_RENDERER);
	printf("using OpenGL %s: %s\n", version, renderer);
	if (GLVersion.major < 4 || (GLVersion.major == 4 && GLVersion.minor < 3)) {
		fatalError("need at least OpenGL 4.3 to run");
	}

	uint64_t t1 = glfwGetTimerValue();
	printf("created universe in %.3lf seconds\n\n", (t1 - t0) / timerFrequency);

	printHelp();
	printf("\n");

	/* Start the simulation loop. */
	glfwShowWindow(window);
	double totalTime = 0;
	double timeAcc = 0;
	int frameAcc = 0;
	t0 = glfwGetTimerValue();

#ifdef BENCHMARK
	glfwSwapInterval(0);
	const int benchmarkTimesteps = 1000;
	printf("running benchmark for %d timesteps\n", benchmarkTimesteps);
	while (frameAcc < benchmarkTimesteps) {
		simulateTimestep(&universe);
		glClear(GL_COLOR_BUFFER_BIT);
		draw(&universe);
		glfwSwapBuffers(window);
		frameAcc += 1;
	}
	t1 = glfwGetTimerValue();
	double benchmarkTime = (t1 - t0) / timerFrequency;
	printf("benchmark finished! took %lg seconds to finish %d timesteps (average time per timestep is %lg seconds).", 
		benchmarkTime, benchmarkTimesteps, benchmarkTime / benchmarkTimesteps);
#else
	/* Enter the simulation loop. */
	while (!glfwWindowShouldClose(window)) {
		glfwPollEvents();
		t1 = glfwGetTimerValue();
		totalTime += universe.deltaTime;
		double deltaTime = (t1 - t0) / timerFrequency;
		t0 = t1;

		simulateTimestep(&universe);
		glClear(GL_COLOR_BUFFER_BIT);
		draw(&universe);
		glCheckErrors();

		/* Update the statistics in the window title. */
		timeAcc  += deltaTime;
		frameAcc += 1;
		if (timeAcc >= 0.1) {
			char newTitle[512];
			if (totalTime >= 1000000) {
				sprintf(newTitle, "Pocket Universe [t=%.1lfM (+%g) | %.1lf tsps]",
					totalTime / 1000000.0, universe.deltaTime, frameAcc / timeAcc);
			} else if (totalTime >= 2000) {
				sprintf(newTitle, "Pocket Universe [t=%.1lfk (+%g) | %.1lf tsps]",
					totalTime / 1000.0, universe.deltaTime, frameAcc / timeAcc);
			} else {
				sprintf(newTitle, "Pocket Universe [t=%.1lf (+%g) | %.1lf tsps]",
					totalTime, universe.deltaTime, frameAcc / timeAcc);
			}
			glfwSetWindowTitle(window, newTitle);
			frameAcc = 0;
			timeAcc  = 0;
		}

		glfwSwapBuffers(window);
	}
#endif

	/* Destroy all used resources and end the program. */
	destroyUniverse(&universe);
	glfwDestroyWindow(window);
	glfwTerminate();	
	return 0;
}
