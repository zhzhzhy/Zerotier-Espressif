file(REMOVE_RECURSE
  "lib/libztcore.a"
  "lib/libztcore.pdb"
)

# Per-language clean rules from dependency scanning.
foreach(lang CXX)
  include(CMakeFiles/ztcore.dir/cmake_clean_${lang}.cmake OPTIONAL)
endforeach()
