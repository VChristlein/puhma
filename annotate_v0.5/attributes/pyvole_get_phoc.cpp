#include "trans_phoc.h"

#include <boost/python.hpp>
#include <boost/python/suite/indexing/vector_indexing_suite.hpp>

namespace pyvole {
	BOOST_PYTHON_MODULE(_getphoc){
		using namespace boost::python;
		docstring_options local_docstring_options(true, true, false);
		def("get_phoc", puhma::TransPhoc::getPhoc,
				"produce PHOC of given transcription, params: transcription");
	}
}
