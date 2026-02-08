cmake_minimum_required(VERSION 3.25)

project(undying_terminal VERSION 1.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(MINGW)
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static -static-libgcc -static-libstdc++")
  set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -static -static-libgcc -static-libstdc++")
endif()

if(MSVC)
  set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

option(UNDYING_TERMINAL_BUILD_TESTS "Build tests" ON)

if(WIN32)
  set(_undying_terminal_default_deps ON)
else()
  set(_undying_terminal_default_deps OFF)
endif()
option(UNDYING_TERMINAL_REQUIRE_DEPS "Require crypto/protobuf dependencies" ${_undying_terminal_default_deps})

set(UNDYING_TERMINAL_MINGW64_PREFIX "" CACHE PATH "MSYS2 mingw64 prefix (eg C:/msys64/mingw64)")

set(UT_PROTO_SRCS
  build/UT.pb.cc
  build/UTerminal.pb.cc
)

set(UT_PROTO_HDRS
  build/UT.pb.h
  build/UTerminal.pb.h
)

if(UNDYING_TERMINAL_REQUIRE_DEPS)
  if(NOT UNDYING_TERMINAL_MINGW64_PREFIX)
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/deps/msys2/mingw64")
      set(UNDYING_TERMINAL_MINGW64_PREFIX "${CMAKE_CURRENT_SOURCE_DIR}/deps/msys2/mingw64")
    elseif(EXISTS "C:/msys64/mingw64")
      set(UNDYING_TERMINAL_MINGW64_PREFIX "C:/msys64/mingw64")
    endif()
  endif()

  if(UNDYING_TERMINAL_MINGW64_PREFIX)
    list(PREPEND CMAKE_PREFIX_PATH "${UNDYING_TERMINAL_MINGW64_PREFIX}")
  endif()

  find_package(absl CONFIG REQUIRED)
  find_package(utf8_range CONFIG REQUIRED)
  find_package(ZLIB REQUIRED)
  find_package(Protobuf CONFIG REQUIRED)
  set(PROTOBUF_LIBRARIES protobuf::libprotobuf)

  find_package(unofficial-sodium CONFIG QUIET)
  if(TARGET unofficial-sodium::sodium)
    set(_undying_terminal_sodium_target unofficial-sodium::sodium)
  elseif(UNDYING_TERMINAL_MINGW64_PREFIX)
    set(SODIUM_INCLUDE_DIR "${UNDYING_TERMINAL_MINGW64_PREFIX}/include" CACHE PATH "")
    set(SODIUM_LIBRARIES "${UNDYING_TERMINAL_MINGW64_PREFIX}/lib/libsodium.a" CACHE FILEPATH "")
    if(NOT EXISTS "${SODIUM_LIBRARIES}")
      message(FATAL_ERROR "libsodium not found; install libsodium or provide UNDYING_TERMINAL_MINGW64_PREFIX")
    endif()
  else()
    message(FATAL_ERROR "libsodium not found; use vcpkg (unofficial-sodium) or set UNDYING_TERMINAL_MINGW64_PREFIX")
  endif()
endif()

add_executable(undying_terminal
  src/ut/main.cpp
  src/ut/ClientId.cpp
  src/ut/CryptoUtils.cpp
  src/ut/Keepalive.cpp
  src/ut/MockServer.cpp
  src/ut/PseudoTerminalConsole.cpp
  src/ut/ReconnectionManager.cpp
  src/ut/SshConfig.cpp
  src/ut/protocol/BackedReader.cpp
  src/ut/protocol/BackedWriter.cpp
  src/ut/protocol/ClientConnection.cpp
  src/ut/protocol/Connection.cpp
  src/ut/protocol/CryptoHandler.cpp
  src/ut/protocol/PipeSocketHandler.cpp
  src/ut/protocol/PortForwardHandler.cpp
  src/ut/protocol/ServerClientConnection.cpp
  src/ut/protocol/SocketHandler.cpp
  src/ut/protocol/TcpSocketHandler.cpp
  src/ut/protocol/TunnelUtils.cpp
  src/ut/SshCommandBuilder.cpp
  src/ut/SshSubprocess.cpp
  src/ut/TcpClient.cpp
  src/ut/WinsockContext.cpp
  src/ut/WindowsPaths.cpp
  src/ut/Config.cpp
  ${UT_PROTO_SRCS}
  ${UT_PROTO_HDRS}
)
set_target_properties(undying_terminal PROPERTIES OUTPUT_NAME "undying-terminal")

if(WIN32)
  target_sources(undying_terminal PRIVATE src/win/undying-terminal.rc)
endif()

target_compile_definitions(undying_terminal PRIVATE UNDYING_TERMINAL_VERSION="${PROJECT_VERSION}")

target_include_directories(undying_terminal PRIVATE
  src/ut
  src/ut/protocol
  ${CMAKE_CURRENT_SOURCE_DIR}/build
  ${CMAKE_CURRENT_SOURCE_DIR}/proto
)

if(UNDYING_TERMINAL_REQUIRE_DEPS)
  target_link_libraries(undying_terminal PRIVATE
    ${PROTOBUF_LIBRARIES}
  )
  if(TARGET unofficial-sodium::sodium)
    target_link_libraries(undying_terminal PRIVATE ${_undying_terminal_sodium_target})
  else()
    target_include_directories(undying_terminal PRIVATE ${SODIUM_INCLUDE_DIR})
    target_link_libraries(undying_terminal PRIVATE ${SODIUM_LIBRARIES})
  endif()
endif()

if(WIN32)
  target_link_libraries(undying_terminal PRIVATE ws2_32 advapi32 ole32 uuid)
endif()

add_executable(undying_terminal_terminal
  src/utterminal/main.cpp
  src/utterminal/ConPTYSession.cpp
  src/utterminal/NamedPipeClient.cpp
  src/ut/PseudoTerminalConsole.cpp
  src/ut/protocol/BackedReader.cpp
  src/ut/protocol/BackedWriter.cpp
  src/ut/protocol/ClientConnection.cpp
  src/ut/protocol/Connection.cpp
  src/ut/protocol/CryptoHandler.cpp
  src/ut/protocol/SocketHandler.cpp
  src/ut/protocol/PipeSocketHandler.cpp
  src/ut/protocol/TcpSocketHandler.cpp
  ${UT_PROTO_SRCS}
  ${UT_PROTO_HDRS}
)
set_target_properties(undying_terminal_terminal PROPERTIES OUTPUT_NAME "undying-terminal-terminal")

if(WIN32)
  target_sources(undying_terminal_terminal PRIVATE src/win/undying-terminal-terminal.rc)
endif()

target_compile_definitions(undying_terminal_terminal PRIVATE UNDYING_TERMINAL_VERSION="${PROJECT_VERSION}")

target_include_directories(undying_terminal_terminal PRIVATE
  src/ut
  src/ut/protocol
  src/utterminal
  ${CMAKE_CURRENT_SOURCE_DIR}/build
  ${CMAKE_CURRENT_SOURCE_DIR}/proto
)

if(UNDYING_TERMINAL_REQUIRE_DEPS)
  target_link_libraries(undying_terminal_terminal PRIVATE
    ${PROTOBUF_LIBRARIES}
  )
  if(TARGET unofficial-sodium::sodium)
    target_link_libraries(undying_terminal_terminal PRIVATE ${_undying_terminal_sodium_target})
  else()
    target_include_directories(undying_terminal_terminal PRIVATE ${SODIUM_INCLUDE_DIR})
    target_link_libraries(undying_terminal_terminal PRIVATE ${SODIUM_LIBRARIES})
  endif()
endif()

if(WIN32)
  target_link_libraries(undying_terminal_terminal PRIVATE ws2_32 advapi32 ole32 uuid)
endif()

add_executable(undying_terminal_server
  src/utserver/main.cpp
  src/utserver/ClientRegistry.cpp
  src/utserver/FirewallRules.cpp
  src/utserver/JobObject.cpp
  src/utserver/NamedPipeServer.cpp
  src/utserver/TcpListener.cpp
  src/utserver/Verbose.cpp
  src/utserver/Server.cpp
  src/utserver/WindowsService.cpp
  src/ut/protocol/BackedReader.cpp
  src/ut/protocol/BackedWriter.cpp
  src/ut/protocol/Connection.cpp
  src/ut/protocol/CryptoHandler.cpp
  src/ut/protocol/PipeSocketHandler.cpp
  src/ut/protocol/PortForwardHandler.cpp
  src/ut/protocol/ServerClientConnection.cpp
  src/ut/protocol/SocketHandler.cpp
  src/ut/protocol/TcpSocketHandler.cpp
  src/ut/protocol/TunnelUtils.cpp
  src/ut/WinsockContext.cpp
  src/ut/CryptoUtils.cpp
  src/ut/Config.cpp
  src/ut/WindowsPaths.cpp
  ${UT_PROTO_SRCS}
  ${UT_PROTO_HDRS}
)
set_target_properties(undying_terminal_server PROPERTIES OUTPUT_NAME "undying-terminal-server")

if(WIN32)
  target_sources(undying_terminal_server PRIVATE src/win/undying-terminal-server.rc)
endif()

target_compile_definitions(undying_terminal_server PRIVATE UNDYING_TERMINAL_VERSION="${PROJECT_VERSION}")

target_include_directories(undying_terminal_server PRIVATE
  src/ut
  src/ut/protocol
  src/utserver
  ${CMAKE_CURRENT_SOURCE_DIR}/build
  ${CMAKE_CURRENT_SOURCE_DIR}/proto
)

if(UNDYING_TERMINAL_REQUIRE_DEPS)
  target_link_libraries(undying_terminal_server PRIVATE
    ${PROTOBUF_LIBRARIES}
  )
  if(TARGET unofficial-sodium::sodium)
    target_link_libraries(undying_terminal_server PRIVATE ${_undying_terminal_sodium_target})
  else()
    target_include_directories(undying_terminal_server PRIVATE ${SODIUM_INCLUDE_DIR})
    target_link_libraries(undying_terminal_server PRIVATE ${SODIUM_LIBRARIES})
  endif()
endif()

if(WIN32)
  target_link_libraries(undying_terminal_server PRIVATE ws2_32 advapi32 ole32 uuid)
endif()

if(UNDYING_TERMINAL_BUILD_TESTS)
  enable_testing()

  add_executable(winsock_context_test
    tests/winsock_context_test.cpp
    src/ut/WinsockContext.cpp
  )
  target_include_directories(winsock_context_test PRIVATE src/ut)
  if(WIN32)
    target_link_libraries(winsock_context_test PRIVATE ws2_32)
  endif()
  add_test(NAME winsock_context_test COMMAND winsock_context_test)

  add_executable(pseudo_terminal_console_test
    tests/pseudo_terminal_console_test.cpp
    src/ut/PseudoTerminalConsole.cpp
  )
  target_include_directories(pseudo_terminal_console_test PRIVATE src/ut)
  add_test(NAME pseudo_terminal_console_test COMMAND pseudo_terminal_console_test)

  add_executable(subprocess_utils_test
    tests/subprocess_utils_test.cpp
    src/ut/SubprocessUtils.cpp
  )
  target_include_directories(subprocess_utils_test PRIVATE src/ut)
  add_test(NAME subprocess_utils_test COMMAND subprocess_utils_test)

  add_executable(ssh_subprocess_test
    tests/ssh_subprocess_test.cpp
    src/ut/SshSubprocess.cpp
  )
  target_include_directories(ssh_subprocess_test PRIVATE src/ut)
  add_test(NAME ssh_subprocess_test COMMAND ssh_subprocess_test)

  add_executable(ssh_command_builder_test
    tests/ssh_command_builder_test.cpp
    src/ut/SshCommandBuilder.cpp
  )
  target_include_directories(ssh_command_builder_test PRIVATE src/ut)
  add_test(NAME ssh_command_builder_test COMMAND ssh_command_builder_test)

  add_executable(ssh_config_test
    tests/ssh_config_test.cpp
    src/ut/SshConfig.cpp
  )
  target_include_directories(ssh_config_test PRIVATE src/ut)
  add_test(NAME ssh_config_test COMMAND ssh_config_test)

  add_executable(windows_paths_test
    tests/windows_paths_test.cpp
    src/ut/WindowsPaths.cpp
  )
  target_include_directories(windows_paths_test PRIVATE src/ut)
  if(WIN32)
    target_link_libraries(windows_paths_test PRIVATE shell32 ole32)
  endif()
  add_test(NAME windows_paths_test COMMAND windows_paths_test)

  add_executable(client_id_test
    tests/client_id_test.cpp
    src/ut/ClientId.cpp
  )
  target_include_directories(client_id_test PRIVATE src/ut)
  if(WIN32)
    target_link_libraries(client_id_test PRIVATE advapi32)
  endif()
  add_test(NAME client_id_test COMMAND client_id_test)
endif()
