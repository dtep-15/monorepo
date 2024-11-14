add_library(frontend)

add_custom_target(frontend trunk build --release)

get_filename_component(PARENT_DIR ${CMAKE_SOURCE_DIR}/.. ABSOLUTE)

add_custom_command(
	TARGET frontend POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy_directory
	${PARENT_DIR}/frontend ${PARENT_DIR}/server/data/public
)
