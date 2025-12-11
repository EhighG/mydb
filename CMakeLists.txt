cmake_minimum_required(VERSION 3.20)

# WSL 환경 변수 VCPKG_ROOT를 감지하여 툴체인 설정
if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    if(DEFINED ENV{VCPKG_ROOT})
        set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake" CACHE STRING "")
    else()
        message(FATAL_ERROR "VCPKG_ROOT 환경변수가 설정되지 않았습니다. .bashrc를 확인해주세요.")
    endif()
endif()

project(my_sql VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CSS_STANDARD_REQUIRED ON)

# 컴파일러 경고 강화 (Linux GCC/Clang 전용)
add_compile_options(-Wall -Wextra -Wpedantic)

# 라이브러리 찾기 (Vcpkg가 설치해준 것을 찾음)
find_package(fmt CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)
find_package(absl CONFIG REQUIRED)
find_package(Boost REQUIRED COMPONENTS system)

# 헤더 경로 포함
include_directories(include)

# 실행 파일 생성
add_executable(mydb
        src/main.cpp
        include/mydb/storage/Page.hpp
        include/mydb/storage/DiskManager.hpp
        src/storage/DiskManager.cpp
        include/mydb/buffer/LRUReplacer.hpp
        src/buffer/LRUReplacer.cpp
        include/mydb/buffer/BufferPoolManager.hpp
        src/buffer/BufferPoolManager.cpp
        include/mydb/storage/Tuple.hpp
        include/mydb/storage/TablePage.hpp
)

# 라이브러리 연결 (Link)
target_link_libraries(mydb PRIVATE
    fmt::fmt
    spdlog::spdlog
    absl::strings
    Boost::system
)

# # 테스트 설정 추가

enable_testing()
find_package(GTest CONFIG REQUIRED)

# 테스트용 실행파일 생성
add_executable(mydb_test
    src/storage/DiskManager.cpp
    src/buffer/LRUReplacer.cpp
    src/buffer/BufferPoolManager.cpp
    tests/buffer_test.cpp
)

# GTest 라이브러리 연결
target_link_libraries(mydb_test PRIVATE
    GTest::gtest
    GTest::gtest_main
    GTest::gmock
    fmt::fmt
    spdlog::spdlog
    absl::strings
    Boost::system
)

# 'ctest' 명령어로 발견되도록 등록
add_test(NAME BufferPoolTest COMMAND mydb_test)