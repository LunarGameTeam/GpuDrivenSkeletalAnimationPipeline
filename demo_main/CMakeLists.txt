add_executable (demo_main 
	"application_demo.cpp"
	"application_demo.h"
	"demo_main.cpp"
	"demo_main.h"
	"globel_device_value.cpp"
	"globel_device_value.h"
	"gpu_resource_helper.cpp"
	"gpu_resource_helper.h"
	"model_mesh.cpp"
	"model_mesh.h"
	"skeletal_animation.cpp"
	"skeletal_animation.h"
	"sky_box.cpp"
	"sky_box.h"
	"camera.cpp"
	"camera.h"
	)
target_link_libraries(demo_main imgui directx_help)
if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET demo_main PROPERTY CXX_STANDARD 20)
endif()
