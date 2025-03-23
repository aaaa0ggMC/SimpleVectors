pacman -S --noconfirm --needed git mingw-w64-ucrt-x86_64-gcc mingw-w64-ucrt-x86_64-glm mingw-w64-ucrt-x86_64-glfw mingw-w64-ucrt-x86_64-rapidjson mingw-w64-ucrt-x86_64-glew mingw-w64-ucrt-x86_64-tomlplusplus mingw-w64-ucrt-x86_64-cmake mingw-w64-ucrt-x86_64-make
export ROOT_PATH=$(pwd)
mkdir -p links
mkdir -p CBuild
mkdir -p CBuild_CACHE_WIN
cd CBuild_CACHE_WIN


# del .dll.as
find /ucrt64/lib -name "*.dll.a" -delete


echo 'Building alib'
git clone https://github.com/aaaa0ggMC/UnlimitedLife-Linux.git
cd UnlimitedLife-Linux
cp ./CDep/headers/* $ROOT_PATH/src
sh configure_win_msys2.sh
cd CBuild_CACHE_WIN
make aaaa0ggmcLib
cd ../CBuild/Windows
cp * $ROOT_PATH/links/
cp * $ROOT_PATH/CBuild/

cd $ROOT_PATH/CBuild_CACHE_WIN

# Run CMake to configure the project
cmake .. -DCMAKE_BUILD_TYPE=Release -G "Unix Makefiles"
