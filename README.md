# Material Design 3 for Qml
This is a library of Qml implementing Google's Material Design.

## 🌐 [Live Demo](https://hypengw.github.io/QmlMaterialWeb/qmaterial_example.html)

### Dependencies

- Qt 6.8+

## Installation
```cmake
FetchContent_Declare(
  qml_material
  GIT_REPOSITORY https://github.com/hypengw/QmlMaterial.git
  GIT_TAG main
  # GIT_TAG <commit>
  EXCLUDE_FROM_ALL)
FetchContent_MakeAvailable(qml_material)
```

### Acknowledgements
- [material-color-utilities](https://github.com/material-foundation/material-color-utilities)
- [kirigmai](https://invent.kde.org/frameworks/kirigami)
