
# Function to add Edit and Continue support for MSVC
function(add_option_edit_and_continue target_name)
    if (WIN32)
        if(MSVC) # Enable Edit and Continue for Debug builds
            target_compile_options(${target_name} PRIVATE "/ZI")
            #string(REGEX REPLACE "/Z[iI7]" "" CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG}")
            #set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /ZI")
            #target_compile_options(${target_name} PRIVATE ${CMAKE_CXX_FLAGS_DEBUG})

            target_link_options(${target_name} PRIVATE "/INCREMENTAL")
            target_compile_options(${target_name} PRIVATE "/MP") # multiprocessor build
            
            if(NOT CMAKE_GENERATOR_PLATFORM)
                set(CMAKE_GENERATOR_PLATFORM "Win32") # add Win32 platform to solution
            endif()
        endif()
    endif()
endfunction()
