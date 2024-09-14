cmake_minimum_required(VERSION 3.28)
project(Toy LANGUAGES C)

### Setup
if(NOT IS_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/bootstrap)
    execute_process(
        COMMAND make bootstrap OUTPUT=${CMAKE_CURRENT_BINARY_DIR}/
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/bpftool/src
    )
else()
    message(STATUS "Skipping bootstrapping")
endif()
set(bootstrap_dir ${CMAKE_CURRENT_BINARY_DIR}/bootstrap)
set(bpftool_EXECUTABLE ${bootstrap_dir}/bpftool)
set(libbpf_INCLUDE_DIRS ${bootstrap_dir}/libbpf/include)
set(libbpf_LIBRARIES ${bootstrap_dir}/libbpf/libbpf.a)


set(vmlinux_dir ${CMAKE_CURRENT_BINARY_DIR}/vmlinux)
set(vmlinux ${vmlinux_dir}/vmlinux.h)
if (NOT IS_DIRECTORY ${vmlinux_dir})
    file(MAKE_DIRECTORY ${vmlinux_dir})
    execute_process(
        COMMAND ${bpftool_EXECUTABLE} btf dump file /sys/kernel/btf/vmlinux format c
        OUTPUT_FILE ${vmlinux}
        ERROR_VARIABLE vmlinux_error
        RESULT_VARIABLE vmlinux_result
    )
    if (NOT ${vmlinux_result} EQUAL 0)
        message(FATAL_ERROR "Failed to dump vmlinux.h from BTF: ${vmlinux_error}")
    endif()
else()
    message(STATUS "Skipping vmlinux.h dumping")
endif()


### Build bpf objects
set(ARCH "x86")
find_program(clang_EXECUTABLE REQUIRED NAMES clang)
macro(bpfobject bpf_c)
    get_filename_component(name ${bpf_c} NAME_WE)
    get_filename_component(dir ${bpf_c} DIRECTORY)

    set(bpf_dir ${CMAKE_CURRENT_BINARY_DIR}/bpfobjects)
    set(bpf_o ${bpf_dir}/${name}.bpf.o)
    set(bpf_skel ${bpf_dir}/${name}.skel.h)
    set(bpf_target ${name}_skel)

    message(STATUS "skel: ${bpf_skel}")

    add_custom_command( 
        OUTPUT ${bpf_o}
        COMMAND 
            ${clang_EXECUTABLE} -g -O2 -Wall -target bpf -D__TARGET_ARCH_${ARCH}
            -I${vmlinux_dir} -I${libbpf_INCLUDE_DIRS}
            -c ${bpf_c} -o ${bpf_o}
        VERBATIM
        DEPENDS ${bpf_c}
        COMMENT "[clang] Building BPF object: ${name}"
    )

    add_custom_command(
        OUTPUT ${bpf_skel}
        COMMAND bash -c "${bpftool_EXECUTABLE} gen skeleton ${bpf_o} > ${bpf_skel}"
        VERBATIM
        DEPENDS ${bpf_o}
        COMMENT "[skel] Building BPF skeleton: ${name}"
    )

    add_library(${bpf_target} INTERFACE)
    target_include_directories(${bpf_target} INTERFACE ${bpf_dir})
    target_include_directories(${bpf_target} SYSTEM INTERFACE ${libbpf_INCLUDE_DIRS})
    target_link_libraries(${bpf_target} INTERFACE ${libbpf_LIBRARIES} -lelf -lz)
endmacro()

file(GLOB apps
    toy/*.bpf.c
)

add_library(apps_target) # Just for bpf.c files intellisense
target_sources(apps_target PRIVATE ${apps})
target_include_directories(apps_target PRIVATE ${vmlinux_dir} ${libbpf_INCLUDE_DIRS})

foreach(app ${apps})
    bpfobject(${app})

    get_filename_component(name ${app} NAME_WE)
    get_filename_component(dir ${app} DIRECTORY)

    add_executable(${name} ${dir}/${name}.c)
    target_link_libraries(${name} PRIVATE ${name}_skel)
endforeach()
