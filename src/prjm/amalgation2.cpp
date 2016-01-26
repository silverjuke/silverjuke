
#include "amalgation.h"
#if SJ_USE_PROJECTM


#include "src/MilkdropPresetFactory/Parser.cpp"


extern "C"
{
	#ifdef USE_NATIVE_GLEW
	#include "glew/glew.c"
	#endif

	#include "src/Renderer/SOIL/stb_image_aug.c"  // image_DXT.c and stb_image_aug.c use different structures with the same name; this is the reason for the two amalgation files
};


#endif // SJ_USE_PROJECTM

