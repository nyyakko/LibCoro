add_subdirectory(source)
add_subdirectory(include/libcoro)

add_library(${PROJECT_NAME} STATIC "${LibCoro_SourceFiles}")
add_library(${PROJECT_NAME}::${PROJECT_NAME} ALIAS ${PROJECT_NAME})
set_target_properties(${PROJECT_NAME} PROPERTIES OUTPUT_NAME ${PROJECT_NAME} PREFIX "")

if (ENABLE_CLANGTIDY)
    enable_clang_tidy(${PROJECT_NAME})
endif()

if (ENABLE_CPPCHECK)
    enable_cppcheck(${PROJECT_NAME})
endif()

target_include_directories(${PROJECT_NAME}
    PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/include/libcoro"
    INTERFACE
        "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
        "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>"
)

target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_23)

target_sources(${PROJECT_NAME} PUBLIC FILE_SET HEADERS BASE_DIRS include FILES ${LibCoro_HeaderFiles})

export(TARGETS      ${PROJECT_NAME}
       FILE         ${EXPORT_DIR}/${PROJECT_NAME}Targets.cmake
       NAMESPACE    ${PROJECT_NAME}::
)

install(TARGETS     ${PROJECT_NAME}
        EXPORT      ${PROJECT_NAME}Targets
        ARCHIVE
        FILE_SET HEADERS
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(EXPORT      ${PROJECT_NAME}Targets
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/${PROJECT_NAME}/cmake
        NAMESPACE   ${PROJECT_NAME}::
)

install(FILES ${PROJECT_SOURCE_DIR}/cmake/${PROJECT_NAME}Config.cmake DESTINATION ${CMAKE_INSTALL_LIBDIR}/${PROJECT_NAME}/cmake)

target_link_options(${PROJECT_NAME} PRIVATE ${LibCoro_LinkerOptions})
target_compile_options(${PROJECT_NAME} PRIVATE ${LibCoro_CompilerOptions})
target_link_libraries(${PROJECT_NAME} PRIVATE ${LibCoro_ExternalLibraries})

