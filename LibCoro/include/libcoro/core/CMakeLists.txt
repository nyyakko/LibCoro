set(DIR ${CMAKE_CURRENT_SOURCE_DIR})

set(LibCoro_HeaderFiles ${LibCoro_HeaderFiles}
    "${DIR}/MutexProtected.hpp"
    "${DIR}/ThreadPool.hpp"

    PARENT_SCOPE
)

