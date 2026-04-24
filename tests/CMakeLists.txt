if(NOT TARGET Qt6::Quick)
  find_package(Qt6 REQUIRED COMPONENTS Core Gui Quick Qml)
endif()
if(NOT TARGET Qt6::GuiPrivate)
  find_package(Qt6 REQUIRED COMPONENTS GuiPrivate)
endif()

enable_testing()

add_executable(qm_grab qm_grab.cpp)
target_link_libraries(qm_grab PRIVATE Qt6::Core Qt6::Gui Qt6::GuiPrivate Qt6::Quick Qt6::Qml)
set_target_properties(qm_grab PROPERTIES AUTOMOC ON)

set(VISUAL_SCENES button elevation shape)
file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/out)
foreach(scene ${VISUAL_SCENES})
  add_test(
    NAME visual_${scene}
    COMMAND bash -c
      "LIBGL_ALWAYS_SOFTWARE=1 \
       QT_QPA_PLATFORM=eglfs \
       QML_IMPORT_PATH=${CMAKE_BINARY_DIR}/qml_modules \
       $<TARGET_FILE:qm_grab> \
         ${CMAKE_CURRENT_SOURCE_DIR}/scenes/${scene}.qml \
         ${CMAKE_CURRENT_BINARY_DIR}/out/visual_${scene}.png \
       && test -s ${CMAKE_CURRENT_BINARY_DIR}/out/visual_${scene}.png"
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  )
endforeach()
