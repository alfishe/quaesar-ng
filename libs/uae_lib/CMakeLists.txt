cmake_minimum_required(VERSION 3.10)
project(uae_lib LANGUAGES CXX)

get_filename_component(UAE_DIR ${CMAKE_CURRENT_SOURCE_DIR} ABSOLUTE)


set(UAE_SOURCE_LIST
    aros.rom.cpp
    akiko.cpp
    amax.cpp
    ar.cpp
    audio.cpp
    autoconf.cpp
    blitfunc.cpp
    blittable.cpp
    blitter.cpp
    calc.cpp
    cd32_fmv_genlock.cpp
    cdrom.cpp
    cdtvcr.cpp
    cfgfile.cpp
    cia.cpp
    consolehook.cpp
    cpudefs.cpp
    cpuemu_0.cpp
    cpuemu_11.cpp
    cpuemu_13.cpp
    cpuemu_20.cpp
    cpuemu_21.cpp
    cpuemu_22.cpp
    cpuemu_23.cpp
    cpuemu_24.cpp
    cpuemu_31.cpp
    cpuemu_32.cpp
    cpuemu_33.cpp
    cpuemu_34.cpp
    cpuemu_35.cpp
    cpuemu_40.cpp
    cpuemu_50.cpp
    cpummu.cpp
    cpummu30.cpp
    cpustbl.cpp
    crc32.cpp
    custom.cpp
    debug.cpp
    debugmem.cpp
    def_icons.cpp
    devices.cpp
    disasm.cpp
    disk.cpp
    diskutil.cpp
    dlopen.cpp
    dongle.cpp
    drawing.cpp
    driveclick.cpp
    enforcer.cpp
    ethernet.cpp
    events.cpp
    expansion.cpp
    fdi2raw.cpp
    filesys.cpp
    flashrom.cpp
    fpp.cpp
    fpp_native.cpp
    fpp_softfloat.cpp
    fsdb.cpp
    gayle.cpp
    hardfile.cpp
    hrtmon.rom.cpp
    ide.cpp
    idecontrollers.cpp
    identify.cpp
    ini.cpp
    inputdevice.cpp
    inputrecord.cpp
    isofs.cpp
    keybuf.cpp
    logging.cpp
    main.cpp
    memory.cpp
    missing.cpp
    native2amiga.cpp
    newcpu.cpp
    newcpu_common.cpp
    readcpu.cpp
    rommgr.cpp
    rtc.cpp
    sana2.cpp
    savestate.cpp
    scp.cpp
    scsi.cpp
    scsiemul.cpp
    scsitape.cpp
    serial.cpp
    sndboard.cpp
    statusline.cpp
    tabletlibrary.cpp
    test_card.cpp
    tinyxml2.cpp
    traps.cpp
    uaeexe.cpp
    uaelib.cpp
    uaenative.cpp
    uaeresource.cpp
    uaeserial.cpp
    blkdev.cpp
    gfxutil.cpp
    zfile.cpp
    zfile_archive.cpp
    vm.cpp
    softfloat/softfloat.cpp
    softfloat/softfloat_fpsp.cpp
    softfloat/softfloat_decimal.cpp
    cputbl.h 
    jit/comptbl.h
    blit.h
)

add_library(uae_lib ${UAE_SOURCE_LIST})
target_compile_definitions(uae_lib PRIVATE -D_HAS_STD_BYTE=0)

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${UAE_SOURCE_LIST})

#-------------------------------------------------------------------------------------------
if (ENABLE_CODE_GENERATION)
    add_executable(build68k uae_src/build68k.cpp)
    add_executable(gencpu uae_src/cpudefs.cpp uae_src/gencpu.cpp uae_src/missing.cpp uae_src/readcpu.cpp src/unicode.cpp)
    add_executable(gencomp uae_src/cpudefs.cpp uae_src/jit/gencomp.cpp uae_src/missing.cpp uae_src/readcpu.cpp src/unicode.cpp)
    add_executable(genblitter uae_src/blitops.cpp uae_src/genblitter.cpp)

    target_compile_definitions(build68k PRIVATE FSUAE)

    if (NOT WIN32)
        target_compile_definitions(gencomp PRIVATE FSUAE)
        target_compile_definitions(gencpu PRIVATE FSUAE)
    endif()

    # Set the output path for the generated code
    set(BUILD68K_OUTPUT ../uae_src/cpudefs.cpp)
    set(GENCPU_OUTPUT ../uae_src/cputbl.h)
    set(GENCOMP_OUTPUT ../uae_jit/comptbl.h)

    add_custom_command(
        OUTPUT ${BUILD68K_OUTPUT}
        COMMAND build68k < table68k > ${BUILD68K_OUTPUT}
        WORKING_DIRECTORY ../uae_src 
        DEPENDS build68k
        COMMENT "Generating 68k cpu definitions"
    )

    add_custom_command(
        OUTPUT ${GENCOMP_OUTPUT}
        COMMAND gencomp
        WORKING_DIRECTORY ../uae_src
        DEPENDS build68k
        COMMENT "Generating jit/comptbl.h"
    )

    add_custom_command(
        OUTPUT ${GENCPU_OUTPUT}
        COMMAND gencpu
        WORKING_DIRECTORY ../uae_src 
        DEPENDS gencpu
        COMMENT "Generating CPU code"
    )

    function(gen_blitter output letter)
        add_custom_command(
            OUTPUT ${output}
            COMMAND genblitter ${letter} > ${output}
            DEPENDS genblitter
            COMMENT "Generating blitter code for ${letter}"
        )
    endfunction()

    gen_blitter(../uae_src/blit.h i)
    gen_blitter(../uae_src/blitfunc.cpp f)
    gen_blitter(../uae_src/blitfunc.h h)
    gen_blitter(../uae_src/blittable.cpp t)
endif()

target_link_libraries(uae_lib zlibstatic)
if (WIN32)
    target_link_libraries(uae_lib ws2_32)
endif()


#-------------------------------------------------------------------------------------------
# Disable some compiler warnings
if (MSVC)
    target_compile_options(uae_lib PRIVATE "/MP") # Enable multi-threaded compilation
    target_compile_options(uae_lib PRIVATE "/wd4244") #("conversion from 'type1' to 'type2', possible loss of data")
    target_compile_options(uae_lib PRIVATE "/wd4309") #(truncation of constant value)
    target_compile_options(uae_lib PRIVATE "/wd4101") #(unreferenced local variable)
    target_compile_options(uae_lib PRIVATE "/wd4200") #(nonstandard extension used: zero-sized array in struct/union)
else()
    target_compile_options(uae_lib PRIVATE "-Wno-conversion" "-Wno-sign-conversion")
    target_compile_options(uae_lib PRIVATE "-Wno-unused-variable" "-Wno-unused-but-set-variable")
endif()
#-------------------------------------------------------------------------------------------


#defines
if (APPLE OR LINUX OR UNIX)
	target_compile_options(uae_lib PRIVATE -DUAE=1 -D_cdecl= -DOS_NAME=\"linux\")
    target_compile_definitions(uae_lib PRIVATE FSUAE)
endif()


#-------------------------------------------------------------------------------------------
# Include dirs
#-------------------------------------------------------------------------------------------
target_include_directories(uae_lib 
    PUBLIC ${UAE_CUSTOM_IMPL_DIR}
    PUBLIC .
    PUBLIC include
    )
