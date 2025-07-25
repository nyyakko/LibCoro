add_subdirectory(core)

set(DIR ${CMAKE_CURRENT_SOURCE_DIR})

set(LibCoro_HeaderFiles ${LibCoro_HeaderFiles}
    "${DIR}/Task.hpp"
    "${DIR}/Generator.hpp"

    PARENT_SCOPE
)

