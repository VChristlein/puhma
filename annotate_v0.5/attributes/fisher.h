#pragma once
#ifndef FVV_H
#define FVV_H

#include <opencv2/imgproc/imgproc.hpp>

namespace puhma {
class FileHandler;
class Statistic;
class AttriConfig;

class FisherV {
public:
    FisherV (const AttriConfig & c,
             const FileHandler *fpm,
             Statistic *const st);
    ~FisherV (){}
    ///extract FV from daten.
    void compute(const std::vector<std::vector<std::string> > &data,
                 const cv::PCA pca,
                 const cv::Mat &mGMM,
                 const std::vector<cv::Mat> &covsGMM,
                 const cv::Mat &weightsGMM);
    cv::Mat computeOneFV(const cv::Mat &source,
                         const cv::PCA pca,
                         const cv::Mat &mGMM,
                         const std::vector<cv::Mat> &covsGMM,
                         const cv::Mat &weightsGMM,
                         cv::Mat &desc);
    ///fetch one fv row vector according to given file name.
    cv::Mat fetchOneFeature(const std::string fn) const;
    ///return the size of fv vector
    const cv::Size getSize(){ return sizeOfFV; }

private:
    cv::Size sizeOfFV;
    const AttriConfig  & config;
    const FileHandler *filePathManager;
    Statistic *const st;

    //void normalizeSift(cv::Mat &s);
    //bool fileExists();
};

} // end namespace
#endif
