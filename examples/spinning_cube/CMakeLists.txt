add_executable(spinning_cube WIN32
	"Source.cpp")

spall_configure_target(spinning_cube 20)

target_link_libraries(spinning_cube PRIVATE
	spall::runtime)
