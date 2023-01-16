#
# Protobuf/Grpc source files
#
set(PROTONAME opcgrpc)
set(PROTO_FILES ${PROTONAME}.proto)
set (PYTHONAPP opcgrpcpy)

#
# Add Library target with protobuf sources
#
add_library(opcgrpcproto ${PROTO_FILES})
target_compile_features(opcgrpcproto PRIVATE cxx_std_20)
target_link_libraries(opcgrpcproto PUBLIC protobuf::libprotobuf gRPC::grpc gRPC::grpc++)

target_include_directories(opcgrpcproto PUBLIC ${CMAKE_CURRENT_BINARY_DIR})

find_program(gRPC_CPP_PLUGIN grpc_cpp_plugin)
message(STATUS "grpc cpp plugin for host build is ${gRPC_CPP_PLUGIN}")
find_program(gRPC_PYTHON_PLUGIN grpc_python_plugin)
message(STATUS "grpc python plugin for host build is ${gRPC_PYTHON_PLUGIN}")
message(STATUS "protoc executable for host build is ${Protobuf_PROTOC_EXECUTABLE}")

protobuf_generate(TARGET opcgrpcproto LANGUAGE cpp)
protobuf_generate(
  TARGET
  opcgrpcproto
  LANGUAGE
  grpc
  GENERATE_EXTENSIONS
  .grpc.pb.h
  .grpc.pb.cc
  PLUGIN
  "protoc-gen-grpc=${gRPC_CPP_PLUGIN}")

protobuf_generate(TARGET opcgrpcproto LANGUAGE python)
protobuf_generate(
  TARGET
  opcgrpcproto
  LANGUAGE
  grpc
  GENERATE_EXTENSIONS
  .grpc.py
  PLUGIN
  "protoc-gen-grpc=${gRPC_PYTHON_PLUGIN}")

set(PYTHON_GRPC_FILES ${PROTONAME}_pb2.py ${PROTONAME}_pb2_grpc.py)

foreach(file_i ${PYTHON_GRPC_FILES})
  message(
    STATUS
      "proto python files in ${CMAKE_CURRENT_BINARY_DIR} {file_i} to ${CMAKE_SOURCE_DIR}/apps/python/sio2_on_si/{file_i}"
  )
  add_custom_command(
    TARGET opcgrpcproto
    POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_BINARY_DIR}/${file_i}
            ${CMAKE_SOURCE_DIR}/apps/python/${PYTHONAPP}/${file_i})
endforeach(file_i)
#
# Compile protobuf and grpc files in opcgrpcproto target to cpp
#
# get_target_property(grpc_cpp_plugin_location gRPC::grpc_cpp_plugin LOCATION)
# protobuf_generate(TARGET opcgrpcproto LANGUAGE cpp) protobuf_generate(TARGET opcgrpcproto LANGUAGE
# grpc GENERATE_EXTENSIONS .grpc.pb.h .grpc.pb.cc PLUGIN
# "protoc-gen-grpc=${grpc_cpp_plugin_location}")
