include(FetchContent)

find_package(Boost)
find_package(Threads REQUIRED)
find_package(fmt)

include_directories(${Boost_INCLUDE_DIRS})

add_executable(benchmark_single benchmark_single.cpp)