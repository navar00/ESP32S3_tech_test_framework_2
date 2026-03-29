# Register the 'main' component (which corresponds to 'src' directory in PIO)

file(GLOB_RECURSE app_sources "*.cpp" "*.c")

idf_component_register(SRCS ${app_sources}
                       INCLUDE_DIRS "." "core" "hal" "screens"
                       REQUIRES bluepad32)
