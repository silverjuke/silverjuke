
#include "amalgation.h"
#if SJ_USE_PROJECTM


//#include "src/ConfigFile.cpp"
#include "src/fftsg.cpp"
#include "src/KeyHandler.cpp"
#include "src/MilkdropPresetFactory/BuiltinFuncs.cpp"
#include "src/MilkdropPresetFactory/BuiltinParams.cpp"
#include "src/MilkdropPresetFactory/CustomShape.cpp"
#include "src/MilkdropPresetFactory/CustomWave.cpp"
#include "src/MilkdropPresetFactory/Eval.cpp"
#include "src/MilkdropPresetFactory/Expr.cpp"
#include "src/MilkdropPresetFactory/Func.cpp"
#include "src/MilkdropPresetFactory/IdlePreset.cpp"
#include "src/MilkdropPresetFactory/InitCond.cpp"
#include "src/MilkdropPresetFactory/MilkdropPreset.cpp"
#include "src/MilkdropPresetFactory/MilkdropPresetFactory.cpp"
#include "src/MilkdropPresetFactory/Param.cpp"
#include "src/MilkdropPresetFactory/PerFrameEqn.cpp"
#include "src/MilkdropPresetFactory/PerPixelEqn.cpp"
#include "src/MilkdropPresetFactory/PerPointEqn.cpp"
#include "src/MilkdropPresetFactory/PresetFrameIO.cpp"
#include "src/PCM.cpp"
#include "src/PipelineMerger.cpp"
#include "src/Preset.cpp"
#include "src/PresetFactory.cpp"
#include "src/PresetFactoryManager.cpp"
#include "src/PresetLoader.cpp"
#include "src/projectM.cpp"
#include "src/Renderer/BeatDetect.cpp"
#include "src/Renderer/FBO.cpp"
#include "src/Renderer/Filters.cpp"
#include "src/Renderer/MilkdropWaveform.cpp"
#include "src/Renderer/PerlinNoise.cpp"
#include "src/Renderer/PerPixelMesh.cpp"
#include "src/Renderer/Pipeline.cpp"
#include "src/Renderer/PipelineContext.cpp"
#include "src/Renderer/Renderable.cpp"
#include "src/Renderer/Renderer.cpp"
#include "src/Renderer/RenderItemDistanceMetric.cpp"
#include "src/Renderer/RenderItemMatcher.cpp"
#include "src/Renderer/Shader.cpp"
#include "src/Renderer/ShaderEngine.cpp"
#include "src/Renderer/TextureManager.cpp"
#include "src/Renderer/UserTexture.cpp"
#include "src/Renderer/VideoEcho.cpp"
#include "src/Renderer/Waveform.cpp"
#include "src/TimeKeeper.cpp"
#include "src/timer.cpp"
#include "src/wipemalloc.cpp"

extern "C"
{
	#include "src/Renderer/SOIL/image_DXT.c" // image_DXT.c and stb_image_aug.c use different structures with the same name; this is the reason for the two amalgation files
	#include "src/Renderer/SOIL/image_helper.c"
	#include "src/Renderer/SOIL/SOIL.c"
};


#endif // SJ_USE_PROJECTM

