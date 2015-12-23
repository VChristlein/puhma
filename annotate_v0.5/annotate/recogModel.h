#pragma once
#ifndef REECOG_MODEL_H
#define REECOG_MODEL_H

#include <QString>

#ifndef Q_MOC_RUN
#include "attributes_core.h"
#include "attributes_model.h"
#endif


class RecogModel{
public:
    RecogModel(puhma::AttributeEmbed *ae);
    ~RecogModel(){}

    cv::PCA pca;
    cv::Mat meansEM;
    std::vector<cv::Mat> covsEM;
    cv::Mat weightsEM;
    puhma::AttriModel *attM;

    //result:
    //std::vector<std::string> results;
};

struct RecogResult{
    std::vector<std::string> candi;
    QString result;
};
#endif
