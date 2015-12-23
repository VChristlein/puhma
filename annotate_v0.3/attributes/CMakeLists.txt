
vole_module_name("puhma_attributes")
vole_module_description("Preprocessing of ancient documents")
vole_module_variable("Puhma_Attributes")

vole_add_required_dependencies("OPENCV" "BOOST" "BOOST_PROGRAM_OPTIONS" "QT")
vole_add_required_modules("puhma_common" "vlfeat")
vole_add_command("attri" "attributes_command.h" "puhma::AttributesCommand")

vole_compile_library(
 "attributes_config"
 "attributes_command"
 "attributes_core"
 "attributes_learning"
 "attributes_model"
 "trans_phoc"
 "esift"
 "pcagmm"
 "fisher"
 "util"
 "cca"
 "eva"
 "statistic"
 "attrispace"
 "calibrate"
 "lexicon"
 "recog"
)
#vole_add_python_module("_getphoc" "pyvole_get_phoc.cpp")
vole_add_module()
