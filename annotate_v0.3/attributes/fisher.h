#pragma once
#include "attributes_config.h"
#include "util.h"
#include "statistic.h"

#include <opencv2/imgproc/imgproc.hpp>

namespace puhma {
class FV {
public:
    FV (const AttriConfig &c, const FileHandler *fpm, Statistic *const st);
    ~FV ();
    ///extract FV from daten.
    void compute(const std::vector<std::vector<std::string> > &data,
            const cv::PCA pca,
            const cv::Mat &mGMM,
            const std::vector<cv::Mat> &covsGMM,
            const cv::Mat &weightsGMM);
    ///fetch one fv row vector according to given file name.
    cv::Mat fetchOneFeature(const std::string fn) const;
    ///return the size of fv vector
    const cv::Size getSize(){return sizeOfFV;};

private:
    cv::Size sizeOfFV;
    const AttriConfig config;
    const FileHandler *filePathManager;
    Statistic *const st;

    //void normalizeSift(cv::Mat &s);
public:
    cv::Mat computeOneFV(const cv::Mat &source,
            const cv::PCA pca,
            const cv::Mat &mGMM,
            const std::vector<cv::Mat> &covsGMM,
            const cv::Mat &weightsGMM,
            cv::Mat &desc);
    //bool fileExists();
};
}
