########################################################################
# Filter blocks module
########################################################################
include_directories(remez-19980711)
POTHOS_MODULE_UTIL(
    TARGET FilterBlocks
    SOURCES
        remez-19980711/remez.c
        FIRDesigner.cpp
        FIRFilter.cpp
        TestFIRFilter.cpp
    DESTINATION blocks
    ENABLE_DOCS
)
