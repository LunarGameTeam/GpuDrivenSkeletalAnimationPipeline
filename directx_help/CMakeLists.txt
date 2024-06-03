add_library(directx_help STATIC 
	"DDS.h"
	"DDSTextureLoader.cpp"
	"DDSTextureLoader.h"
	"DirectXHelpers.cpp"
	"DirectXHelpers.h"
	"GraphicsMemory.cpp"
	"GraphicsMemory.h"
	"LinearAllocator.cpp"
	"LinearAllocator.h"
	"LoaderHelpers.h"
	"pch.h"
	"PlatformHelpers.h"
	"ResourceUploadBatch.cpp"
	"ResourceUploadBatch.h"
	"d3dx12.h"
	"GenerateMips_main.inc"
 )
if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET directx_help PROPERTY CXX_STANDARD 20)
endif()
