{
  description = "Latte Dock NG: a Wayland-first dock for KDE Plasma 6.5+";

  inputs.nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";

  outputs = { self, nixpkgs }:
    let
      forAllSystems = nixpkgs.lib.genAttrs [ "x86_64-linux" ];
    in {
      packages = forAllSystems (system:
        let pkgs = nixpkgs.legacyPackages.${system};
        in { default = import ./default.nix { inherit pkgs; }; });

      overlays.default = final: prev: {
        latte-dock-ng = import ./default.nix { pkgs = final; };
      };

      nixosModules.default = { ... }: {
        nixpkgs.overlays = [ self.overlays.default ];
      };
    };
}
