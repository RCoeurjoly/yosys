{
  description = "A nix flake for the Yosys synthesis suite";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
    flake-utils.url = "github:numtide/flake-utils";
    abc-src = {
      # Replace "rev" and "sha256" with the actual values obtained from nix-prefetch-git
      url = "github:YosysHQ/abc/yosys-experimental";
      flake = false;
    };
  };

  outputs = { self, nixpkgs, flake-utils, abc-src }:
    flake-utils.lib.eachDefaultSystem (system:
      let
        pkgs = import nixpkgs {
          inherit system;
        };
        customYosys = pkgs.clangStdenv.mkDerivation {
          name = "yosys";
          pname = "yosys";
          version = "0.35";
          src = ./yosys;
          buildInputs = with pkgs; [ clang bison flex libffi tcl readline python3 llvmPackages.libcxxClang zlib git ];
          checkInputs = with pkgs; [ gtest ];
          # propagatedBuildInputs = with pkgs; [ abc-verifier ];
          preBuild = ''
        cp -r ${abc-src}/* abc/
      '';
          preConfigure = "make config-clang";
          checkTarget = "test";
          installPhase = ''
            make install PREFIX=$out
          '';
          buildPhase = ''
          make -j$(nproc)
          '';
          meta = with pkgs.lib; {
            description = "Yosys Open SYnthesis Suite";
            homepage = "https://yosyshq.net/yosys/";
            license = licenses.isc;
            maintainers = with maintainers; [ ];
          };
        };
      in {
        packages.default = customYosys;
        defaultPackage = customYosys;
        devShell = pkgs.mkShell {
          buildInputs = with pkgs; [ clang bison flex libffi tcl readline python3 llvmPackages.libcxxClang zlib git gtest abc-verifier nix-update-source ];
        };
      }
    );
}
