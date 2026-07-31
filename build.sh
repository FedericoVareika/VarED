#!/bin/sh

ignore_warning_flags="-Wno-missing-field-initializers"

common_flags_internal="-DVARED_SLOW=1 -DVARED_INTERNAL=1 -ffile-prefix-map=old=new -g -W -pedantic" # -fsanitize=address"
common_flags_external="-DVARED_SLOW=0 -DVARED_INTERNAL=0 -ffile-prefix-map=old=new -g -W -O3 -pedantic"

pkgs="sdl2 glew freetype2"
linker_flags="-lm -ldl"

glew_libdir="$(pkg-config --variable=libdir glew)"

sdl2_include="$(pkg-config --variable=includedir sdl2)/SDL2"
freetype_include="$(pkg-config --variable=includedir freetype2)/freetype2"

ctags --recurse=yes \
    --exclude=.git \
    --exclude=build \
    --exclude=data \
    --exclude=vendored \
    --c++-kinds=+p \
    . $sdl2_include $freetype_include

mkdir -p build

# gcc -std=gnu11 \
#     $common_flags_internal \
#     $ignore_warning_flags \
#     -shared \
#     -o build/vared.so \
#     -fPIC $(realpath src/vared.c) \
#     $linker_flags 
#
# gcc -std=gnu11 \
#     $common_flags_internal \
#     $(realpath src/linux_vared.c) \
#     $(pkg-config --cflags $pkgs) \
#     -o build/vared \
#     $(pkg-config --libs $pkgs) \
#     $linker_flags -Wl,-rpath,$(realpath build):$glew_libdir

gcc -std=gnu11 \
    $common_flags_internal \
    $(realpath src/linux_vared.c) \
    $(pkg-config --cflags $pkgs) \
    -o build/vared \
    $(pkg-config --libs $pkgs) \
    $linker_flags -Wl,-rpath,$(realpath build):$glew_libdir

