file(REMOVE_RECURSE
  "libglfw.pdb"
  "libglfw.so"
  "libglfw.so.3.2"
  "libglfw.so.3"
)

# Per-language clean rules from dependency scanning.
foreach(lang C)
  include(CMakeFiles/glfw.dir/cmake_clean_${lang}.cmake OPTIONAL)
endforeach()
