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
        in {
          autotests = package.overrideAttrs (old: {
            pname = "${old.pname}-tests";
            cmakeFlags = (old.cmakeFlags or [ ]) ++ [
              "-DBUILD_TESTING=ON"
              "-DLATTE_STRICT_WARNINGS=ON"
            ];
            buildInputs = (old.buildInputs or [ ]) ++ [
              pkgs.kdePackages.breeze-icons
            ];
            doCheck = true;
            buildPhase = ''
              runHook preBuild
              cmake --build . --parallel "$NIX_BUILD_CORES" \
                --target latte-dock-ng latteprivateappplugin latte-autotests
              runHook postBuild
            '';
            checkPhase = ''
              runHook preCheck
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
