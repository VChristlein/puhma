#include "felzenszwalb/felzenszwalb_segmentation.h"
#include "superpixel.h"

#include <boost/python.hpp>
#include <boost/python/suite/indexing/vector_indexing_suite.hpp>
#include <boost/foreach.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/core/core.hpp>


namespace pyvole {

  using namespace superpixels;

  // wrapper for easy use of Felzenszwalb segmentation
  cv::Mat fzs(cv::Mat image, int min_size, int kThresh = 500, double sigma = 0.5){
    FelzenszwalbSegmentation felzenszwalb(min_size,kThresh,sigma);
    cv::Mat_<cv::Vec3b>  Mat(image);
    std::vector<Superpixel> supixels = felzenszwalb.superpixels(Mat);
    return felzenszwalb.superpixelsImage(supixels,image.rows,image.cols,false);
  }

  BOOST_PYTHON_FUNCTION_OVERLOADS(fzs_overloads, fzs,2,4);
    
  // Wrappers for overloaded functions of the class FelzenszwalbSegmentation
  std::vector<Superpixel> (FelzenszwalbSegmentation::*fzsp1)(const cv::Mat_<cv::Vec3b>&) = &FelzenszwalbSegmentation::superpixels;
  cv::Mat_<cv::Vec3b> (FelzenszwalbSegmentation::*fzspI)(const std::vector<Superpixel>&, int, int, bool) = &FelzenszwalbSegmentation::superpixelsImage;
    
  struct superpixelVec_to_list {
	static PyObject* convert(const std::vector<Superpixel>& v){
		boost::python::list ret;
		BOOST_FOREACH(const Superpixel& s, v) {
			boost::python::list sl;
			BOOST_FOREACH(const cv::Point& p, s.coordinates){
				sl.append(boost::python::make_tuple(p.x, p.y));
			}
			ret.append(sl);
		}
		return boost::python::incref(ret.ptr());
	}
  };

  /*
    expose felzenszwalb. note the "_" before the module name to keep the package
    structure in order. The module name MUST BE EQUAL to the one provided
    in the vole_add_python_module in the CMakeLists.txt!!!
  */
  BOOST_PYTHON_MODULE(_felzenszwalb){
    using namespace boost::python;

    to_python_converter<std::vector<Superpixel>, superpixelVec_to_list>();
	docstring_options doc_options(true, true, false);

    // expose a function: def("name in python",function,(optional) "user defined doc_string")
    def("segment_felzenszwalb",fzs,fzs_overloads(
						 args("img", "min_size", "kThresh", "sigma"),
						 "Superpixel Segmentation by Felzenszwalb & Huttenlocher"
						 ));

    /*
      expose a class: init<> behind the name describes the default constuctor,
      others can be added in the next lines.
      .def exposes methods, see above for syntax
    */
    class_<superpixels::FelzenszwalbSegmentation>("FelzenszwalbSegmentation", init<int>(args("minSize")))
	.def(init<int, int>((args("minSize"), args("kThreshold"))))
	.def(init<int, int, double>((args("minSize"), args("kThreshold"), args("sigma"))))
      .def("superpixels", fzsp1)
      .def("superpixelsImage", fzspI)
      ;
      
  }

}
