# - Find FFTW
# Find the native FFTW includes and library
#
# FFTW_INCLUDES - where to find fftw3.h
# FFTW_LIBRARIES - List of libraries when using FFTW.
# FFTW_FOUND - True if FFTW found.

if (FFTW_INCLUDE_DIR)
  # Already in cache, be silent
  set (FFTW_FIND_QUIETLY TRUE)
endif (FFTW_INCLUDE_DIR)

find_path (FFTW_INCLUDE_DIR fftw3.h)

find_library (FFTW_LIBRARY NAMES fftw3)

# handle the QUIETLY and REQUIRED arguments and set FFTW_FOUND to TRUE if
# all listed variables are TRUE
include (FindPackageHandleStandardArgs)
find_package_handle_standard_args (FFTW DEFAULT_MSG FFTW_LIBRARY FFTW_INCLUDE_DIR)

mark_as_advanced (FFTW_LIBRARY FFTW_INCLUDE_DIR)
