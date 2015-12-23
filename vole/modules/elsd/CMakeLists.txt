vole_module_name("elsd")
vole_module_description("ellipse fitting based on line segment detector")
vole_module_variable("Vole_ELSD")

vole_add_required_dependencies("OPENCV" "LAPACK" )
vole_add_required_modules()

vole_compile_library(
		"elsd_wrapper.h" "elsd_wrapper.cpp"
		"elsd.h" "elsd.c"
		"process_curve.h" "process_curve.c"
		"process_line.h" "process_line.c"
		"valid_curve.h" "valid_curve.c"
		"write_svg.h" "write_svg.c"
)

vole_add_module()
