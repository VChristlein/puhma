#pragma once

#include "attributes_config.h"

#include <opencv2/imgproc/imgproc.hpp>

namespace puhma{

class Evaluate{
public:
    Evaluate(){};
    ~Evaluate(){};
    static void computemAP(const AttriConfig &config,
            const cv::Mat &queries,
            const cv::Mat &dataset,
            const cv::Mat &allClasses,
            const std::vector<std::string> &queriesWords,
            cv::Mat1d &mAP,
            cv::Mat1d &p1,
            const bool doqbs = false
            );

    ///online evaluating version
    // return < 0 means stop word, process abort
    // return = 0 means no relavant exists, process abort
    // return > 0 means evaluate processed.
    static int computemAP2(const AttriConfig &config,
            const int idx,
            const cv::Mat &querie,
            const cv::Mat &dataset,
            const cv::Mat &allClasses,
            const std::vector<std::string> &queriesWords,
            const std::vector<std::string> &sword,
            cv::Mat1d &mAP,
            cv::Mat1d &p1,
            const bool doqbs = false
            );

    static void computeState(const cv::Mat &score, 
            const cv::Mat &queriesClass,
            const cv::Mat &allClasses,
            const cv::Mat &nRelevantsPerQuery,
            const std::vector<int> &keep,
            cv::Mat1d &pMap,
            cv::Mat1d &pP1);

    static void computePhocClassNumber(
            const cv::Mat &attriRep,
            const cv::Mat &phocs,
            const cv::Mat &label);

private:
    //static void showHist(const cv::MatND &hist);
};

}
