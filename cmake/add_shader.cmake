function(add_shader shader_files output_spv_names)
    find_program(GLSLC "glslc")
    if (NOT GLSLC)
        message(FATAL_ERROR "glslc не найден. Убедитесь, что он установлен и доступен в PATH.")
    endif()
    set(spv_names)
    foreach(shader_file ${shader_files})
        add_custom_command(PRE_BUILD
            COMMAND ${GLSLC} ${CMAKE_SOURCE_DIR}/${shader_file} -o ${CMAKE_SOURCE_DIR}/${shader_file}.spv
            DEPENDS ${shader_file}
            OUTPUT ${shader_file}.spv
            COMMENT "Copmiling shader: ${shader_file} -> ${shader_file}.spv"
            VERBATIM
        )
        list(APPEND spv_names ${shader_file}.spv)
    endforeach()
    set(${output_spv_names} ${spv_names} PARENT_SCOPE)
endfunction()
