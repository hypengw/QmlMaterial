{
  description = "Example for QmlMaterial with Nix Flakes";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
    qmlmaterial.url = "github:moeleak/QmlMaterial";
    qmlmaterial.inputs.nixpkgs.follows = "nixpkgs";
  };

  outputs =
    {
      self,
      nixpkgs,
      qmlmaterial,
    }:
    let
      supportedSystems = [
        "x86_64-linux"
        "aarch64-linux"
        "x86_64-darwin"
        "aarch64-darwin"
      ];

      forAllSystems = nixpkgs.lib.genAttrs supportedSystems;
      nixpkgsFor = forAllSystems (system: import nixpkgs { inherit system; });
    in
    {
      packages = forAllSystems (
        system:
        let
          pkgs = nixpkgsFor.${system};
          qmLib = qmlmaterial.packages.${system}.default;
        in
        {
          default = pkgs.stdenv.mkDerivation {
            pname = "qml-material-demo";
            version = "0.0.1";
            src = ./.;

            nativeBuildInputs = with pkgs; [
              cmake
              ninja
              pkg-config
              qt6.wrapQtAppsHook
            ];

            buildInputs =
              (with pkgs.qt6; [
                qtbase
                qtdeclarative
                qtshadertools
              ])
              ++ [ qmLib ];

            cmakeFlags = [
              "-DCMAKE_BUILD_TYPE=Release"
              "-DCMAKE_INSTALL_RPATH=${qmLib}/lib"
              "-DCMAKE_INSTALL_RPATH_USE_LINK_PATH=ON"
            ];

            qtWrapperArgs = [
              "--prefix"
              "QML_IMPORT_PATH"
              ":"
              "$out/lib/qt-6/qml:${qmLib}/lib/qt-6/qml"
              "--prefix"
              "QML2_IMPORT_PATH"
              ":"
              "$out/lib/qt-6/qml:${qmLib}/lib/qt-6/qml"
              "--prefix"
              "DYLD_LIBRARY_PATH"
              ":"
              "${qmLib}/lib"
              "--prefix"
              "LD_LIBRARY_PATH"
              ":"
              "${qmLib}/lib"
            ];
          };
        }
      );

      apps = forAllSystems (
        system:
        {
          default = {
            type = "app";
            program = "${self.packages.${system}.default}/bin/qml-material-demo";
          };
        }
      );

      devShells = forAllSystems (
        system:
        let
          pkgs = nixpkgsFor.${system};
        in
        {
          default = pkgs.mkShell {
            inputsFrom = [ self.packages.${system}.default ];
          };
        }
      );
    };
}
