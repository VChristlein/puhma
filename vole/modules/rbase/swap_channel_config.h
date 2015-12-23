#ifndef RBASE_SWAP_CHANNEL_CONFIG_H
#define RBASE_SWAP_CHANNEL_CONFIG_H

#include "vole_config.h"
#include "img_read_config.h"

namespace rbase {

	class SwapChannelConfig : public vole::Config {
		public:
			SwapChannelConfig(const std::string& prefix = std::string());
		public:
			virtual std::string getString() const;

		protected:
			#ifdef WITH_BOOST
				virtual void initBoostOptions();
			#endif // WITH_BOOST

		public:
			iread::IReadConfig img;
			std::string output_file;

			bool swap;
			std::string channels;

	};
}

#endif //  RBASE_SWAP_CHANNEL_CONFIG_H
