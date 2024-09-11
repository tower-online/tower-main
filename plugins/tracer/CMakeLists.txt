cmake_minimum_required(VERSION 3.28)
project(tracer 
    LANGUAGES C
)

file(GLOB apps tracer/*.bpf.c)
foreach (app ${apps})
    get_filename_component(app_stem ${app} NAME_WE)

endforeach ()