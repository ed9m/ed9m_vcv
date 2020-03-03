#include <stdio.h>

#ifdef NANOVG_GLEW
#	include <GL/glew.h>
#endif
#include <GLFW/glfw3.h>
#define NANOVG_GL2_IMPLEMENTATION	// Use GL2 implementation.
#include <nanovg.h>
#include <nanovg_gl.h>
#include <nanovg_gl_utils.h>

#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif


#ifdef _WIN32

// see following link for a discussion of the
// warning suppression:
// http://sourceforge.net/mailarchive/forum.php?
// thread_name=50F6011C.2020000%40akeo.ie&forum_name=libusbx-devel

// Disable: warning C4200: nonstandard extension used:
// zero-sized array in struct/union
#pragma warning(disable:4200)

#include <windows.h>
#endif


// void nanovg_draw(){
// 	printf("drawing\n");
// 	printf("2");
// 	int winWidth = (int)(fbWidth / pxRatio);
// 	int winHeight = (int)(fbHeight / pxRatio);

// 	// fb = nvgluCreateFramebuffer(vg, (int)(960*pxRatio), (int)(160*pxRatio), NVG_IMAGE_REPEATX | NVG_IMAGE_REPEATY);
// 	// if (fb == NULL) {
// 	// 	printf("Could not create FBO.\n");
// 	// }

// 	// Draw some stuff to an FBO as a test

// 	// nvgluBindFramebuffer(fb);
// 	glViewport(0, 0, fbWidth, fbHeight);
// 	glClearColor(0, 0, 0, 0);
// 	glClear(GL_COLOR_BUFFER_BIT|GL_STENCIL_BUFFER_BIT);
// 	nvgBeginFrame(vg, winWidth,  winHeight, pxRatio);

// 	float s = 1.0;

// 	int pw = (int)ceilf(winWidth / s);
// 	int ph = (int)ceilf(winHeight / s);

// 	nvgBeginPath(vg);
// 	nvgFontSize(vg, 40.0f);
// 	nvgFontFace(vg, "sans");
// 	nvgTextAlign(vg,NVG_ALIGN_LEFT|NVG_ALIGN_MIDDLE);
// 	nvgText(vg, 300, 50, "Hello from VCV Rack", NULL);
// 	nvgFillColor(vg, nvgRGB(255,255,255));
// 	nvgFill(vg);
// 	nvgClosePath(vg);


// 	nvgBeginPath(vg);
// 	nvgCircle(vg, 500, 100, 10);
// 	nvgFillColor(vg, nvgRGBA(255,0,0,255));
// 	nvgFill(vg);
// 	nvgClosePath(vg);
	
// 	nvgEndFrame(vg);

// 	unsigned short * buf = (unsigned short*)malloc(960*160*4);


// 	glReadPixels(0, 0, 960, 160, GL_RGB, GL_UNSIGNED_SHORT_5_6_5, image);	

// 	printf("2");

// 	for (int i = 0; i < 80*960; i ++){
// 		image[i * 4] ^= 0xE7;
// 		image[i * 4 + 1] ^= 0xF3;
// 		image[i * 4 + 2] ^= 0xE7;
// 		image[i * 4 + 3] ^= 0xFF;
// 	}

// 	// nvgluBindFramebuffer(NULL);
// }

void errorcb(int error, const char* desc)
{
	printf("GLFW error %d: %s\n", error, desc);
}