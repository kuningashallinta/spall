add_executable(compute_image WIN32
	"Source.cpp")

spall_configure_target(compute_image 20)

target_link_libraries(compute_image PRIVATE
	spall::backends)
