if(WIN32)
    file(INSTALL DESTINATION ${PACKAGE_DIR}/bin
        TYPE FILE
        FILES
            ${BIN_DIR}/porthole.pyd
        )
elseif(OMEGA_OS_LINUX)
    file(INSTALL DESTINATION ${PACKAGE_DIR}/bin
        TYPE FILE
        FILES
            ${BIN_DIR}/porthole.so
        )
endif()

file(INSTALL DESTINATION ${PACKAGE_DIR}/modules/porthole
    TYPE DIRECTORY
    FILES
        ${SOURCE_DIR}/modules/porthole/examples
        ${SOURCE_DIR}/modules/porthole/examples_direct
        ${SOURCE_DIR}/modules/porthole/elements
        ${SOURCE_DIR}/modules/porthole/res
    )

file(INSTALL DESTINATION ${PACKAGE_DIR}/modules/porthole
    TYPE FILE
    FILES
        ${SOURCE_DIR}/modules/porthole/server.cfg 
    )
    