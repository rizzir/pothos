########################################################################
# install include files
########################################################################
install(
    DIRECTORY Pothos
    DESTINATION include
    COMPONENT pothos_devel
    PATTERN "*.tmpl.*" EXCLUDE
)

########################################################################
# generated headers
########################################################################
find_package(PythonInterp)

#convenience macro to generate from a templated file
function(gen_tmpl inFile)
    string(REPLACE ".tmpl." "." outFile ${inFile})
    set(exp_tmpl_py ${CMAKE_CURRENT_SOURCE_DIR}/ExpandTmpl.py)
    add_custom_command(
        OUTPUT ${outFile}
        COMMAND ${PYTHON_EXECUTABLE} ${exp_tmpl_py} ${inFile} ${outFile}
        DEPENDS ${inFile} ${exp_tmpl_py}
    )
    list(APPEND gen_tmpl_outputs ${outFile})
    set(gen_tmpl_outputs ${gen_tmpl_outputs} PARENT_SCOPE)
endfunction(gen_tmpl)

#declare generation rules for the templated sources
#developers must manually configure with -DGEN_ENABLED for generation
if(GEN_ENABLED AND PYTHON_EXECUTABLE)
    gen_tmpl(${CMAKE_CURRENT_SOURCE_DIR}/Pothos/Callable/Callable.tmpl.hpp)
    gen_tmpl(${CMAKE_CURRENT_SOURCE_DIR}/Pothos/Callable/CallableImpl.tmpl.hpp)
    gen_tmpl(${CMAKE_CURRENT_SOURCE_DIR}/Pothos/Callable/CallInterface.tmpl.hpp)
    gen_tmpl(${CMAKE_CURRENT_SOURCE_DIR}/Pothos/Callable/CallInterfaceImpl.tmpl.hpp)
    gen_tmpl(${CMAKE_CURRENT_SOURCE_DIR}/Pothos/Callable/CallRegistry.tmpl.hpp)
    gen_tmpl(${CMAKE_CURRENT_SOURCE_DIR}/Pothos/Callable/CallRegistryImpl.tmpl.hpp)
    gen_tmpl(${CMAKE_CURRENT_SOURCE_DIR}/Pothos/Proxy/Proxy.tmpl.hpp)
    gen_tmpl(${CMAKE_CURRENT_SOURCE_DIR}/Pothos/Proxy/ProxyImpl.tmpl.hpp)
    gen_tmpl(${CMAKE_CURRENT_SOURCE_DIR}/Pothos/Managed/Class.tmpl.hpp)
    gen_tmpl(${CMAKE_CURRENT_SOURCE_DIR}/Pothos/Managed/ClassImpl.tmpl.hpp)
    add_custom_target(gen_tmpl ALL DEPENDS ${gen_tmpl_outputs})
endif()

########################################################################
# precompiled headers - top level headers only
########################################################################
file(GLOB_RECURSE all_include_files
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/Pothos/*.hpp)
list(APPEND all_include_files ${gen_tmpl_outputs})

file(GLOB include_files
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/Pothos/*.hpp)

foreach(include_file ${include_files})
    message(STATUS "include_file: ${include_file}")
    get_filename_component(include_path ${include_file} PATH)
    file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/${include_path})
    set(pch_source ${CMAKE_CURRENT_BINARY_DIR}/${include_file}.cpp)
    file(WRITE ${pch_source} "#include <${CMAKE_CURRENT_SOURCE_DIR}/${include_file}>\n")
    if(CMAKE_COMPILER_IS_GNUCXX)
        set(pch_file ${CMAKE_CURRENT_BINARY_DIR}/${include_file}.gch)
        add_custom_command(
            DEPENDS ${all_include_files} OUTPUT ${pch_file}
            COMMAND ${CMAKE_CXX_COMPILER} -std=c++11
                -I ${CMAKE_CURRENT_SOURCE_DIR}
                -x c++-header ${pch_source} -o ${pch_file}
        )
    elseif(MSVC)
        set(pch_file ${CMAKE_CURRENT_BINARY_DIR}/${include_file}.pch)
        add_custom_command(
            DEPENDS ${all_include_files} OUTPUT ${pch_file}
            COMMAND ${CMAKE_CXX_COMPILER} /c /EHsc
                /I ${CMAKE_CURRENT_SOURCE_DIR} /Yc ${pch_source}
            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/${include_path}
        )
    endif()
    if(pch_file)
        list(APPEND pch_files ${pch_file})
        install(
            FILES ${pch_file}
            DESTINATION include/${include_path}
            COMPONENT pothos_devel
        )
    endif()
endforeach(include_file)

if (pch_files)
    add_custom_target(pch_build ALL DEPENDS ${pch_files})
endif (pch_files)
