# Unit tests using Qt Test framework

find_package(Qt6 REQUIRED COMPONENTS Test)

# Enable automoc for tests
set(CMAKE_AUTOMOC ON)

# Test executable for Config
add_executable(test_config test_config.cpp)
target_link_libraries(test_config PRIVATE
    observatory-shared
    Qt6::Test
)

# Add test to CTest
add_test(NAME ConfigTests COMMAND test_config)

# Test executable for Logger
add_executable(test_logger test_logger.cpp)
target_link_libraries(test_logger PRIVATE
    observatory-shared
    Qt6::Test
)

add_test(NAME LoggerTests COMMAND test_logger)

# Test executable for Layout
add_executable(test_layout test_layout.cpp)
target_link_libraries(test_layout PRIVATE
    observatory-shared
    Qt6::Test
)

add_test(NAME LayoutTests COMMAND test_layout)

message(STATUS "Unit tests configured")
