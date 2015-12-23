#pragma once
#ifndef IMAGE_CHOPER_H
#define IMAGE_CHOPER_H

#include <ImgAnnotations.h>
#include <opencv2/imgproc/imgproc.hpp>

class ImageChoper
{
    public:
        ImageChoper(){}
        ~ImageChoper(){}
        static cv::Mat chopOneFromImage(IA::ImgAnnotations *ia,
                                        IA::ID id,
                                        const std::string & curPath);
        static cv::Rect getBBox(const IA::Object * obj,
                                   cv::Mat1b &mask);
private:
};

#endif
