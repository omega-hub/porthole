pack_native_module(porthole)
pack_dir(res)
if(PACK_EXAMPLES)
    pack_dir(examples)
endif()

file(INSTALL DESTINATION ${PACKAGE_DIR}/modules/porthole
    TYPE FILE
    FILES
        ${MODULE_DIR}//server.cfg 
    )
    