option(BUILD_DOC "Build documentation" ON)

# Check if doxygen installed
find_package(Doxygen)
if (DOXYGEN_FOUND AND BUILD_DOC)

  # Doxygen configuration
  set(DOXYGEN_GENERATE_HTML YES)
  set(DOXYGEN_USE_MDFILE_AS_MAINPAGE ../README.md)
      
  # Create target
  doxygen_add_docs(
    doxygen
    ../README.md
    ../mkdocs.yml
    ../src
    emulator.md  
    fw_guide.md  
    install.md        
    logging.md   
    pcie.md         
    scanconsole.md      
    version.md
    fe65p2.md    
    kernel_driver.md  
    netio.md     
    rd53a.md        
    star.md
    fei4.md      
    index.md     
    localdb.md        
    pcie_ext.md  
    rootscripts.md  
    troubleshooting.md
    COMMENT "Generate automatic documentation")
endif()
