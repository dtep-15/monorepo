{ pkgs ? import <nixpkgs> {} }:
pkgs.mkShell {
	nativeBuildInputs = with pkgs.buildPackages; [
		lld
		clang
		trunk
		dart-sass
	];

	shellHook = ''
		export CC=clang
	'';
}
