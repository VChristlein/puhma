vole_module_name("agast")
vole_module_description("agast corner detector")
vole_module_variable("Vole_Agast")

vole_add_required_dependencies("OPENCV")
vole_add_required_modules()

vole_compile_library(
"agast5_8.h" "agast5_8.cc"
"agast7_12d.h" "agast7_12d.cc"
"agast7_12s.h" "agast7_12s.cc"
"AstDetector.h" "AstDetector.cc"
"cvWrapper.h"
"oast9_16.h" "oast9_16.cc"
"agast5_8_nms.cc"
"agast7_12d_nms.cc"
"agast7_12s_nms.cc"
"nonMaximumSuppression.cc"
"oast9_16_nms.cc"
)


vole_add_module()
