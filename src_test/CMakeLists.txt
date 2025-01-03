find_package(GTest REQUIRED)
include_directories(${GTEST_INCLUDE_DIR})

set(TEST_EXECUTABLE ${PROJECT_NAME}_RunTests)

add_executable(${TEST_EXECUTABLE}
ProjHeaderUnitTest.cc
ProjInterfileReaderUnitTest.cc
SiddonUnitTest.cc
)

target_compile_features(${TEST_EXECUTABLE} PUBLIC ${FLAGS})
target_link_libraries(${TEST_EXECUTABLE} ${LIBRARY_NAME})
target_link_libraries(${TEST_EXECUTABLE} ${GTEST_BOTH_LIBRARIES})

# Copy test resources to bin directory
set(TEST_RESOURCES_DIR ${CMAKE_BINARY_DIR}/bin/TestResources/)
file(MAKE_DIRECTORY ${TEST_RESOURCES_DIR})
file(COPY TestResources/ DESTINATION ${TEST_RESOURCES_DIR})

include(GoogleTest)
gtest_discover_tests(${TEST_EXECUTABLE})
