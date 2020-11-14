{
  description = "Pingfs";

  inputs = {
    flake-utils.url = "github:numtide/flake-utils";
    nixpkgs.url = "github:Nixos/nixpkgs/nixos-20.09";
  };

  outputs = { self, nixpkgs, flake-utils }:
    flake-utils.lib.eachDefaultSystem (system:
    let 
      pkgs = nixpkgs.legacyPackages.${system};
      buildInputs = with pkgs; [
        gnumake
        gcc
      ];
    in {
      devShell = pkgs.mkShell {
        buildInputs = buildInputs;
      };

      defaultPackage = with pkgs; stdenv.mkDerivation {
        name = "pingfs";
        src = self;
        buildInputs = buildInputs;
        buildPhase = "make build";
        installPhase = "mkdir -p $out/bin; install -t $out/bin pingfs";
      };
    }
  );
}
