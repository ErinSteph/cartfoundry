#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
BUILD_DIR=${1:-build}
QT_PREFIX=${QT_PREFIX:-/opt/homebrew/opt/qt}
cmake --build "$BUILD_DIR" --target CartFoundry cf-build --parallel
mkdir -p dist
# Replace only our generated application bundle.
rm -rf dist/CartFoundry.app
cp -RX "$BUILD_DIR/CartFoundry.app" dist/CartFoundry.app
RES=dist/CartFoundry.app/Contents/Resources
mkdir -p "$RES/tools" "$RES/examples"
cp -RX runtime images docs "$RES/"
cp -RX tools/gbdk "$RES/tools/"
cp -RX examples/first-conversation examples/overworld examples/pokemon examples/scenes "$RES/examples/"
cp THIRD_PARTY.md "$RES/"
cp -RX licenses "$RES/"
COPYFILE_DISABLE=1 tar -czf "$RES/licenses/mgba/source-0.10.5.tar.gz" -C third_party mgba
cp "$BUILD_DIR/cf-build" dist/CartFoundry.app/Contents/MacOS/cf-build
# Copy the native window, image and audio plugins used by this editor.
PLUGINS=dist/CartFoundry.app/Contents/PlugIns
mkdir -p "$PLUGINS/platforms" "$PLUGINS/imageformats" "$PLUGINS/multimedia"
cp -X "$QT_PREFIX/share/qt/plugins/platforms/libqcocoa.dylib" "$PLUGINS/platforms/"
for PLUGIN in qjpeg qgif qico; do
    cp -X "$QT_PREFIX/share/qt/plugins/imageformats/lib$PLUGIN.dylib" "$PLUGINS/imageformats/"
done
cp -X "$QT_PREFIX/share/qt/plugins/multimedia/libdarwinmediaplugin.dylib" "$PLUGINS/multimedia/"
"$QT_PREFIX/bin/macdeployqt" dist/CartFoundry.app -always-overwrite -no-plugins -no-codesign -libpath="$QT_PREFIX/lib" -executable=dist/CartFoundry.app/Contents/MacOS/cf-build -executable="$PLUGINS/platforms/libqcocoa.dylib" -executable="$PLUGINS/multimedia/libdarwinmediaplugin.dylib" -executable="$PLUGINS/imageformats/libqjpeg.dylib" -executable="$PLUGINS/imageformats/libqgif.dylib" -executable="$PLUGINS/imageformats/libqico.dylib"
printf '[Paths]\nPlugins = PlugIns\n' > "$RES/qt.conf"
chmod u+w dist/CartFoundry.app/Contents/Info.plist
# The icon artwork remains in the repository; derive the platform icon at packaging time.
mkdir -p "$BUILD_DIR/CartFoundry.iconset"
for SIZE in 16 32 128 256 512; do
    sips -z "$SIZE" "$SIZE" images/cf_icon.png --out "$BUILD_DIR/CartFoundry.iconset/icon_${SIZE}x${SIZE}.png" >/dev/null
    DOUBLE=$((SIZE * 2))
    sips -z "$DOUBLE" "$DOUBLE" images/cf_icon.png --out "$BUILD_DIR/CartFoundry.iconset/icon_${SIZE}x${SIZE}@2x.png" >/dev/null
done
iconutil -c icns "$BUILD_DIR/CartFoundry.iconset" -o "$RES/CartFoundry.icns"
/usr/libexec/PlistBuddy -c 'Set :CFBundleIconFile CartFoundry.icns' dist/CartFoundry.app/Contents/Info.plist
xattr -cr dist/CartFoundry.app
codesign --force --deep --sign - dist/CartFoundry.app
codesign --verify --deep --strict dist/CartFoundry.app
printf 'Packaged: %s/dist/CartFoundry.app\n' "$PWD"
