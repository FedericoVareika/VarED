#!/usr/bin/bash

ignore_warning_flags="-Wno-missing-field-initializers -Wno-override-init -Wno-override-init-side-effects"

common_flags_internal="-DVARED_SLOW=1 -DVARED_INTERNAL=1 -ffile-prefix-map=old=new -g -W -pedantic" # -fsanitize=address"
common_flags_external="-DVARED_SLOW=0 -DVARED_INTERNAL=0 -ffile-prefix-map=old=new -g -W -O3 -pedantic"

pkgs="sdl2 glew freetype2"
linker_flags="-lm -ldl"

glew_libdir="$(pkg-config --variable=libdir glew)"

sdl2_include="$(pkg-config --variable=includedir sdl2)/SDL2"
freetype_include="$(pkg-config --variable=includedir freetype2)/freetype2"

# Macro expansion 
find src -type f \( -name "*.macros.c" -o -name "*.macros.h" \) | while read -r source_file; do
    
    dir_name=$(dirname "$source_file")
    base_name=$(basename "$source_file")
    gen_dir="$dir_name/generated"
    mkdir -p "$gen_dir"
    
    out_name=$(echo "$base_name" | sed 's/macros/expanded/')
    out_file="$gen_dir/$out_name"
    
    if [[ "$source_file" == *.h ]]; then
        define="-DMACROS_H"
    else
        define="-DMACROS_C"
    fi

    gcc -E -P -CC -nostdinc $define "$source_file" > "$out_file" 2>/dev/null
    
    echo " -> Generated: $out_file"
done

ctags --recurse=yes \
    --exclude=.git \
    --exclude=build \
    --exclude=data \
    --exclude=vendored \
    --c++-kinds=+p \
    . $sdl2_include $freetype_include

mkdir -p build

gcc -std=gnu11 \
    $common_flags_internal \
    $ignore_warning_flags \
    $(realpath src/linux_vared.c) \
    $(pkg-config --cflags $pkgs) \
    -o build/vared \
    $(pkg-config --libs $pkgs) \
    $linker_flags -Wl,-rpath,$(realpath build):$glew_libdir

