qt_add_resources(
  ${LIBNAME}
  "example"
  PREFIX
  /Qcm/Material/Example
  # BASE <PATH>
  FILES
  Color.qml
  Shadow.qml
  Typography.qml)

qt_add_executable(qmaterial_example main.cpp)
qt_add_resources(
  qmaterial_example
  "example_main"
  PREFIX
  /
  # BASE <PATH>
  FILES
  main.qml)

target_link_libraries(
  qmaterial_example
  PRIVATE Qt6::Core Qt6::Quick)
if(QML_MATERIAL_BUILD_TYPE MATCHES STATIC)
  qt_import_qml_plugins(qmaterial_example)
  target_link_libraries(qmaterial_example PRIVATE qml_materialplugin)
  target_compile_definitions(qmaterial_example PRIVATE QML_STATIC)
endif()
