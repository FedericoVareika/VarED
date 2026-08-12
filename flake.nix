{
    description = "VarED Development Environment";

    inputs = {
        nixpkgs.url = "github:nixos/nixpkgs/nixos-unstable";
    };

    outputs = { self, nixpkgs }:
    let
      system = "x86_64-linux";
      pkgs = nixpkgs.legacyPackages.${system};
    in
    {
      devShells.${system}.default = pkgs.mkShell {
        packages = with pkgs; [
          gcc
          SDL2
          SDL2.dev
          libGL
          pkg-config
          ctags

          freetype

          renderdoc
          sdl3
        ];

        shellHook = ''
          export LD_LIBRARY_PATH="/run/opengl-driver/lib:${pkgs.lib.makeLibraryPath [ pkgs.libGL pkgs.sdl3 ]}:$LD_LIBRARY_PATH"
        '';
      };
    };
}
