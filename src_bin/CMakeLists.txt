# Forward projection

set(FORWARD_PROJ "ForwardProj")

set(FORWARD_PROJ_EXEC ${PROJECT_NAME}_${FORWARD_PROJ})
set(FORWARD_PROJ_SRC ${SRC_BIN_DIR}/${FORWARD_PROJ}.cc)

add_executable(${FORWARD_PROJ_EXEC} ${FORWARD_PROJ_SRC})
target_compile_features(${FORWARD_PROJ_EXEC} PUBLIC ${FLAGS})
target_link_libraries(${FORWARD_PROJ_EXEC} PUBLIC ${LIBRARY_NAME})

# Backward projection

set(BACK_PROJ "BackProj")

set(BACK_PROJ_EXEC ${PROJECT_NAME}_${BACK_PROJ})
set(BACK_PROJ_SRC ${SRC_BIN_DIR}/${BACK_PROJ}.cc)

add_executable(${BACK_PROJ_EXEC} ${BACK_PROJ_SRC})
target_compile_features(${BACK_PROJ_EXEC} PUBLIC ${FLAGS})
target_link_libraries(${BACK_PROJ_EXEC} PUBLIC ${LIBRARY_NAME})

# OSEM

set(OSEM "OSEM")

set(OSEM_EXEC ${PROJECT_NAME}_${OSEM})
set(OSEM_SRC ${SRC_BIN_DIR}/${OSEM}.cc)

add_executable(${OSEM_EXEC} ${OSEM_SRC})
target_compile_features(${OSEM_EXEC} PUBLIC ${FLAGS})
target_link_libraries(${OSEM_EXEC} PUBLIC ${LIBRARY_NAME})
