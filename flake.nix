{
  description = "QmlMaterial Development Environment";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
  };

  outputs =
    { self, nixpkgs }:
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
      # 1. Build rules for "nix build"
      packages = forAllSystems (
        system:
        let
          pkgs = nixpkgsFor.${system};
        in
        {
          default = pkgs.stdenv.mkDerivation {
            pname = "QmlMaterial";
            version = "0.1.4";

            # Only drop the example-nix directory; keep everything else intact.
            src = pkgs.lib.cleanSourceWith {
              src = ./.;
              filter =
                path: type:
                let
                  rel = pkgs.lib.removePrefix (toString ./. + "/") (toString path);
                in
                !(pkgs.lib.hasPrefix "example-nix" rel);
            };

            # Build-time dependencies
            nativeBuildInputs = with pkgs; [
              cmake
              ninja
              pkg-config
              qt6.wrapQtAppsHook # Wrap to expose Qt plugin/QML paths
            ];

            # Runtime dependencies (Libraries)
            buildInputs =
              (with pkgs.qt6; [
                qtbase
                qtdeclarative
                qtshadertools
                qtsvg
                qttools
                qt5compat
              ])
              ++ (if pkgs.stdenv.isLinux then [ pkgs.qt6.qtwayland ] else [ ]);

            configurePhase = ''
              runHook preConfigure
              cmake -S . -B build-nix \
                -DCMAKE_BUILD_TYPE=Release \
                -DQM_BUILD_EXAMPLE=ON
              runHook postConfigure
            '';
            buildPhase = ''
              runHook preBuild
              cmake --build build-nix -- -j''${NIX_BUILD_CORES:-1}
              runHook postBuild
            '';
            installPhase = ''
              runHook preInstall
              cmake --install build-nix --prefix $out
              runHook postInstall
            '';

            qtWrapperArgs = [
              "--prefix"
              "QML_IMPORT_PATH"
              ":"
              "$out/lib/qt-6/qml"
              "--prefix"
              "QML2_IMPORT_PATH"
              ":"
              "$out/lib/qt-6/qml"
            ];
          };
        }
      );

      # 2. Development environment for "nix develop"
      devShells = forAllSystems (
        system:
        let
          pkgs = nixpkgsFor.${system};
        in
        {
          default = pkgs.mkShell {
            name = "qmlmaterial-dev-shell";

            # Inherit dependencies from the package definition above
            # This ensures dev env matches build env exactly
            inputsFrom = [ self.packages.${system}.default ];

            packages = with pkgs; [
              gdb
              clang-tools # clangd, clang-format
            ];

            CMAKE_EXPORT_COMPILE_COMMANDS = "1";
          };
        }
      );

      # 3. "nix run"
      apps = forAllSystems (
        system:
        let
          pkg = self.packages.${system}.default;
        in
        {
          default = {
            type = "app";
            program = "${pkg}/bin/qm_example";
          };
        }
      );
    };
}
