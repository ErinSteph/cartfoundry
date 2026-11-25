#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
BUILD_DIR=${1:-build}
QT_PREFIX=${QT_PREFIX:-/opt/homebrew/opt/qt}
DIST_DIR=${DIST_DIR:-dist}
GBDK_ROOT=${GBDK_ROOT:-tools/gbdk}
QT_PLUGINS=$("$QT_PREFIX/bin/qmake" -query QT_INSTALL_PLUGINS)
cmake --build "$BUILD_DIR" --target CartFoundry cf-build cf-update-helper --parallel
mkdir -p "$DIST_DIR"
# Replace only our generated application bundle.
rm -rf "$DIST_DIR/CartFoundry.app"
cp -RX "$BUILD_DIR/CartFoundry.app" "$DIST_DIR/CartFoundry.app"
RES="$DIST_DIR/CartFoundry.app/Contents/Resources"
mkdir -p "$RES/tools" "$RES/examples"
cp -RX runtime images docs "$RES/"
mkdir -p "$RES/docs/versions"
cp -RX "$BUILD_DIR/documentation/." "$RES/docs/versions/"
cp -RX "$GBDK_ROOT" "$RES/tools/gbdk"
cp -RX examples/first-conversation examples/overworld examples/pokemon examples/scenes examples/sound-studio examples/menu-saves "$RES/examples/"
cp THIRD_PARTY.md "$RES/"
cp -RX licenses "$RES/"
COPYFILE_DISABLE=1 tar -czf "$RES/licenses/mgba/source-0.10.5.tar.gz" -C third_party mgba
cp "$BUILD_DIR/cf-build" "$DIST_DIR/CartFoundry.app"/Contents/MacOS/cf-build
cp "$BUILD_DIR/cf-update-helper" "$DIST_DIR/CartFoundry.app"/Contents/MacOS/cf-update-helper
ARCH=$(lipo -archs "$DIST_DIR/CartFoundry.app"/Contents/MacOS/CartFoundry)
case "$ARCH" in arm64|x86_64) ;; *) echo "Updater packages require a single-architecture build." >&2; exit 1;; esac
python3 scripts/make-update.py stamp --root "$DIST_DIR/CartFoundry.app" --platform "macos-$ARCH"
# Copy the native window, image and audio plugins used by this editor.
PLUGINS="$DIST_DIR/CartFoundry.app/Contents/PlugIns"
mkdir -p "$PLUGINS/platforms" "$PLUGINS/imageformats" "$PLUGINS/multimedia" "$PLUGINS/tls"
cp -X "$QT_PLUGINS/platforms/libqcocoa.dylib" "$PLUGINS/platforms/"
for PLUGIN in qjpeg qgif qico; do
    cp -X "$QT_PLUGINS/imageformats/lib$PLUGIN.dylib" "$PLUGINS/imageformats/"
done
cp -X "$QT_PLUGINS/multimedia/libdarwinmediaplugin.dylib" "$PLUGINS/multimedia/"
cp -X "$QT_PLUGINS/tls/libqsecuretransportbackend.dylib" "$PLUGINS/tls/"
"$QT_PREFIX/bin/macdeployqt" "$DIST_DIR/CartFoundry.app" -always-overwrite -no-plugins -no-codesign -libpath="$QT_PREFIX/lib" -executable="$DIST_DIR/CartFoundry.app"/Contents/MacOS/cf-build -executable="$DIST_DIR/CartFoundry.app"/Contents/MacOS/cf-update-helper -executable="$PLUGINS/tls/libqsecuretransportbackend.dylib" -executable="$PLUGINS/platforms/libqcocoa.dylib" -executable="$PLUGINS/multimedia/libdarwinmediaplugin.dylib" -executable="$PLUGINS/imageformats/libqjpeg.dylib" -executable="$PLUGINS/imageformats/libqgif.dylib" -executable="$PLUGINS/imageformats/libqico.dylib"
printf '[Paths]\nPlugins = PlugIns\n' > "$RES/qt.conf"
chmod u+w "$DIST_DIR/CartFoundry.app"/Contents/Info.plist
# The icon artwork remains in the repository; derive the platform icon at packaging time.
mkdir -p "$BUILD_DIR/CartFoundry.iconset"
for SIZE in 16 32 128 256 512; do
    sips -z "$SIZE" "$SIZE" images/cf_icon.png --out "$BUILD_DIR/CartFoundry.iconset/icon_${SIZE}x${SIZE}.png" >/dev/null
    DOUBLE=$((SIZE * 2))
    sips -z "$DOUBLE" "$DOUBLE" images/cf_icon.png --out "$BUILD_DIR/CartFoundry.iconset/icon_${SIZE}x${SIZE}@2x.png" >/dev/null
done
iconutil -c icns "$BUILD_DIR/CartFoundry.iconset" -o "$RES/CartFoundry.icns"
/usr/libexec/PlistBuddy -c 'Set :CFBundleIconFile CartFoundry.icns' "$DIST_DIR/CartFoundry.app"/Contents/Info.plist
xattr -cr "$DIST_DIR/CartFoundry.app"
python3 scripts/thin-macos.py "$DIST_DIR/CartFoundry.app" "$ARCH"
codesign --force --deep --sign - "$DIST_DIR/CartFoundry.app"
codesign --verify --deep --strict "$DIST_DIR/CartFoundry.app"
printf 'Packaged: %s/%s/CartFoundry.app\n' "$PWD" "$DIST_DIR"

python3 scripts/make-update.py archive --root "$DIST_DIR/CartFoundry.app" --output "$DIST_DIR"
