[![DDraceNetwork](https://ddnet.org/ddnet-small.png)](https://ddnet.org) [![](https://github.com/ddnet/ddnet/workflows/Build/badge.svg)](https://github.com/ddnet/ddnet/actions?query=workflow%3ABuild+event%3Apush+branch%3Amaster) [![](https://codecov.io/gh/ddnet/ddnet/branch/master/graph/badge.svg)](https://codecov.io/gh/ddnet/ddnet/branch/master)

This is only a test client used in a Tutorial: https://www.youtube.com/watch?v=8hOoQNq7k-c

This client contains just a "Pet" feature...

Cloning
-------

To clone this repository with full history and external libraries (~350 MB):

    git clone --recursive https://github.com/ddnet/ddnet

To clone this repository with full history when you have the necessary libraries on your system already (~220 MB):

    git clone https://github.com/ddnet/ddnet

To clone this repository with history since we moved the libraries to https://github.com/ddnet/ddnet-libs (~40 MB):

    git clone --shallow-exclude=included-libs https://github.com/ddnet/ddnet

To clone the libraries if you have previously cloned DDNet without them, or if you require the ddnet-libs history instead of a shallow clone:

    git submodule update --init --recursive

Dependencies on Linux / macOS
-----------------------------

You can install the required libraries on your system, `touch CMakeLists.txt` and CMake will use the system-wide libraries by default. You can install all required dependencies and CMake on Debian or Ubuntu like this:

    sudo apt install build-essential cargo cmake git glslang-tools google-mock libavcodec-extra libavdevice-dev libavfilter-dev libavformat-dev libavutil-dev libcurl4-openssl-dev libfreetype6-dev libglew-dev libnotify-dev libogg-dev libopus-dev libopusfile-dev libpng-dev libsdl2-dev libsqlite3-dev libssl-dev libvulkan-dev libwavpack-dev libx264-dev python rustc spirv-tools

On older distributions like Ubuntu 18.04 don't install `google-mock`, but instead set `-DDOWNLOAD_GTEST=ON` when building to get a more recent gtest/gmock version.

On older distributions `rustc` version might be too old, to get an up-to-date Rust compiler you can use [rustup](https://rustup.rs/) with stable channel instead or try the `rustc-mozilla` package.

Or on CentOS, RedHat and AlmaLinux like this:

    sudo yum install cargo cmake ffmpeg-devel freetype-devel gcc gcc-c++ git glew-devel glslang gmock-devel gtest-devel libcurl-devel libnotify-devel libogg-devel libpng-devel libx264-devel make openssl-devel opus-devel opusfile-devel python2 rust SDL2-devel spirv-tools sqlite-devel vulkan-devel wavpack-devel

Or on Fedora like this:

    sudo dnf install cargo cmake ffmpeg-devel freetype-devel gcc gcc-c++ git glew-devel glslang gmock-devel gtest-devel libcurl-devel libnotify-devel libogg-devel libpng-devel make openssl-devel opus-devel opusfile-devel python2 SDL2-devel spirv-tools sqlite-devel vulkan-devel wavpack-devel x264-devel

Or on Arch Linux like this:

    sudo pacman -S --needed base-devel cmake curl ffmpeg freetype2 git glew glslang gmock libnotify libpng opusfile python rust sdl2 spirv-tools sqlite vulkan-headers vulkan-icd-loader wavpack x264

Or on Gentoo like this:

    emerge --ask dev-db/sqlite dev-lang/rust-bin dev-libs/glib dev-libs/openssl dev-util/glslang dev-util/spirv-headers dev-util/spirv-tools media-libs/freetype media-libs/glew media-libs/libglvnd media-libs/libogg media-libs/libpng media-libs/libsdl2 media-libs/libsdl2[vulkan] media-libs/opus media-libs/opusfile media-libs/pnglite media-libs/vulkan-loader[layers] media-sound/wavpack media-video/ffmpeg net-misc/curl x11-libs/gdk-pixbuf x11-libs/libnotify

On macOS you can use [homebrew](https://brew.sh/) to install build dependencies like this:

    brew install cmake ffmpeg freetype glew glslang googletest libpng molten-vk opusfile rust SDL2 spirv-tools vulkan-headers wavpack x264

If you don't want to use the system libraries, you can pass the `-DPREFER_BUNDLED_LIBS=ON` parameter to cmake.

Building on Linux and macOS
---------------------------

To compile DDNet yourself, execute the following commands in the source root:

    mkdir build
    cd build
    cmake ..
    make -j$(nproc)

Pass the number of threads for compilation to `make -j`. `$(nproc)` in this case returns the number of processing units.

DDNet requires additional libraries, some of which are bundled for the most common platforms (Windows, Mac, Linux, all x86 and x86\_64) for convenience and the official builds. The bundled libraries for official builds are now in the ddnet-libs submodule. Note that when you build and develop locally, you should ideally use your system's package manager to install the dependencies, instead of relying on ddnet-libs submodule, which does not contain all dependencies anyway (e.g. openssl, vulkan). See the previous section for how to get the dependencies. Alternatively see the following build arguments for how to disable some features and their dependencies (`-DVULKAN=OFF` won't require Vulkan for example).
