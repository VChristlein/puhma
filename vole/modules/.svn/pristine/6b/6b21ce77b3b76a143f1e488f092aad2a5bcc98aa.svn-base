#include "commands/command_swap_channel.h"
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>
#include "img_read.h"

namespace rbase {

CommandSwapChannel::CommandSwapChannel()
 : vole::Command(
		"swap_channels",
		config,
		"Christian Riess",
		"christian.riess@informatik.uni-erlangen.de")
{
}

int CommandSwapChannel::execute() {
	if (config.img.imageFile.length() < 1) {
		std::cerr << "input file name (--img.image) required!" << std::endl;
		return 1;
	}
	if (config.output_file.length() < 1) {
		std::cerr << "output file name (-O) required!" << std::endl;
		return 1;
	}

	if ((config.swap) && (config.channels.length() != 3)) {
		std::cerr << "option --swap: --channels must be exactly three characters, i.e. the two color channels (abbreviated by one lower-case letter), separated by a comma" << std::endl;
	}

	cv::Mat imageRaw = cv::imread(config.img.imageFile, -1);

	if (config.swap) {

		// figure out which channel switches with which
		int source, target;
		switch (config.channels[0]) {
			case 'r': source = 2; break; // note bgr channel order in OpenCV
			case 'g': source = 1; break;
			case 'b': source = 0; break; // note bgr channel order in OpenCV
					break;
			default: 
				std::cerr << "option --swap: first character of --channels must be either 'r', 'g' or 'b', aborted." << std::endl;
				return 1;
		}
		switch (config.channels[2]) {
			case 'r': target = 2; break; // note bgr channel order in OpenCV
			case 'g': target = 1; break;
			case 'b': target = 0; break; // note bgr channel order in OpenCV
					break;
			default: 
				std::cerr << "option --swap: third character of --channels must be either 'r', 'g' or 'b', aborted." << std::endl;
				return 1;
		}

		if (imageRaw.depth() == 0) { // 8 bit image
			cv::Mat_<cv::Vec3b> inputImage = imageRaw;
			cv::Mat_<cv::Vec3b> mat(inputImage.rows, inputImage.cols);
			for (int y = 0; y < inputImage.rows; ++y) {
				for (int x = 0; x < inputImage.cols; ++x) {
					for (int c = 0; c < 3; ++c)
						mat[y][x][c] = inputImage[y][x][c];
					mat[y][x][source] = inputImage[y][x][target];
					mat[y][x][target] = inputImage[y][x][source];
				}
			}
			cv::imwrite(config.output_file, mat);
		} else { // depth == 2: 16 bit image
			cv::Mat_<cv::Vec3w> inputImage = imageRaw;
			cv::Mat_<cv::Vec3w> mat(inputImage.rows, inputImage.cols);
			for (int y = 0; y < inputImage.rows; ++y) {
				for (int x = 0; x < inputImage.cols; ++x) {
					for (int c = 0; c < 3; ++c)
						mat[y][x][c] = inputImage[y][x][c];
					mat[y][x][source] = inputImage[y][x][target];
					mat[y][x][target] = inputImage[y][x][source];
				}
			}
			cv::imwrite(config.output_file, mat);
		}
	}

	if (config.verbosity > 0) {
		std::cout << "wrote image to " << config.output_file << std::endl;
	}

	return 0;
}


void CommandSwapChannel::printShortHelp() const {
	std::cout << "Perform a simple channel operation (like swapping) on an RGB image" << std::endl;
}

void CommandSwapChannel::printHelp() const {
	std::cout << "Perform a simple channel operation (like swapping) on an RGB image" << std::endl;
}


}
