
include_directories(
  libs
  libs/uae_src/include
  libs/uae_src
  external
  src
  src/uae_imp
)


set(UAE_SOURCE_LIST
    libs/uae_src/aros.rom.cpp
    libs/uae_src/akiko.cpp
    libs/uae_src/amax.cpp
    libs/uae_src/ar.cpp
    libs/uae_src/audio.cpp
    libs/uae_src/autoconf.cpp
    libs/uae_src/blitfunc.cpp
    libs/uae_src/blittable.cpp
    libs/uae_src/blitter.cpp
    libs/uae_src/calc.cpp
    libs/uae_src/cd32_fmv_genlock.cpp
    libs/uae_src/cdrom.cpp
    libs/uae_src/cdtvcr.cpp
    libs/uae_src/cfgfile.cpp
    libs/uae_src/cia.cpp
    libs/uae_src/consolehook.cpp
    libs/uae_src/cpudefs.cpp
    libs/uae_src/cpuemu_0.cpp
    libs/uae_src/cpuemu_11.cpp
    libs/uae_src/cpuemu_13.cpp
    libs/uae_src/cpuemu_20.cpp
    libs/uae_src/cpuemu_21.cpp
    libs/uae_src/cpuemu_22.cpp
    libs/uae_src/cpuemu_23.cpp
    libs/uae_src/cpuemu_24.cpp
    libs/uae_src/cpuemu_31.cpp
    libs/uae_src/cpuemu_32.cpp
    libs/uae_src/cpuemu_33.cpp
    libs/uae_src/cpuemu_34.cpp
    libs/uae_src/cpuemu_35.cpp
    libs/uae_src/cpuemu_40.cpp
    libs/uae_src/cpuemu_50.cpp
    libs/uae_src/cpummu.cpp
    libs/uae_src/cpummu30.cpp
    libs/uae_src/cpustbl.cpp
    libs/uae_src/crc32.cpp
    libs/uae_src/custom.cpp
    libs/uae_src/debug.cpp
    libs/uae_src/debugmem.cpp
    libs/uae_src/def_icons.cpp
    libs/uae_src/devices.cpp
    libs/uae_src/disasm.cpp
    libs/uae_src/disk.cpp
    libs/uae_src/diskutil.cpp
    libs/uae_src/dlopen.cpp
    libs/uae_src/dongle.cpp
    libs/uae_src/drawing.cpp
    libs/uae_src/driveclick.cpp
    libs/uae_src/enforcer.cpp
    libs/uae_src/ethernet.cpp
    libs/uae_src/events.cpp
    libs/uae_src/expansion.cpp
    libs/uae_src/fdi2raw.cpp
    libs/uae_src/filesys.cpp
    libs/uae_src/flashrom.cpp
    libs/uae_src/fpp.cpp
    libs/uae_src/fpp_native.cpp
    libs/uae_src/fpp_softfloat.cpp
    libs/uae_src/fsdb.cpp
    libs/uae_src/gayle.cpp
    libs/uae_src/hardfile.cpp
    libs/uae_src/hrtmon.rom.cpp
    libs/uae_src/ide.cpp
    libs/uae_src/idecontrollers.cpp
    libs/uae_src/identify.cpp
    libs/uae_src/ini.cpp
    libs/uae_src/inputdevice.cpp
    libs/uae_src/inputrecord.cpp
    libs/uae_src/isofs.cpp
    libs/uae_src/keybuf.cpp
    libs/uae_src/logging.cpp
    libs/uae_src/main.cpp
    libs/uae_src/memory.cpp
    libs/uae_src/missing.cpp
    libs/uae_src/native2amiga.cpp
    libs/uae_src/newcpu.cpp
    libs/uae_src/newcpu_common.cpp
    libs/uae_src/readcpu.cpp
    libs/uae_src/rommgr.cpp
    libs/uae_src/rtc.cpp
    libs/uae_src/sana2.cpp
    libs/uae_src/savestate.cpp
    libs/uae_src/scp.cpp
    libs/uae_src/scsi.cpp
    libs/uae_src/scsiemul.cpp
    libs/uae_src/scsitape.cpp
    libs/uae_src/serial.cpp
    libs/uae_src/sndboard.cpp
    libs/uae_src/statusline.cpp
    libs/uae_src/tabletlibrary.cpp
    libs/uae_src/test_card.cpp
    libs/uae_src/tinyxml2.cpp
    libs/uae_src/traps.cpp
    libs/uae_src/uaeexe.cpp
    libs/uae_src/uaelib.cpp
    libs/uae_src/uaenative.cpp
    libs/uae_src/uaeresource.cpp
    libs/uae_src/uaeserial.cpp
    libs/uae_src/blkdev.cpp
    libs/uae_src/gfxutil.cpp
    libs/uae_src/zfile.cpp
    libs/uae_src/zfile_archive.cpp
    libs/uae_src/vm.cpp
    libs/uae_src/softfloat/softfloat.cpp
    libs/uae_src/softfloat/softfloat_fpsp.cpp
    libs/uae_src/softfloat/softfloat_decimal.cpp
    libs/uae_src/cputbl.h 
    libs/uae_src/jit/comptbl.h
    libs/uae_src/blit.h
)



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


add_executable(quaesar ${UAE_SOURCE_LIST})
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${UAE_SOURCE_LIST})
