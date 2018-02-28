# global
cmake_minimum_required(VERSION 3.2)

if(DEFINED ENV{TRAVIS_BUILD_NUMBER})
    project(bulk VERSION 0.0.$ENV{TRAVIS_BUILD_NUMBER})
else()
    project(bulk VERSION 0.0.1)
endif()

set(Boost_USE_STATIC_LIBS ON)
set(Boost_USE_STATIC_RUNTIME ON)

find_package(Boost COMPONENTS unit_test_framework REQUIRED)

set(CPACK_GENERATOR DEB)

set(CPACK_PACKAGE_VERSION_MAJOR "${PROJECT_VERSION_MAJOR}")
set(CPACK_PACKAGE_VERSION_MINOR "${PROJECT_VERSION_MINOR}")
set(CPACK_PACKAGE_VERSION_PATCH "${PROJECT_VERSION_PATCH}")

set(CPACK_PACKAGE_CONTACT madmazoku@gmail.com)

include(CPack)

enable_testing()

# local
configure_file(in.version.h version.h)

add_executable(
    bulk
        "main.cpp"
)

add_executable(
    bulk_test
        "test.cpp"
)

set_target_properties(bulk bulk_test PROPERTIES
    CXX_STANDARD 14
    CXX_STANDARD_REQUIRED ON
    COMPILE_OPTIONS -Wpedantic -Wall -Wextra
)

set_target_properties(bulk bulk_test PROPERTIES
    COMPILE_DEFINITIONS BOOST_TEST_STATIC_LINK
    INCLUDE_DIRECTORIES ${Boost_INCLUDE_DIR}
)

target_link_libraries(bulk)

target_link_libraries(bulk_test
    ${Boost_LIBRARIES}
)

install(TARGETS bulk RUNTIME DESTINATION bin)

add_test(bulk bulk_test)
