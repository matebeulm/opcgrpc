cmake_minimum_required(VERSION 3.22.0)

option(CROSSBUILD "cross compilation for arm64" OFF)
option(NATIVEBUILDARM64 "native compilation for arm64" OFF)

if (${NATIVEBUILDARM64})
  option(CROSSBUILD "cross compilation for arm64" OFF)
endif()

set(vcpkg "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake")
if(NOT CMAKE_TOOLCHAIN_FILE AND EXISTS "${vcpkg}")
  set(CMAKE_TOOLCHAIN_FILE "${vcpkg}"
      CACHE FILEPATH "CMake toolchain file")
  message(STATUS "vcpkg toolchain found: ${CMAKE_TOOLCHAIN_FILE}")
endif()

if (WIN32)
  message(STATUS "system is windows")
  set(VCPKG_HOST_TRIPLET "x64-windows" CACHE STRING "")
  message(STATUS "CMake based manifest mode ${VCPKG_HOST_TRIPLET}")
endif()

if (UNIX)
  if (${NATIVEBUILDARM64})
    set(VCPKG_HOST_TRIPLET "arm64-linux" CACHE STRING "")
  else()
    set(VCPKG_HOST_TRIPLET "x64-linux" CACHE STRING "")
  endif()
  message(STATUS "CMake based manifest mode ${VCPKG_HOST_TRIPLET}")
endif()

if(${CROSSBUILD})
  message(STATUS "this is a cross build")
  set(VCPKG_TARGET_TRIPLET arm64-linux)

  set(VCPKG_TARGET_ARCHITECTURE arm64)
  set(VCPKG_CRT_LINKAGE dynamic)
  set(VCPKG_LIBRARY_LINKAGE static)

  set(VCPKG_CMAKE_SYSTEM_NAME Linux)
  set(VCPKG_CHAINLOAD_TOOLCHAIN_FILE ${CMAKE_SOURCE_DIR}/cross_toolchain.cmake)

  set(CMAKE_SYSROOT "/usr/aarch64-linux-gnu/sys-root")

  set(ENV{PKG_CONFIG_DIR} "")
  set(ENV{PKG_CONFIG_LIBDIR}
      "${CMAKE_SYSROOT}/usr/lib/pkgconfig:${CMAKE_SYSROOT}/usr/share/pkgconfig")
  set(ENV{PKG_CONFIG_SYSROOT_DIR} ${CMAKE_SYSROOT})
else()
  message(STATUS "this is a native build")
  set(VCPKG_TARGET_TRIPLET ${VCPKG_HOST_TRIPLET})
endif()

message(STATUS "this file has defined host triplet as ${VCPKG_HOST_TRIPLET}")
message(STATUS "this file has defined target triplet as ${VCPKG_TARGET_TRIPLET}")

project(opcgrpc LANGUAGES CXX)

find_package(fmt CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)
find_package(lyra CONFIG REQUIRED)
find_package(asio CONFIG REQUIRED)
find_package(gRPC CONFIG REQUIRED)
find_package(Protobuf CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)

add_subdirectory(proto-grpc)
add_subdirectory(libs)
