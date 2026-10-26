#!/usr/bin/env bash
# Build and run the game. Works from any folder: path/to/repo/run.sh
#
# Maekfile.js expects nest-libs at ../nest-libs/<os>, so if that is missing it
# is downloaded next to the repo first.

set -euo pipefail

NEST_VERSION="v0.15" #same as NEST_LIBS_VERSION in .github/workflows/build-workflow.yml

die() {
	echo "" >&2
	echo "ERROR: $1" >&2
	shift
	for line in "$@"; do echo "  $line" >&2; done
	exit 1
}

case "$(uname -s)" in
	Darwin) OS=macos; CXX=clang++ ;;
	Linux) OS=linux; CXX=g++ ;;
	*) die "Unsupported system '$(uname -s)'." "This script handles macOS and Linux. On Windows, build by hand as described in NEST.md." ;;
esac

command -v node >/dev/null || die "Node.js is not installed." "The build tool (Maekfile.js) runs on Node.js. Install it from https://nodejs.org and run this again."
if ! command -v $CXX >/dev/null || ! $CXX --version >/dev/null 2>&1; then
	if [ $OS = macos ]; then
		die "No C++ compiler ($CXX) found." "Install Apple's command line tools with: xcode-select --install"
	else
		die "No C++ compiler ($CXX) found." "Install it with your package manager, e.g.: sudo apt install g++"
	fi
fi

REPO="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PARENT="$(dirname "$REPO")"
NEST="$PARENT/nest-libs"

if [ ! -d "$NEST/$OS" ]; then
	URL="https://github.com/15-466/nest-libs/releases/download/$NEST_VERSION/nest-libs-$OS-$NEST_VERSION.tar.gz"
	echo "nest-libs not found at $NEST/$OS, downloading $NEST_VERSION (about 50 MB) ..."
	[ -w "$PARENT" ] || die "Cannot write to $PARENT" "nest-libs has to go next to the repo, at $NEST"
	TMP="$(mktemp -d)"
	curl -fL --progress-bar -o "$TMP/nest-libs.tar.gz" "$URL" || die "Could not download nest-libs from:" "$URL" "Check your internet connection, or download it by hand and unpack it so that $NEST/$OS exists."
	tar xzf "$TMP/nest-libs.tar.gz" -C "$PARENT" || die "Could not unpack nest-libs into $PARENT"
	rm -rf "$TMP"
	[ -d "$NEST/$OS" ] || die "nest-libs was unpacked, but $NEST/$OS is still missing." "Unpack $URL by hand so that $NEST/$OS exists."
elif [ ! -e "$NEST/$NEST_VERSION" ]; then
	echo "WARNING: $NEST does not look like version $NEST_VERSION. If the build fails, delete it and run this again."
fi

cd "$REPO"
echo "Building in $REPO ..."
#only build the game; the default targets include freetype-test, whose source is not in the repo
node Maekfile.js dist/game || die "The build failed (see the compiler output above)." "If it complains about missing headers or libraries, delete $NEST and run this again to re-download it."

echo "Starting the game ..."
cd dist
exec ./game
