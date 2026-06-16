qt_add_resources(
  ${LIBNAME}
  "example"
  PREFIX
  /Qcm/Material/Example
  BASE
  "${CMAKE_CURRENT_SOURCE_DIR}"
  FILES
  Components.qml
  Color.qml
  Effect.qml
  Example.qml
  Typography.qml
  About.qml
  assets/carousel/abstract-amber-wide.jpg
  assets/carousel/abstract-coral-landscape.jpg
  assets/carousel/abstract-indigo-portrait.jpg
  assets/carousel/abstract-lime-diagonal.jpg
  assets/carousel/abstract-mint-square.jpg
  assets/carousel/abstract-rose-medium.jpg
  assets/carousel/abstract-sky-small.jpg
  assets/carousel/abstract-sun-ultrawide.jpg
  assets/carousel/abstract-teal-tall.jpg
  assets/carousel/abstract-violet-portrait.jpg)

qt_add_executable(qm_example main.cpp)
qt_add_resources(
  qm_example "example_main" PREFIX /
  # BASE <PATH>
  FILES main.qml)

set_target_properties(qm_example PROPERTIES AUTOMOC ON)
target_link_libraries(qm_example PRIVATE Qt6::Core Qt6::Quick)

# Allow running example from the build tree without installing the QML module.
target_compile_definitions(qm_example PRIVATE QT_QML_OUTPUT_DIRECTORY="${QT_QML_OUTPUT_DIRECTORY}")

if(QML_MATERIAL_BUILD_TYPE MATCHES STATIC)
  qt_import_qml_plugins(qm_example)
  target_link_libraries(qm_example PRIVATE qml_materialplugin)
  target_compile_definitions(qm_example PRIVATE QML_STATIC)
endif()
