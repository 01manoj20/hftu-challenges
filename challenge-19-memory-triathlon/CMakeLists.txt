cmake_minimum_required(VERSION 3.20)
project(challenge-19-memory-triathlon LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Fetch the local-dev sample trace into the BUILD dir on first configure (not
# committed — it's a 11 MB binary; the build dir is gitignored and rm-clean). The
# certified run uses a different held-out trace anyway.
set(SAMPLE_TRACE "${CMAKE_CURRENT_BINARY_DIR}/sample.trace")
set(SAMPLE_URL  "https://github.com/Vitorian/hft-challenges/releases/download/c19-sample-v2/sample.trace")
set(SAMPLE_SHA256 "01cb06389c66fc411af22605d2f04c179fa55bbefc9a8fa26e55674ee98a7bf3")
if(NOT EXISTS "${SAMPLE_TRACE}")
    message(STATUS "Fetching sample.trace into build dir ...")
    file(DOWNLOAD "${SAMPLE_URL}" "${SAMPLE_TRACE}"
         EXPECTED_HASH SHA256=${SAMPLE_SHA256} STATUS _dl SHOW_PROGRESS)
    list(GET _dl 0 _code)
    if(NOT _code EQUAL 0)
        list(GET _dl 1 _msg)
        file(REMOVE "${SAMPLE_TRACE}")
        message(WARNING "Could not fetch sample.trace (${_msg}). "
                        "Run ./fetch_sample.sh, or set TRIATHLON_TRACE to your own trace.")
    endif()
endif()

add_executable(benchmark benchmark.cpp solution/solution.cpp)
# Bake the fetched trace's absolute path in as the default, so ./build/benchmark
# finds it regardless of the working directory.
target_compile_definitions(benchmark PRIVATE DEFAULT_TRACE="${SAMPLE_TRACE}")

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/../common/challenge.cmake)
    include(${CMAKE_CURRENT_SOURCE_DIR}/../common/challenge.cmake)
elseif(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/common/challenge.cmake)
    include(${CMAKE_CURRENT_SOURCE_DIR}/common/challenge.cmake)
endif()
