set(LIBRARY_NAME ${PROJECT_NAME})
set(LIBRARY_NAME ${LIBRARY_NAME} PARENT_SCOPE)

SET(LIBRARY_HEADERS
    ${SRC_LIB_DIR}/KeyParser.h
    ${SRC_LIB_DIR}/writeKeys.h
    ${SRC_LIB_DIR}/writeKeys.inl

    ${SRC_LIB_DIR}/console.h
    ${SRC_LIB_DIR}/console.inl
    ${SRC_LIB_DIR}/macros.h
    ${SRC_LIB_DIR}/types.h
    ${SRC_LIB_DIR}/types.inl
    ${SRC_LIB_DIR}/tools.h
    ${SRC_LIB_DIR}/tools.inl

    ${SRC_LIB_DIR}/VolHeader.h
    ${SRC_LIB_DIR}/VolHeader.inl
    ${SRC_LIB_DIR}/VolInterfileReader.h
    ${SRC_LIB_DIR}/VolInterfileReader.inl
    ${SRC_LIB_DIR}/VolData.h
    ${SRC_LIB_DIR}/VolData.inl

    ${SRC_LIB_DIR}/ScannerHeader.h
    ${SRC_LIB_DIR}/ScannerInterfileReader.h
    ${SRC_LIB_DIR}/ScannerData.h
    ${SRC_LIB_DIR}/ScannerData.inl

    ${SRC_LIB_DIR}/ProjHeader.h
    ${SRC_LIB_DIR}/ProjHeader.inl
    ${SRC_LIB_DIR}/ProjInterfileReader.h
    ${SRC_LIB_DIR}/ProjInterfileReader.inl
    ${SRC_LIB_DIR}/ProjData.h
    ${SRC_LIB_DIR}/ProjData.inl

    ${SRC_LIB_DIR}/Siddon.h
    ${SRC_LIB_DIR}/LORCache.h

    ${SRC_LIB_DIR}/operations.h
    ${SRC_LIB_DIR}/projections.h
    ${SRC_LIB_DIR}/reconAlgos.h
)

set(LIBRARY_SRC
    ${SRC_LIB_DIR}/KeyParser.cc
    ${SRC_LIB_DIR}/writeKeys.cc

    ${SRC_LIB_DIR}/tools.cc

    ${SRC_LIB_DIR}/VolHeader.cc
    ${SRC_LIB_DIR}/VolInterfileReader.cc
    ${SRC_LIB_DIR}/VolData.cc

    ${SRC_LIB_DIR}/ProjHeader.cc
    ${SRC_LIB_DIR}/ProjInterfileReader.cc
    ${SRC_LIB_DIR}/ProjData.cc

    ${SRC_LIB_DIR}/ScannerHeader.cc
    ${SRC_LIB_DIR}/ScannerInterfileReader.cc
    ${SRC_LIB_DIR}/ScannerData.cc

    ${SRC_LIB_DIR}/Siddon.cc
    ${SRC_LIB_DIR}/LORCache.cc

    ${SRC_LIB_DIR}/operations.cc
    ${SRC_LIB_DIR}/projections.cc
    ${SRC_LIB_DIR}/reconAlgos.cc
)

add_library(${LIBRARY_NAME} SHARED ${LIBRARY_SRC} ${LIBRARY_HEADERS})
target_compile_features(${LIBRARY_NAME} PUBLIC ${FLAGS})
target_include_directories(${LIBRARY_NAME} PUBLIC ${INC_DIR})

source_group("Headers" FILES ${LIBRARY_HEADERS})

find_package(OpenMP)
if(OpenMP_CXX_FOUND)
    target_link_libraries(${LIBRARY_NAME} PUBLIC OpenMP::OpenMP_CXX)
endif()
