#include "fisher.h"
#include "esift.h"


#include <opencv2/highgui/highgui.hpp>

#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <iostream>
#include <sstream>

extern "C" {
    #include <vl/fisher.h>
}

namespace puhma {
FV :: FV(const AttriConfig &c, const FileHandler *fpm, Statistic *const st)
    :config(c), filePathManager(fpm), st(st){
        sizeOfFV = cv::Size(2*(config.dimPCA+2)*config.numSpatialX*config.numSpatialY*config.clusterGMM, 1);
    }
FV :: ~FV(){}

void FV :: compute(const std::vector<std::vector<std::string> > &data,
            const cv::PCA pca,
            const cv::Mat &mGMM,
            const std::vector<cv::Mat> &covsGMM,
            const cv::Mat &weightsGMM)
{
    int number, processednum;
    number = data.size();
    processednum = 1;
    std::string dirname = filePathManager->featsDir;
    //if folder not exist, create one
    if(!(filePathManager->dirExist("feature"))){
        if(!(QDir(QString::fromStdString(dirname)).mkpath("."))){
            throw std::runtime_error("FV::compute : can not create Dir.");
        }
    }
    qDebug()<<"Computing Fisher Vector... This process may take some time.";

    for(auto datum : data){
        st->setStatus("Generating Fisher Vector... ("+std::to_string(processednum)+"/"+std::to_string(number)+")");
        QString fn = QString::fromStdString(datum[0])+".yaml";
        QFileInfo fileFeature = QFileInfo(QDir(QString::fromStdString(dirname)),fn);
        if(!(fileFeature.exists())){
            std::string p = QFileInfo(QDir(QString::fromStdString(config.inputfolder)),QString::fromStdString(datum[0])).filePath().toStdString();
            //std::cout<<p<<std::endl;
            cv::Mat im = DataPrepare::prepareIm(p, std::stoi(datum[2]), config.heightIm);

            cv::Mat desc;
            cv::Mat fisher = computeOneFV(im, pca, mGMM, covsGMM, weightsGMM, desc);
            if(fisher.rows > 0){
                fn = fileFeature.filePath();
                cv::FileStorage fs(fn.toStdString(), cv::FileStorage::WRITE);
                fs << "fisher" << fisher;
                //fs << "desc" << desc;
                fs.release();
            }
        }
        processednum++;
    }

    qDebug()<<"Done.";
    st->setStatus("");

}

cv::Mat FV::computeOneFV(const cv::Mat &im,
        const cv::PCA pca,
        const cv::Mat &mGMM,
        const std::vector<cv::Mat> &covsGMM,
        const cv::Mat &weightsGMM,
        cv::Mat &rdesc){
    ESIFT* eSIFT = new ESIFTPhow();
    cv::Mat desc; //descriptor
    eSIFT->compute(im,desc);
    if(desc.rows==0){
        cv::Mat no_re(0,0,CV_32F);
        return no_re;
    }
    //use PCA to reduce the dim.
    cv::Mat xy = desc.colRange(128,130);
    desc = desc.colRange(0,128);
    pca.project(desc,desc);
    desc = desc.t();
    xy = xy.t();
    desc.push_back(xy);
    desc = desc.t();
    CV_Assert(desc.isContinuous());
    //data
    float *d = (float*) desc.data;


    //fisher vector
    //prepare the parameter
    int dimension = config.dimPCA+2;
    int numCluster = config.numSpatialX*config.numSpatialY*config.clusterGMM;
    float *vlm;
    vlm = (float *)vl_malloc(sizeof(float)*2*dimension*numCluster);

    //mean
    cv::Mat mGMMt = mGMM.clone();
    mGMMt.convertTo(mGMMt,CV_32F);
    CV_Assert(mGMMt.isContinuous());
    float *mean = (float *)mGMMt.data;

    //covs
    cv::Mat covsFV = cv::Mat::zeros(covsGMM.size(),covsGMM[0].rows,CV_32FC1);
    for(size_t i=0; i<covsGMM.size(); ++i){
        for(int j=0; j<covsGMM[i].rows; ++j){
            covsFV.at<float>(i,j) = static_cast<float>(covsGMM[i].at<double>(j,j));
        }
    }
    CV_Assert(covsFV.isContinuous());
    float *cov = (float *)covsFV.data;

    //prior
    cv::Mat prior;
    weightsGMM.convertTo(prior,CV_32F);
    CV_Assert(prior.isContinuous());
    float *pri = (float*) prior.data;
    
    vl_fisher_encode(vlm,
            VL_TYPE_FLOAT,
            mean,
            dimension,
            numCluster,
            cov,
            pri,
            d,
            desc.rows,
            VL_FISHER_FLAG_IMPROVED
            );

    cv::Mat fisher(sizeOfFV, CV_32FC1, vlm);
    Util :: nanTo0f(fisher);
    rdesc = desc;
    delete eSIFT;
    return fisher;
}

cv::Mat FV :: fetchOneFeature(const std::string fn) const
{
    cv::Mat re;
    QString filep = QString::fromStdString(fn)+".yaml";
    filep = QFileInfo(QDir(QString::fromStdString(filePathManager->featsDir)), filep).filePath();
    cv::FileStorage fs(filep.toStdString(), cv::FileStorage::READ);
    if (!fs.isOpened())
    {
        //throw std::runtime_error("Attri_Learn::fetchFeature_error: can not open file");
        re = cv::Mat::zeros(sizeOfFV, CV_32F);
    }else{
        fs["fisher"]>>re;
    }
    fs.release();
    return re;
}

} 
