{
    inputs = {
        nixpkgs.url = "github:nixos/nixpkgs/nixos-25.11";
    };

    outputs = {
        nixpkgs,
        ...
    }: let
        system = "x86_64-linux";
    in {
        devShells."${system}".default = let
            pkgs = import nixpkgs {
                inherit system;
            };
        in (pkgs.mkShell.override { stdenv = pkgs.gcc15Stdenv; }) {
            buildInputs = with pkgs; [
                cli11
                lief
                elfio
                elfutils # for libelf
            ];
            nativeBuildInputs = with pkgs; [
                cmake
            ];
            shellHook = ''
            export CPATH="${pkgs.lief}/include:${pkgs.elfio}/include:${pkgs.cli11}/include:${pkgs.elfutils.dev}/include"
            '';
        };
    };
}
