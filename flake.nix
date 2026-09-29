{
  description = "Latte Dock NG: a Wayland-first dock for KDE Plasma 6.3+";

  inputs.nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";

  outputs = { self, nixpkgs }:
    let
      forAllSystems = nixpkgs.lib.genAttrs [ "x86_64-linux" ];
    in {
      packages = forAllSystems (system:
        let pkgs = nixpkgs.legacyPackages.${system};
        in { default = import ./default.nix { inherit pkgs; }; });

      checks = forAllSystems (system:
        let
          pkgs = nixpkgs.legacyPackages.${system};
          package = import ./default.nix { inherit pkgs; };
          testInputs = (package.buildInputs or [ ]) ++ [
            pkgs.kdePackages.breeze-icons
            pkgs.fontconfig
            pkgs.dejavu_fonts
          ];
        in {
          autotests = package.overrideAttrs (old: {
            pname = "${old.pname}-tests";
            cmakeFlags = (old.cmakeFlags or [ ]) ++ [
              "-DBUILD_TESTING=ON"
              "-DLATTE_STRICT_WARNINGS=ON"
            ];
            buildInputs = testInputs;
            doCheck = true;
            buildPhase = ''
              runHook preBuild
              cmake --build . --parallel "$NIX_BUILD_CORES" \
                --target latte-dock-ng latteprivateappplugin latte-autotests
              runHook postBuild
            '';
            checkPhase = ''
              runHook preCheck
              mkdir -p "$TMPDIR/bin" "$TMPDIR/font-cache"
              cat > "$TMPDIR/bin/dbus-run-session" <<'WRAPPER'
              #!/bin/sh
              exec ${pkgs.dbus}/bin/dbus-run-session --config-file=${pkgs.dbus}/share/dbus-1/session.conf "$@"
              WRAPPER
              chmod +x "$TMPDIR/bin/dbus-run-session"
              export PATH="$TMPDIR/bin:$PATH"
              export QML2_IMPORT_PATH="${pkgs.lib.makeSearchPath "lib/qt-6/qml" testInputs}''${QML2_IMPORT_PATH:+:$QML2_IMPORT_PATH}"
              export QML_IMPORT_PATH="$QML2_IMPORT_PATH"
              export QT_PLUGIN_PATH="${pkgs.lib.makeSearchPath "lib/qt-6/plugins" testInputs}''${QT_PLUGIN_PATH:+:$QT_PLUGIN_PATH}"
              export FONTCONFIG_FILE="${pkgs.makeFontsConf { fontDirectories = [ pkgs.dejavu_fonts ]; }}"
              export XDG_CACHE_HOME="$TMPDIR/font-cache"
              export LATTE_TEST_ICON_THEME_PATH="${pkgs.kdePackages.breeze-icons}/share/icons"
              QT_QPA_PLATFORM=offscreen QT_QUICK_BACKEND=software \
                dbus-run-session -- ctest --output-on-failure
              runHook postCheck
            '';
            installPhase = ''
              mkdir -p "$out"
            '';
          });
        });

      devShells = forAllSystems (system:
        let
          pkgs = nixpkgs.legacyPackages.${system};
          package = import ./default.nix { inherit pkgs; };
        in {
          default = pkgs.mkShell {
            inputsFrom = [ package ];
            packages = [
              pkgs.clang
              pkgs.dbus
              pkgs.gnumake
              pkgs.python3
            ];
          };
        });

      overlays.default = final: prev: {
        latte-dock-ng = import ./default.nix { pkgs = final; };
      };

      nixosModules.default = { ... }: {
        nixpkgs.overlays = [ self.overlays.default ];
      };
    };
}
