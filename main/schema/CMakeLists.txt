function(tower_add_packet_schemas in out)
    file(GLOB schemas
        ${in}/*.fbs
        ${in}/client/*.fbs
        ${in}/entity/*.fbs
        ${in}/entity/skill/*.fbs
        ${in}/player/*.fbs
    )

    add_custom_target(
        tower-schema-packet
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        COMMAND flatc -I ${in} -o ${out} --cpp --cpp-std c++17 --filename-suffix "\"\"" --filename-ext hpp ${schemas}
        DEPENDS flatc
    )
    message("packet schemas: ${schemas}")
endfunction()

function(tower_add_world_schemas in out)
    file(GLOB schemas ${in}/*.fbs)

    add_custom_target(
        tower-schema-world
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        COMMAND flatc -I ${in} -o ${out} --cpp --cpp-std c++17 --filename-suffix "\"\"" --filename-ext hpp ${schemas}
        DEPENDS flatc
    )
    message("world schemas: ${schemas}")
endfunction()