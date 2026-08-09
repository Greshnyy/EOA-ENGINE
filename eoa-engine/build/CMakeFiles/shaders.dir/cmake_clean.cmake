file(REMOVE_RECURSE
  "CMakeFiles/shaders"
  "shaders/deferred_lighting.frag.spv"
  "shaders/deferred_lighting.vert.spv"
  "shaders/gbuffer.frag.spv"
  "shaders/gbuffer.vert.spv"
  "shaders/particle.frag.spv"
  "shaders/particle.vert.spv"
  "shaders/shadow_depth.frag.spv"
  "shaders/shadow_depth.vert.spv"
  "shaders/triangle.frag.spv"
  "shaders/triangle.vert.spv"
)

# Per-language clean rules from dependency scanning.
foreach(lang )
  include(CMakeFiles/shaders.dir/cmake_clean_${lang}.cmake OPTIONAL)
endforeach()
