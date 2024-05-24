{
  description = "A nix flake for the Yosys synthesis suite";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
    flake-utils.url = "github:numtide/flake-utils";
  };

  outputs = { self, nixpkgs, flake-utils }: flake-utils.lib.eachDefaultSystem (system:
    let
      pkgs = import nixpkgs {
        inherit system;
      };

      commonAttrs = { doCheck ? false, sanitizer ? "", sanitizerFlags ? "" }: {
        src = ./.;
        checkInputs = with pkgs; [ gtest ];
        propagatedBuildInputs = [ abc-verifier ];
        installPhase = ''
          make install PREFIX=$out ABCEXTERNAL=yosys-abc
          ln -s ${abc-verifier}/bin/abc $out/bin/yosys-abc
        '';
        meta = with pkgs.lib; {
          description = "Yosys Open SYnthesis Suite";
          homepage = "https://yosyshq.net/yosys/";
          license = licenses.isc;
          maintainers = with maintainers; [ ];
        };
        doCheck = doCheck;
        dontStrip = true;  # Ensure that debugging symbols are not stripped
      };

      abc-verifier = pkgs.abc-verifier.overrideAttrs (x: y: { src = ./abc; });

      yosys = pkgs.clangStdenv.mkDerivation (commonAttrs {
        doCheck = false;
      } // {
        name = "yosys";
        buildInputs = with pkgs; [ clang bison flex libffi tcl readline python3 llvmPackages.libcxxClang zlib git pkg-configUpstream gcc ];
        preConfigure = "make config-clang";
        checkTarget = "test";
        buildPhase = ''
          export CXXFLAGS="$CXXFLAGS -g"  # Enable debug symbols
          export LDFLAGS="-L${pkgs.gcc.libc}/lib"
          make -j$(nproc) ABCEXTERNAL=yosys-abc
        '';
      });

      gcc-yosys = pkgs.gccStdenv.mkDerivation (commonAttrs {
        doCheck = false;
      } // {
        name = "gcc-yosys";
        buildInputs = with pkgs; [ gcc bison flex libffi tcl readline python3 llvmPackages.libcxx zlib git pkg-configUpstream ];
        preConfigure = "make config-gcc";
        checkTarget = "test";
        buildPhase = ''
          export CXXFLAGS="$CXXFLAGS -g"  # Enable debug symbols
          make -j$(nproc) ABCEXTERNAL=yosys-abc
        '';
      });

      yosysWithSanitizers = sanitizer: pkgs.clangStdenv.mkDerivation (commonAttrs {
        doCheck = true;
        sanitizer = sanitizer;
        sanitizerFlags = if sanitizer == "memory" then "-fsanitize-memory-track-origins=2" else "";
      } // {
        name = "yosys-with-${sanitizer}";
        buildInputs = with pkgs; [ clang bison flex libffi tcl readline python3 llvmPackages.libcxxClang llvmPackages.libstdcxxClang zlib git pkg-configUpstream ];
        preConfigure = "make config-clang";
        checkTarget = "test";
        buildPhase = ''
          export CXXFLAGS="$CXXFLAGS -g -fsanitize=${sanitizer} ${if sanitizer == "memory" then "-fsanitize-memory-track-origins=2" else ""}"  # Enable debug symbols and sanitizer flags
          export LDFLAGS="-L${pkgs.gcc.libc}/lib"
          make -j$(nproc) ABCEXTERNAL=yosys-abc SANITIZER=${sanitizer}
        '';
        checkPhase = ''
          export CXXFLAGS="$CXXFLAGS -g -fsanitize=${sanitizer} ${if sanitizer == "memory" then "-fsanitize-memory-track-origins=2" else ""}"  # Enable debug symbols and sanitizer flags
          export LDFLAGS="-L${pkgs.gcc.libc}/lib"
          make test SANITIZER=${sanitizer}
        '';
        doCheck = false;
      });

      yosysWithMultipleSanitizers = pkgs.clangStdenv.mkDerivation (commonAttrs {
        doCheck = true;
      } // {
        name = "yosys-with-multiple-sanitizers";
        buildInputs = with pkgs; [ clang bison flex libffi tcl readline python3 llvmPackages.libcxxClang zlib git pkg-configUpstream gcc ];
        preConfigure = "make config-clang";
        checkTarget = "test";
        buildPhase = ''
          export CXXFLAGS="$CXXFLAGS -g -fsanitize=address,undefined"  # Enable debug symbols and multiple sanitizers
          export LDFLAGS="-L${pkgs.gcc.libc}/lib"
          make -j$(nproc) ABCEXTERNAL=yosys-abc SANITIZER="address,undefined"
        '';
        checkPhase = ''
          export CXXFLAGS="$CXXFLAGS -g -fsanitize=address,undefined"  # Enable debug symbols and multiple sanitizers
          export LDFLAGS="-L${pkgs.gcc.libc}/lib"
          make test SANITIZER="address,undefined"
        '';
      });

      sanitizers = [ "address" "memory" "undefined" "cfi" ];
      sanitizerPackages = map (sanitizer: {
        name = "yosys-with-${sanitizer}";
        value = yosysWithSanitizers sanitizer;
      }) sanitizers;

    in {
      packages = flake-utils.lib.flattenTree (builtins.listToAttrs ([
        { name = "yosys"; value = yosys; }
        { name = "gcc-yosys"; value = gcc-yosys; }
        { name = "yosys-with-multiple-sanitizers"; value = yosysWithMultipleSanitizers; }
      ] ++ sanitizerPackages));
      defaultPackage = yosys;
      devShells = {
        default = pkgs.mkShell {
          buildInputs = with pkgs; [ clang bison flex libffi tcl readline python3 llvmPackages.libcxxClang zlib git gtest abc-verifier ];
          LOCALE_ARCHIVE = "${pkgs.glibcLocales}/lib/locale/locale-archive";
        };
      };
      
      checks = {
        makeTest = pkgs.runCommand "make-test" {
          buildInputs = [ pkgs.makeWrapper ];
        } ''
          cp -r ${self}/* .
          export PATH=$PATH:${pkgs.make}/bin
          make test
        '';
      };
    }
  );
}
