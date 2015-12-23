vole_module_name("puhma_common")
vole_module_description("Common stuff for puhma")
vole_module_variable("Puhma_Common")

vole_add_required_dependencies("OPENCV" "BOOST" "BOOST_PROGRAM_OPTIONS" "QT" "QT_XML")

vole_compile_library(
	functions
	puhma_common
        platt
)

vole_add_module()
