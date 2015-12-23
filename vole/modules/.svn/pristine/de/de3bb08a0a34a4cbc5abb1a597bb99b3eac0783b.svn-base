#include "swap_channel_config.h"

using namespace boost::program_options;

namespace rbase {

SwapChannelConfig::SwapChannelConfig(const std::string& prefix) :
	Config(prefix), img("img")
{
	#ifdef WITH_BOOST
		initBoostOptions();
	#endif // WITH_BOOST
}

std::string SwapChannelConfig::getString() const
{
	std::stringstream s;

	if (prefix_enabled) {
		s << "[" << prefix << "]" << std::endl;
	} else {
	}
	s << img.getString()
	  << "output_file=" << output_file
		<< " # file name of output RGB image" << std::endl
	  << "channels=" << channels << " # two channels, comma-separated (e.g., 'r,g' for red,green)" << std::endl
	  << "swap=" << swap << " # swap two color channels (specified in 'channels')" << std::endl
	;

	return s.str();
}

#ifdef WITH_BOOST
void SwapChannelConfig::initBoostOptions()
{
	if (!prefix_enabled) {
	}

	options.add(img.options);
	options.add_options()
		(key("output_file,O"), value(&output_file)->default_value(""),
			"file name of output RGB image")
	    (key("channels,C"), value(&channels)->default_value("r,b"),
	  		"two channels, comma-separated (e.g., 'r,g' for red,green)")
	    (key("swap,S"), bool_switch(&swap)->default_value(false),
	  		"swap two color channels (specified in 'channels')")
	;
}
#endif // WITH_BOOST

} // namespace rbase

