cmake_minimum_required(VERSION 3.15)

include(proj.conf)

find_package(bouffalo_sdk REQUIRED HINTS $ENV{BL_SDK_BASE})

sdk_add_include_directories(.)

target_sources(app PRIVATE sdk-usb.c flash-storage.c)
target_link_libraries(app libsunela.a -L$ENV{TOP}/obj-sdk/)
sdk_set_main_file(sdk-main.c)
#sdk_set_linker_script(bl808_flash_${CPU_ID}.ld)
sdk_add_compile_definitions(-DFLASH_STORAGE_BASE=$ENV{FLASH_STORAGE_BASE})
sdk_add_compile_definitions(-DFLASH_STORAGE_SIZE=$ENV{FLASH_STORAGE_SIZE})
sdk_add_compile_definitions(-DTARGET_NAME=$ENV{TARGET_NAME})
sdk_add_compile_definitions($ENV{TARGET_CFLAGS})
sdk_set_linker_script(sunela-$ENV{TARGET}.ld)

project(sunela)
