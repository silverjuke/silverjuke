
#ifndef __PRJM_AMAGATION_H__
#define __PRJM_AMAGATION_H__


#include <sjbase/base.h>
#if SJ_USE_PROJECTM


/*******************************************************************************
 * OS define needed for projectM
 ******************************************************************************/


#if defined(__WXMAC__)
	#ifndef __APPLE__
	#define __APPLE__
	#endif
	#ifndef MACOS
	#define MACOS
	#endif
#elif defined(__WXMSW__)
	#ifndef WIN32
	#define WIN32
	#endif
	#ifndef _WIN32
	#define _WIN32
	#endif
#else
	#ifndef LINUX
	#define LINUX
	#endif
#endif

// some checks for OS consistences
#if (defined(LINUX) && defined(WIN32)) || (defined(LINUX) && defined(MACOS)) || (defined(WIN32) && defined(MACOS))
	#error define only one of LINUX, WIN32 or MACOS
#endif
#if (defined(__APPLE__) && !defined(MACOS)) || (!defined(__APPLE__) && defined(MACOS))
	#error define both, __APPLE__ and MACOS - both are used in the code :-(
#endif


/*******************************************************************************
 * projectM special includes, types and settings
 ******************************************************************************/


#define CMAKE_INSTALL_PREFIX ""
#define projectM_FONT_TITLE  ""
#define projectM_FONT_MENU   ""

// the following options are enabled by default - and should be enabled by us (non-defaults are more buggy ...)
//#define USE_FBO
//#define USE_NATIVE_GLEW // we prefer our own glew.h, needed for USE_FBO

#define DISABLE_NATIVE_PRESETS // presets in C++-libraries not needed

#define STBI_NO_FAILURE_STRINGS // used in stb_image_aug.c, disabling the failure strings avoid lots of string errors on Mac

//#define USE_OPENMP
//#define USE_FTGL
#undef USE_THREADS

// all options - description - default:
//OPTION (USE_DEVIL "Use devIL for image loading rather than the builtin SOIL library" OFF)
//OPTION (USE_FBO "Use Framebuffer Objects for increased rendering quality.  Disable this for OpenGL ES 1.x or if you are experiencing problems on older or poorly supported hardware." ON)
//OPTION(USE_FTGL "Use FTGL for on-screen fonts (found on your system)" ON)
//OPTION (USE_GLES1 "Use OpenGL ES 1.x" OFF)
//OPTION (USE_THREADS "Use threads for parallelization" ON)
//OPTION (USE_OPENMP "Use OpenMP and OMPTL for multi-core parallelization" ON)
//OPTION (USE_NATIVE_GLEW "Use projectM's native implemention of GLEW." OFF)
//OPTION (USE_CG "Use Cg for Pixel Shader support" OFF)
//OPTION (DISABLE_NATIVE_PRESETS "Turn off support for native (C++ style) presets" OFF)
//OPTION (DISABLE_MILKDROP_PRESETS "Turn off support for Milkdrop (.milk / .prjm) presets"  OFF)



#endif // SJ_USE_PROJECTM


#endif // __PRJM_AMAGATION_H__

