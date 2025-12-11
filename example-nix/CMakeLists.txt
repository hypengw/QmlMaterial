cmake_minimum_required(VERSION 3.21)
project(qml_material_demo LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)

find_package(Qt6 6.5 REQUIRED COMPONENTS Core Quick Qml)
if(COMMAND qt_policy)
  qt_policy(SET QTP0001 NEW)
endif()
find_package(qml_material CONFIG REQUIRED)

add_executable(qml-material-demo main.cpp)
set_target_properties(qml-material-demo PROPERTIES AUTOMOC ON)
target_link_libraries(qml-material-demo PRIVATE Qt6::Core Qt6::Quick Qt6::Qml qml_material)

qt_add_qml_module(
  qml-material-demo
  URI Demo
  VERSION 1.0
  QML_FILES main.qml
)

qt_generate_deploy_qml_app_script(
  TARGET qml-material-demo
  OUTPUT_SCRIPT deploy_qml_material_demo
  NO_UNSUPPORTED_PLATFORM_ERROR
)
install(TARGETS qml-material-demo RUNTIME DESTINATION bin BUNDLE DESTINATION .)
install(SCRIPT ${deploy_qml_material_demo})
