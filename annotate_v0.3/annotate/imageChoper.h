#pragma once

#include <ImgAnnotations.h>
#include <opencv2/imgproc/imgproc.hpp>

class ImageChoper
{
    public:
        ImageChoper(){}
        ~ImageChoper(){}
        static cv::Mat chopOneFromImage(IA::ImgAnnotations *ia,
                                        IA::ID id,
                                        std::string curPath);
    private:
};
