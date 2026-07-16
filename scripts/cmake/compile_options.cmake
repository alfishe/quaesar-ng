
# Function to add Edit and Continue support for MSVC
function(add_option_edit_and_continue target_name)
    if(MSVC) # Enable Edit and Continue for Debug builds
        target_compile_options(${target_name} PRIVATE /W4 /WX /bigobj)
        # Suppress MSVC CRT deprecation warnings (fopen, sprintf, etc.)
        target_compile_definitions(${target_name} PRIVATE _CRT_SECURE_NO_WARNINGS)
        target_link_options(${target_name} PRIVATE /INCREMENTAL /SAFESEH:NO)
        target_compile_options(${target_name} PRIVATE /MP) # multiprocessor build

        #if(NOT CMAKE_GENERATOR_PLATFORM)
            #set(CMAKE_GENERATOR_PLATFORM "Win32") # add Win32 platform to solution
        #endif()
    endif()
endfunction()
