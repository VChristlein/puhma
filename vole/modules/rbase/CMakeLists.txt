vole_module_name("rbase")
vole_module_description("Base Methods for reflectance analysis, e.g. helper methods for reading images")
vole_module_variable("Vole_RBase")

vole_add_required_dependencies("OPENCV")
vole_add_required_modules()

vole_compile_library(
	"img_read.h"                      "img_read.cpp"
	"img_read_config.h"               "img_read_config.cpp"
	"color_space.h"                   "color_space.cpp"
	"mask.h"                          "mask.cpp"
	"illum.h"                         "illum.cpp"
	"color.h"                         "color.cpp"
	"normal_image.h"                  "normal_image.cpp"
	"commands/command_srgb2rgb.h"     "commands/command_srgb2rgb.cpp"
	"commands/command_swap_channel.h" "commands/command_swap_channel.cpp"
	"swap_channel_config.h"           "swap_channel_config.cpp"
	"srgb2rgb_config.h"               "srgb2rgb_config.cpp"
)

vole_add_command("srgb2rgb" "commands/command_srgb2rgb.h" "rbase::CommandSrgb2Rgb")
vole_add_command("swap_channels" "commands/command_swap_channel.h" "rbase::CommandSwapChannel")

vole_add_module()
