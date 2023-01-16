add_executable(opc-reader)

target_compile_features(opc-reader PRIVATE cxx_std_20)
target_compile_options(opc-reader PRIVATE ${MY_WARNINGS})

target_sources(opc-reader PRIVATE main.cpp)

target_link_libraries(opc-reader PRIVATE spdlog::spdlog fmt::fmt bfg::lyra)
target_link_libraries(opc-reader PRIVATE libopcreader)

target_include_directories(opc-reader PRIVATE ${CMAKE_SOURCE_DIR}/include)

target_link_directories(opc-reader PRIVATE "${CMAKE_SOURCE_DIR}/libs/opcdalib/lib")
target_link_libraries(opc-reader PRIVATE debug OPCClientToolKit64D optimized OPCClientToolKit64)
