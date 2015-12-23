#pragma once

#include "attributes_config.h"
#include "util.h"
#include "attributes_model.h"
#include "fisher.h"
#include "trans_phoc.h"
#include "statistic.h"

#include <opencv2/imgproc/imgproc.hpp>

namespace puhma {
class AttriSpace{
public:
    AttriSpace(const AttriConfig &config, 
            const FileHandler *fpm, 
            const DataPrepare *dp,
            Statistic *st);
    ~AttriSpace(){};
    /// training attribute space, low mem cost version.
    void train();

    /// get attribute score of one type, can be "train" and "test"
    static cv::Mat1d getRep(std::string type, const FileHandler *fpm);
    ///use for evaluate the learned attribute
    static double modelMap(const cv::Mat1d &score, const cv::Mat1d &label);

    AttriModel *attModel;
private:
    /// training one attribute with transcriptors.
    void train1d(const int numAttri,
            cv::Mat &featuresTr,
            cv::Mat &featuresVal,
            cv::Mat &phocTr,
            cv::Mat &phocVal,
            std::vector<int> &idxTrMapping,
            std::vector<int> &idxValMapping);


    ///writeout score of Attributes represent.
    /*
     * strings acceptable for type are "train" and "test"
     * */
    void dumpAttRep(const std::string &type, const cv::Mat1d &score) const;


    const AttriConfig config;
    const FileHandler *filePathManager;
    FV *fv;
    TransPhoc *tp;
    Statistic *st;

    //cv::Mat1d attriRepTr;
    //cv::Mat1d attriRepTe;
    //cv::Mat1d attriRepVa;

    std::vector<std::vector<std::string> > dataTrain;
    std::vector<std::vector<std::string> > dataTest;
    std::vector<std::vector<std::string> > dataValidation;
};
}
