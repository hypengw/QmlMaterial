set(md_dir material-color/cpp)
add_library(
  material_color STATIC
  ${md_dir}/scheme/scheme.cc
  ${md_dir}/palettes/core.cc
  ${md_dir}/palettes/tones.cc
  ${md_dir}/utils/utils.cc
  ${md_dir}/cam/cam.cc
  ${md_dir}/cam/hct.cc
  ${md_dir}/cam/hct_solver.cc
  ${md_dir}/cam/viewing_conditions.cc
  ${md_dir}/blend/blend.cc)
target_include_directories(material_color PUBLIC material-color)