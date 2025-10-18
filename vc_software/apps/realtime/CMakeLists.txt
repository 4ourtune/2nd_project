cmake_minimum_required(VERSION 3.10)
project(vc_realtime CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
add_compile_options(-O2 -Wall -Wextra -Wpedantic)

add_executable(vc_realtime
  main.cpp
  joystick_thread.cpp
  sensor_thread.cpp
  control_thread.cpp
  comm_thread.cpp
  ipc_server.cpp
)

target_link_libraries(vc_realtime pthread)
