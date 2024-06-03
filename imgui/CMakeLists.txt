add_library(imgui STATIC 
	"imconfig.h"
	"imgui.h"
	"imgui.cpp"
	"imgui.natstepfilter"
	"imgui.natvis"
	"imgui_demo.cpp"
	"imgui_draw.cpp"
	"imgui_impl_dx12.cpp"
	"imgui_impl_dx12.h"
	"imgui_impl_win32.cpp"
	"imgui_impl_win32.h"
	"imgui_internal.h"
	"imgui_tables.cpp"
	"imgui_widgets.cpp"
 )
if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET imgui PROPERTY CXX_STANDARD 20)
endif()
