#pragma once
#include "attributes_config.h"
#include "util.h"
#include "statistic.h"

#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/ml/ml.hpp>

namespace puhma {

/*! brief this Class implement the PCA and GMM processing for attribute embedding.
 */
class PCAGMM
{
public:
    PCAGMM(const AttriConfig &c, const FileHandler *fpm, Statistic *const st);
    ~PCAGMM();
    /// get PCA model and parameter of GMM model
    void get(DataPrepare *dp, cv::PCA &pca, 
            cv::Mat &EMmean, std::vector<cv::Mat> &EMcovs, cv::Mat &EMweight);
private:
    const AttriConfig config;
    const FileHandler *filePathManager;
    Statistic *const st;
    /// asign descriptors to specific region according to its position
    void asignDesToRegion(const cv::Mat &desc,
        std::vector<cv::Mat> &dr);

    void writeoutPCA(const cv::PCA &pca);
    void restorePCA(cv::PCA &pca);

    ///calculate gmm using EM from opencv
    void gmmWithOpenCV(const cv::Mat &m,
            cv::Mat &means,
            std::vector<cv::Mat> &covs,
            cv::Mat &weights);
    ///calculate gmm using gmm from vl_feat
    void gmmWithVlfeat(const cv::Mat &m,
            cv::Mat &means,
            std::vector<cv::Mat> &covs,
            cv::Mat &weights);

    void writeoutGMM(const cv::Mat &s,
            const std::vector<cv::Mat> &covs,
            const cv::Mat &weights);
    void restoreGMM(cv::Mat &m,
            std::vector<cv::Mat> &covs,
            cv::Mat &weights);
};


}
