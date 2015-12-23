#pragma once

#include <opencv2/imgproc/imgproc.hpp>

namespace puhma{

class CCA{
public:
    CCA(){};
    ~CCA(){};
    /// compute the canonical correlations, where each row of att is one data and each cols as one dimension, so as phoc
    static void compute(const cv::Mat &att, const cv::Mat &phoc, const double reg,
            cv::Mat &Watt, cv::Mat &Wphoc);

    static void normalizePerRow(cv::Mat &matToNor);

    //calculate mean center each col, and return mean matrix.
    static cv::Mat1d meanCenter(cv::Mat &matToMean);
    //calculate mean center each col by given mean matrix.
    static void meanCenter(cv::Mat &matToMean, const cv::Mat &mean);
};

}
