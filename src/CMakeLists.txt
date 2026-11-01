add_library(kliftsubject_core STATIC
    subjectextractor.cpp
    subjectextractor.h
)

target_include_directories(kliftsubject_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(kliftsubject_core PUBLIC
    Qt6::Core
    Qt6::Gui
)

add_executable(kliftsubject
    main.cpp
)

target_link_libraries(kliftsubject PRIVATE
    kliftsubject_core
    Qt6::Core
    Qt6::Gui
)

install(TARGETS kliftsubject DESTINATION ${KDE_INSTALL_BINDIR})
