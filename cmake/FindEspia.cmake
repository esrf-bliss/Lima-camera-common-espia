set(ESPIA_INCLUDE_DIRS)
set(ESPIA_LIBRARIES)
set(ESPIA_DEFINITIONS)

find_path(ESPIA_INCLUDE_DIRS "espia_lib.h")
find_library(ESPIA_LIBRARIES espia)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Espia DEFAULT_MSG
  ESPIA_LIBRARIES
  ESPIA_INCLUDE_DIRS
)
