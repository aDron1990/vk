function(add_shader shader_file)
    find_program(GLSLC "glslc")
    if (NOT GLSLC)
        message(FATAL_ERROR "glslc не найден. Убедитесь, что он установлен и доступен в PATH.")
    endif()
    add_custom_command(PRE_BUILD
        COMMAND ${GLSLC} ${CMAKE_SOURCE_DIR}/${shader_file} -o ${CMAKE_SOURCE_DIR}/${shader_file}.spv
        DEPENDS ${shader_file}
        OUTPUT ${shader_file}.spv
        COMMENT "Copmiling shader: ${shader_file} -> ${shader_file}.spv"
        VERBATIM
    )
endfunction()
