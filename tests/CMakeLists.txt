find_package(Qt6Test ${QT_MIN_VERSION} REQUIRED)

add_executable(test_kliftsubject test_kliftsubject.cpp)
target_compile_definitions(test_kliftsubject PRIVATE KLIFTSUBJECT_SOURCE_DIR="${CMAKE_SOURCE_DIR}")
target_link_libraries(test_kliftsubject PRIVATE
    kliftsubject_core
    Qt6::Core
    Qt6::Gui
    Qt6::Test
)

add_test(NAME test_kliftsubject COMMAND test_kliftsubject)
