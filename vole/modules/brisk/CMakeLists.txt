vole_module_name("brisk")
vole_module_description("brisk feature descriptor")
vole_module_variable("Vole_Brisk")

vole_add_required_dependencies("OPENCV")
vole_add_required_modules("agast")

vole_compile_library(
		"brisk.h" "brisk.cpp"
)


vole_add_module()
