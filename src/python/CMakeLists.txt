YARR_ADD_PYBIND11()
pybind11_add_module(_pyyarr src/_pyyarr.cpp)
target_link_libraries(_pyyarr PRIVATE Yarr Scan)
install(DIRECTORY tools/ DESTINATION ${TARGET_INSTALL_AREA}/python
        FILES_MATCHING PATTERN "*.py")
install(DIRECTORY pyYARR/ DESTINATION ${TARGET_INSTALL_AREA}/python/pyYARR
        FILES_MATCHING PATTERN "*.py")