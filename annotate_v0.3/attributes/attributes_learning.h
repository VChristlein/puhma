#pragma once

#include "attributes_config.h"
#include "util.h"
#include "attributes_model.h"
#include "fisher.h"
#include "statistic.h"
#include "trans_phoc.h"

#include <opencv2/imgproc/imgproc.hpp>

//#include <QString>

namespace puhma {
class AttributesLearning
{
public:
    AttributesLearning(const AttriConfig &config, 
            const FileHandler *fpm, const DataPrepare * dp, Statistic *const st);
    ~AttributesLearning();

    ///evalutate CSR with Online Updating attributes model
    void evalComSubWithOnline();

    void dumpAttRepOnline(const std::string &type, const cv::Mat1d &rep) const;
    //void reloadAttRep(std::string type);

private:
    const AttriConfig config;
    const FileHandler *filePathManager;
    Statistic *const st;
    FV *fv;
    TransPhoc *tp;

    std::vector<std::vector<std::string> > dataTrain;
    std::vector<std::vector<std::string> > dataTest;
    std::vector<std::vector<std::string> > dataValidation;
    std::vector<std::vector<std::string> > dataOnline;

    Embedding embedding;

    ///generate evaluate result of non-online for purpose of comparison
    void evalComSubWithOffline(const std::vector<int> &idxList,
            const cv::Mat1d &rep,
            const cv::Mat1d &phoc,
            const cv::Mat &wordCls,
            const std::vector<std::string> &label,
            const std::vector<std::string> &swords,
            std::vector<double> &mAPqbeAcc_old,
            std::vector<double> &p1qbeAcc_old,
            std::vector<double> &mAPqbsAcc_old,
            std::vector<double> &p1qbsAcc_old);

    ///writeout&restore division info.
    /*void writeout(std::string path, const std::vector<int> &idxList) const;
    void restore(const std::string path, std::vector<int> &idxList);*/

    bool updateCS(
            const cv::Mat1d &attriRepTr,
            const cv::Mat1d &attriRepTe,
            const cv::Mat &phocs,
            const cv::Mat &phocsTe,
            const cv::Mat &wordClsTe,
            const std::vector<std::string> labelTe);

    /*void dumpIdx(const int numAttri,
            const std::vector<int> idxP,
            const std::vector<int> idxN);*/

    bool learnAttriWarm(AttriModel1D* am1d,
            cv::Mat &featuresTr,
            cv::Mat &phocTr,
            cv::Mat &featuresVal,
            cv::Mat &phocVal,
            const double rate);

    void sgdTrainningWarm(
            const cv::Mat &data,
            const cv::Mat &label,
            cv::Mat1d &model,
            double &bias,
            int &numIters,
            double epsilon=0.001,
            double bias_multiplier=0.1
            );

};
}
