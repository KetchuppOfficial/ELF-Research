{
    inputs = {
        nixpkgs.url = "github:nixos/nixpkgs/nixos-26.05";
    };

    outputs = {
        nixpkgs,
        ...
    }: let
        system = "x86_64-linux";
        pkgs = import nixpkgs {
            inherit system;
        };

        elfio-pinned = pkgs.elfio.overrideAttrs (old: {
            version = "unstable-660e905";
            src = pkgs.fetchFromGitHub {
                owner = "serge1";
                repo = "ELFIO";
                rev = "660e90514a2e80fba1c55f2cee07bbeda3d1c601";
                hash = "sha256-fqKOLF7JWTyUnxtUupTguRVsuRa64OcHAgJsuwSIfgA=";
            };
            patches = [];
        });

        mkDevShell = elfio: (pkgs.mkShell.override { stdenv = pkgs.gcc16Stdenv; }) {
            buildInputs = [
                elfio
            ] ++ (with pkgs; [
                cli11
                lief
                elfutils # for libelf
                gbenchmark
                llvm
                nlohmann_json
                nlohmann_json_schema_validator
            ]);
            nativeBuildInputs = with pkgs; [
                cmake
                ninja
                clang-tools
                jq
                python314
            ];
            shellHook = ''
                bash scripts/add_to_local_settings.sh ${pkgs.clang-tools}/bin/clangd
            '';
        };
    in {
        devShells."${system}" = {
            default = mkDevShell pkgs.elfio;
            elfio-pinned = mkDevShell elfio-pinned;
        };
    };
}
