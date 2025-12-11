if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
  cmake_minimum_required(VERSION 3.21)
  project(qml_material_example LANGUAGES CXX)
  find_package(Qt6 6.5 REQUIRED COMPONENTS Core Quick Qml QmlTools)
  find_package(qml_material CONFIG REQUIRED)
  set(LIBNAME qml_material)
  get_target_property(_qml_material_type ${LIBNAME} TYPE)
  if(_qml_material_type STREQUAL "STATIC_LIBRARY")
    set(QML_MATERIAL_BUILD_TYPE STATIC)
  else()
    set(QML_MATERIAL_BUILD_TYPE SHARED)
  endif()
else()
  find_package(Qt6 6.5 REQUIRED COMPONENTS QmlTools) # deploy script support when built as subdir
endif()

if(NOT DEFINED LIBNAME)
  set(LIBNAME qml_material)
endif()

if(NOT TARGET ${LIBNAME})
  find_package(qml_material CONFIG REQUIRED)
endif()

qt_add_resources(
  ${LIBNAME}
  "example"
  PREFIX
  /Qcm/Material/Example
  # BASE <PATH>
  FILES
  Components.qml
  Color.qml
  Elevation.qml
  Example.qml
  Typography.qml
  Shape.qml
  About.qml)

qt_add_executable(qm_example main.cpp)
qt_add_resources(
  qm_example "example_main" PREFIX /
  # BASE <PATH>
  FILES main.qml)

set_target_properties(qm_example PROPERTIES AUTOMOC ON)
target_link_libraries(qm_example PRIVATE Qt6::Core Qt6::Quick ${LIBNAME})

qt_generate_deploy_qml_app_script(
  TARGET qm_example
  OUTPUT_SCRIPT deploy_qm_example
  NO_UNSUPPORTED_PLATFORM_ERROR)

install(TARGETS qm_example RUNTIME DESTINATION bin BUNDLE DESTINATION .)
install(SCRIPT ${deploy_qm_example})

if(QML_MATERIAL_BUILD_TYPE MATCHES STATIC)
  qt_import_qml_plugins(qm_example)
  target_link_libraries(qm_example PRIVATE qml_materialplugin)
  target_compile_definitions(qm_example PRIVATE QML_STATIC)
endif()
