#!/bin/bash

build_target="clang"
build_type="Debug"
build_library=false
clean=false
clean_only=false
skip_cmake=false
args=""
open=false

while [ "$1" != "" ]; do
  case "$1" in
    -h | --help)
      echo ":"
      echo ": Wasp Engine Build System"
      echo ":"
      echo ": - -- Options"
      echo ": h help                                 : prints this message"
      echo ": t target  [clang|wasm|gcc|mingw|msvc]  : sets build target"
      echo ": c clean                                : cleans build files for target then exits"
      echo ":   rebuild                              : cleans files for target before building"
      echo ": r release                              : release build (default is debug)"
      echo ": s spec                                 : unit-test/spec build"
      echo ": m modules                              : update submodules to current versions"
      echo ":   modules-update                       : update submodules to latest versions"
      echo ": p pull                                 : pull with submodules"
      echo ": l library                              : builds as a library"
      echo ": o open                                 : opens the IDE for msvc target"
      echo ": -- <args>                              : passes remaining args to built exe"
      exit
      ;;
    -t | --target)
      build_target="$2"
      shift 1
      ;;
    -- )
      args="$@"
      break 1
      ;;
    -c | --clean)
      clean=true
      clean_only=true
      ;;
    --rebuild)
      clean=true
      ;;
    -m | --modules)
      echo ": Getting submodules at current specified versions"
      git submodule update --init --recursive
      build_target="done"
      ;;
    --modules-update)
      echo ": Updating submodules to latest versions"
      git submodule update --init --recursive --remote
      build_target="done"
      ;;
    -p | --pull)
      echo ": Pulling with submodules"
      git pull --recurse-submodules
      build_target="done"
      ;;
    -r | --release)
      build_type="Release"
      ;;
    -s | --spec)
      build_type="Spec"
      ;;
    -o | --open)
      open=true
      ;;
    *)
      echo ": Unknown parameter: $1"
      exit
      ;;
  esac
  shift 1
done

# Early-exit if no further operations needed
if [ "$build_target" == "done" ]; then
  exit
fi

# Clean files from build target
if [ $clean = true ]; then
  echo ": Deleting build files for $build_target"
  rm -rf ./build/$build_target
  if [ $clean_only = true ]; then
    echo ": Done"
    exit
  fi
  echo ": Continuing build..."
fi

# Get make executable
make_exe="no_make"

which make &> /dev/null
if [ "$?" == "0" ]; then
  make_exe="make"
else
  which mingw32-make &> /dev/null
  if [ "$?" == "0" ]; then
    make_exe="mingw32-make"
  fi
fi

# Executable checks
case "$build_target" in
  "wasm" | "clang" )
    which clang &> /dev/null
    if [ "$?" != "0" ]; then
      echo ": build $build_target: clang compiler not found, exiting build."
      exit
    fi
    ;;&
  "mingw" | "msvc" )
    which cmake &> /dev/null
    if [ "$?" != "0" ]; then
      echo ": build $build_target: cmake not found, exiting build."
      exit
    fi
    ;;&
  "mingw" )
    if [ "$make_exe" == "no_make" ]; then
      echo ": build $build_target: make not found, exiting build."
      exit
    fi
    ;;
esac

# Run build based on target type

sources_test="
  ./lib/cspec/cspec.c
  ./lib/cspec/tst/cspec_spec.c
  ./lib/mclib/tst/*_spec.c
  ./tst/*.c
"

sources_demo="
  ./demo/*.c
"

sources="
  ./src/*.c
  ./src/loaders/*.c
  ./lib/mclib/src/*.c
  ./lib/mclib/lib/murmur3/murmur3.c
"

includes="
  -I ./include
  -I ./lib/mclib/include
  -I ./lib/mclib/lib/murmur3
  -I ./lib/stb/stb_image.h
"

build_native="
  ./lib/galogen/galogen.c
  -I ./lib/galogen
"

flags_memtest="
  -Dmalloc=cspec_malloc -Drealloc=cspec_realloc
  -Dcalloc=cspec_calloc -Dfree=cspec_free
";

flags_common="-Wall -Wextra -Wno-missing-braces -Wno-deprecated-declarations"

flags_debug_opt="-g -O0"
if [ "$build_type" = "Release" ]; then
  flags_debug_opt="-Oz -flto"
fi

# WASM
if [ "$build_target" = "wasm" ]; then

  # -nostdinc doesn't work because wasi doesn't ship with stddef for some reason
  # --target=wasm32 for non-wasi build. It works, but no standard lib is painful
  # -fgnuc-version=0 : tells clang to stop pretending to be GCC for ifdefs
  flags_wasm="--target=wasm32-wasi -D__WASM__ -fgnuc-version=0
    -Wl,--allow-undefined -Wl,--no-entry -Wl,--lto-O3
    --no-standard-libraries -std=c23
    -isystem ./lib/wasi-libc/sysroot/include/wasm32-wasi
    -isystem ./include/wasm
    ./lib/wasi-libc/sysroot/lib/wasm32-wasi/libc.a
    ./src/wasm/*.c
  "

  pushd . &> /dev/null
  cd ./lib/wasi-libc
  if [ ! -d sysroot/ ]; then

    echo ": build wasm: Building wasi-libc..."

    if [ "$make_exe" == "no_make" ]; then
      echo ": build wasm: No make installed, required to build wasi-libc."
      popd &> /dev/null
      exit
    fi

    # Build wasi-libc
    eval $make_exe
  fi
  popd &> /dev/null

  mkdir -p build/wasm/$build_type

  if [ "$build_type" == "Spec" ]; then

    if [ ! -f build/wasm/$build_type/index.html ]; then
      cp lib/cspec/web/* build/wasm/$build_type/ -r
    fi

    clang $flags_memtest -o build/wasm/$build_type/test.wasm \
      $flags_wasm $flags_common $flags_debug_opt $includes \
      $sources $sources_test -I ./lib/cspec

  else

    clang -o build/wasm/$build_type/game.wasm \
      $flags_wasm $flags_common $flags_debug_opt $includes \
      $sources $sources_demo

    if [ "$?" = "0" ]; then
      cp build/wasm/$build_type/game.wasm web/game.wasm
    fi

  fi

# Clang build targets
elif [ "$build_target" = "clang" ]; then

  mkdir -p build/$build_target

  if [ "$build_type" == "Spec" ]; then

    clang $flags_memtest -o build/clang/test.exe \
      $flags_common $flags_debug_opt $includes $build_native $sources $sources_test

    if [ "$?" == "0" ]; then
      ./build/clang/test.exe $args
    fi

  else

    clang $flags_memtest -o build/clang/test.exe \
      $flags_common $flags_debug_opt $includes $build_native $sources $sources_demo

    if [ "$?" == "0" ]; then
      ./build/clang/demo.exe $args
    fi

  fi



# GCC
elif [ "$build_target" = "gcc" ]; then

  mkdir -p build/gcc

  gcc -o build/gcc/test.exe -Wno-cast-function-type -Wno-implicit-fallthrough \
    $flags_common $flags_debug_opt $includes $build_native $sources $sources_test

  if [ "$?" == "0" ]; then
    ./build/gcc/test.exe $args
  fi

# CMake MinGW on Windows with GCC
elif [ "$build_target" = "mingw" ]; then

  cmake -G "MinGW Makefiles" -S . -B build/mingw/$build_type \
    -DCMAKE_BUILD_TYPE=$build_type -DCMAKE_C_COMPILER=gcc
  if [ "$?" != "0" ]; then
    exit
  fi

  pushd . &> /dev/null
  cd build/mingw/$build_type/
  eval $make_exe
  if [ "$?" == "0" ]; then
    if [ "$build_type" == "Spec" ]; then
      ./Wasp_spec.exe $args
      popd &> /dev/null
    else
      popd &> /dev/null
      (cd web; "../build/mingw/$build_type/Wasp_demo.exe" $args)
    fi
  fi

# CMake MSVC
elif [ "$build_target" = "msvc" ]; then

  if [ "$build_type" = "Spec" ]; then
    cmake -G "Visual Studio 17 2022" -S . -B build/msvc/Spec -DCMAKE_BUILD_TYPE=Spec
  else
    cmake -G "Visual Studio 17 2022" -S . -B build/msvc/Demo
  fi

  if [ "$?" != "0" ]; then
    exit
  fi

  if [ "$?" == "0" ] && [ "$open" == true ]; then
    echo ": Opening Visual Studio"
    if [ "$build_type" = "Spec" ]; then
      start ./build/msvc/Spec/Wasp.sln
    else
      start ./build/msvc/Demo/Wasp.sln
    fi
  fi

# No matching build types
else

  echo ": Invalid build target: $build_target"

fi
