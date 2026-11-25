# FFmpeg in the Windows package

Qt 6.8.3 supplies unmodified FFmpeg 7.1 shared libraries (libavcodec 61,
libavformat 61, libavutil 59, libswresample 5 and libswscale 8), built under
LGPL version 2.1 or later. Copyright belongs to the FFmpeg developers.
The libraries remain dynamically linked and replaceable.

Corresponding source: https://ffmpeg.org/releases/ffmpeg-7.1.tar.xz
Upstream source tag: https://github.com/FFmpeg/FFmpeg/tree/n7.1

The supplied libraries report this build configuration:

```
--prefix=installed --disable-programs --disable-doc --disable-debug
--enable-network --disable-lzma --enable-pic --disable-vulkan
--disable-v4l2-m2m --disable-decoder=truemotion1 --toolchain=msvc
--enable-shared --disable-static
```
