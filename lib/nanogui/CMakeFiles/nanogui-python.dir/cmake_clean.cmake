file(REMOVE_RECURSE
  "python/nanogui.pdb"
  "python/nanogui.cpython-37m-x86_64-linux-gnu.so"
)

# Per-language clean rules from dependency scanning.
foreach(lang C CXX)
  include(CMakeFiles/nanogui-python.dir/cmake_clean_${lang}.cmake OPTIONAL)
endforeach()
