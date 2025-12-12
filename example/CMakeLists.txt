if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
  cmake_minimum_required(VERSION 3.21)
  project(qml_material_example LANGUAGES CXX)
  find_package(Qt6 6.5 REQUIRED COMPONENTS Core Quick Qml QmlTools)
  # Upstream packages export that dependency, so ensure it's available here.
  if(Qt6Quick_VERSION VERSION_GREATER_EQUAL 6.10)
    find_package(Qt6 REQUIRED COMPONENTS QuickPrivate)
  endif()
  find_package(qml_material CONFIG REQUIRED)
  if(TARGET qml_material)
    set(LIBNAME qml_material)
  elseif(TARGET qml_material::qml_material)
    set(LIBNAME qml_material::qml_material)
  else()
    message(FATAL_ERROR "qml_material target not found after find_package")
  endif()
  get_target_property(_qml_material_type ${LIBNAME} TYPE)
  if(_qml_material_type STREQUAL "STATIC_LIBRARY")
    set(QML_MATERIAL_BUILD_TYPE STATIC)
  else()
    set(QML_MATERIAL_BUILD_TYPE SHARED)
  endif()
else()
  # Deploy script support when built from the main repo
  find_package(Qt6 6.5 REQUIRED COMPONENTS QmlTools)
endif()

if(NOT DEFINED LIBNAME)
  set(LIBNAME qml_material)
endif()
if(NOT TARGET ${LIBNAME})
  find_package(qml_material CONFIG REQUIRED)
endif()

set(CMAKE_AUTOMOC ON)
if(COMMAND qt_policy)
  qt_policy(SET QTP0001 NEW)
endif()

qt_add_executable(qm_example main.cpp)
set_target_properties(qm_example PROPERTIES AUTOMOC ON)
target_link_libraries(qm_example PRIVATE Qt6::Core Qt6::Quick Qt6::Qml ${LIBNAME})

qt_add_qml_module(
  qm_example
  URI Demo
  VERSION 1.0
  QML_FILES
    Main.qml
    Example.qml
    Components.qml
    Color.qml
    Shape.qml
    Elevation.qml
    Typography.qml
    About.qml
)

qt_generate_deploy_qml_app_script(
  TARGET qm_example
  OUTPUT_SCRIPT deploy_qm_example
  NO_UNSUPPORTED_PLATFORM_ERROR
)
install(TARGETS qm_example RUNTIME DESTINATION bin BUNDLE DESTINATION .)
install(SCRIPT ${deploy_qm_example})

if(QML_MATERIAL_BUILD_TYPE MATCHES STATIC)
  qt_import_qml_plugins(qm_example)
  if(TARGET qml_materialplugin)
    target_link_libraries(qm_example PRIVATE qml_materialplugin)
  elseif(TARGET qml_material::qml_materialplugin)
    target_link_libraries(qm_example PRIVATE qml_material::qml_materialplugin)
  endif()
  target_compile_definitions(qm_example PRIVATE QML_STATIC)
endif()
