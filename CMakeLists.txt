cmake_minimum_required(VERSION 3.14)

project(
  QmlMaterial
  VERSION 0.1
  LANGUAGES CXX)

option(QM_BUILD_STATIC "build static plugin" OFF)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

set(QM_BUILD SHARED)
if(QM_BUILD_STATIC)
  set(QM_BUILD STATIC)
endif()

find_package(Qt6 REQUIRED COMPONENTS Core Quick QuickControls2 Gui)
find_package(PkgConfig REQUIRED)

set(LIBNAME qml_material)

set(SOURCES
    include/qml_material/util.h
    include/qml_material/theme.h
    include/qml_material/color.h
    include/qml_material/corner.h
    include/qml_material/icon.h
    include/qml_material/input_block.h
    include/qml_material/item_holder.h
    include/qml_material/type.h
    include/qml_material/round_item.h
    include/qml_material/helper.h
    include/qml_material/type_scale.h
    include/qml_material/kirigami/wheelhandler.h
    src/kirigami/wheelhandler.cpp
    src/type.cpp
    src/color.cpp
    src/corner.cpp
    src/icon.cpp
    src/input_block.cpp
    src/item_holder.cpp
    src/image.cpp
    src/util.cpp
    src/state.cpp
    src/theme.cpp
    src/helper.cpp
    src/type_scale.cpp
    src/round_item.cpp)
set(QML_FILES
    qml/Button.qml
    qml/Control.qml
    qml/ComboBox.qml
    qml/Dialog.qml
    qml/Divider.qml
    qml/Drawer.qml
    qml/DrawerItem.qml
    qml/Token.qml
    qml/IconLabel.qml
    qml/IconButton.qml
    qml/FAB.qml
    qml/Card.qml
    qml/CircularIndicator.qml
    qml/Menu.qml
    qml/MenuItem.qml
    qml/Image.qml
    qml/ListItem.qml
    qml/ListView.qml
    qml/Label.qml
    qml/FontMetrics.qml
    qml/Flickable.qml
    qml/GridView.qml
    qml/Icon.qml
    qml/Text.qml
    qml/Pane.qml
    qml/Page.qml
    qml/StackView.qml
    qml/SplitView.qml
    qml/Slider.qml
    qml/SearchBar.qml
    qml/ScrollBar.qml
    qml/Switch.qml
    qml/Rail.qml
    qml/TextInput.qml
    qml/TextField.qml
    qml/TabBar.qml
    qml/TabButton.qml
    qml/impl/BoxShadow.qml
    qml/impl/Shadow.qml
    qml/impl/ListBusyFooter.qml
    qml/impl/Ripple.qml
    qml/impl/RectangularGlow.qml
    qml/impl/CursorDelegate.qml
    qml/impl/State.qml
    qml/impl/SliderHandle.qml
    qml/impl/RoundedElevationEffect.qml
    qml/impl/ElevationEffect.qml
    qml/impl/TextFieldEmbed.qml
    qml/impl/RoundClip.qml)
set(RESOURCES assets/MaterialIconsRound-Regular.otf
              assets/MaterialIconsOutlined-Regular.otf)

set_source_files_properties(qml/Token.qml PROPERTIES QT_QML_SINGLETON_TYPE TRUE)
include_directories(include)

qt_add_qml_module(
  ${LIBNAME}
  URI
  Qcm.Material
  ${QM_BUILD}
  RESOURCE_PREFIX
  /
  QML_FILES
  ${QML_FILES}
  SOURCES
  ${SOURCES}
  RESOURCES
  ${RESOURCES}
  NO_PLUGIN_OPTIONAL)

set(SHADER_GLSLS "assets/shader/sdf.glsl")
set(SHADER_SOURCES 
  "assets/shader/round.frag"
  "assets/shader/shadow.frag"
  "assets/shader/rect_glow.frag"
)
qt_add_shaders(${LIBNAME} "shader" PREFIX "/Qcm/Material" FILES
               ${SHADER_SOURCES})
foreach(file ${SHADER_SOURCES})
  set(output_file ".qsb/${file}.qsb")
  add_custom_command(
    OUTPUT ${output_file}
    DEPENDS ${SHADER_GLSLS}
    APPEND)
endforeach()

target_include_directories(
  ${LIBNAME}
  PUBLIC include
  PRIVATE "include/${LIBNAME}" "include/${LIBNAME}/kirigami")

add_subdirectory(third_party)

target_link_libraries(
  ${LIBNAME}
  PUBLIC Qt6::Core Qt6::Quick Qt6::QuickControls2 Qt6::GuiPrivate core
  PRIVATE material_color)
