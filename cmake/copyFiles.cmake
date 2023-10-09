macro(CopyDLL target_name)
    if(WIN32)
        add_custom_command(
            TARGET ${target_name} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy $CACHE{SDL2_DYNAMIC_LIB_DIR}/SDL2.dll $<TARGET_FILE_DIR:${target_name}>
            COMMAND ${CMAKE_COMMAND} -E copy $CACHE{SDL2_IMAGE_DYNAMIC_LIB_DIR}/SDL2_image.dll $<TARGET_FILE_DIR:${target_name}>)
    endif()
endmacro(CopyDLL)

macro(CopyShader target_name)
    add_custom_command(
        TARGET ${target_name} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy ${PROJECT_SOURCE_DIR}/shaders/vert.spv $<TARGET_FILE_DIR:${target_name}>/shaders/vert.spv)
    add_custom_command(
        TARGET ${target_name} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy ${PROJECT_SOURCE_DIR}/shaders/frag.spv $<TARGET_FILE_DIR:${target_name}>/shaders/frag.spv)
endmacro(CopyShader)

macro(CopyTexture target_name)
    add_custom_command(
        TARGET ${target_name} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory ${PROJECT_SOURCE_DIR}/resources $<TARGET_FILE_DIR:${target_name}>/resources)
endmacro(CopyTexture)

macro(CopyModel target_name)
    add_custom_command(
        TARGET ${target_name} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory ${PROJECT_SOURCE_DIR}/models $<TARGET_FILE_DIR:${target_name}>/models)
endmacro(CopyModel)

macro(CopyResources target_name)
    if(WIN32)
        add_custom_command(
            TARGET ${target_name} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/resources/ $<TARGET_FILE_DIR:${target_name}>/resources)
    endif()
endmacro(CopyResources)