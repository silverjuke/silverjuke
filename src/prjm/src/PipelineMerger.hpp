#ifndef PRESET_MERGE_HPP
#define PRESET_MERGE_HPP
#include "Preset.hpp"
#include "Renderer/Pipeline.hpp" // EDIT BY SJ
#include "Renderer/RenderItemMatcher.hpp"  // EDIT BY SJ
#include "Renderer/RenderItemMergeFunction.hpp"  // EDIT BY SJ

class PipelineMerger
{
 template <class T> inline static T lerp(T a, T b, float ratio)
{
  return a * ratio + b * (1 - ratio);
}

public:

  static void mergePipelines(const Pipeline &a,  const Pipeline &b, Pipeline &out,
	RenderItemMatcher::MatchResults & matching, RenderItemMergeFunction & merger, float ratio);

private :

static const double s;
static const double e;

};


#endif
