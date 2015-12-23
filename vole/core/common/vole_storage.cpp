/*	
	Copyright(c) 2010 Christian Riess <christian.riess@cs.fau.de>.

	This file may be licensed under the terms of of the GNU General Public
	License, version 3, as published by the Free Software Foundation. You can
	find it here: http://www.gnu.org/licenses/gpl.html
*/

#include "vole_storage.h"

namespace vole {

void Storage::write(const char *filename, cv::Mat blob, const char *description) {
	cv::FileStorage fs(filename, cv::FileStorage::WRITE);
	fs << description << blob;
	fs.release();
}

cv::Mat Storage::read(const char *filename, const char *description) {
	cv::FileStorage fs(filename, cv::FileStorage::READ);
	cv::Mat m;
	fs[description] >> m;
	fs.release();

	return m;
}

}
