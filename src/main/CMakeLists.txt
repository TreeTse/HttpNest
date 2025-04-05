aux_source_directory(. SRCS)
add_executable(nest ${SRCS})

add_dependencies(base jsoncpp)
target_link_libraries(nest base network http crypto jsoncpp_static.a)
install(TARGETS nest RUNTIME DESTINATION sbin)