
add_library(libopcreader)

target_compile_features(libopcreader PRIVATE cxx_std_20)

target_include_directories(libopcreader PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(libopcreader PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/../opcdalib/include")

target_link_libraries(libopcreader PRIVATE spdlog::spdlog)
target_link_libraries(libopcreader PRIVATE nlohmann_json::nlohmann_json)
target_link_libraries(libopcreader PRIVATE asio asio::asio)

target_sources(libopcreader PRIVATE   
	opcreader.cpp 
	opcreader.h
)


