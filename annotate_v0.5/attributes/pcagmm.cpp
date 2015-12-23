#include "pcagmm.h"
#include "esift.h"

#include <opencv2/highgui/highgui.hpp>

#include <sstream>
#include <QDebug>
#include <QDir>
#include <QFileInfo>

extern "C" {
#include <vl/gmm.h>
}

namespace puhma{

PCAGMM :: PCAGMM(const AttriConfig &c, const FileHandler *fpm, Statistic *const st)
    :config(c), filePathManager(fpm), st(st){
    }
PCAGMM :: ~PCAGMM(){}

void PCAGMM :: get(DataPrepare *dp, cv::PCA &pca,
        cv::Mat &meansEM, std::vector<cv::Mat> &covsEM, cv::Mat &weightsEM){
    int ins4PCA = 200000;
    meansEM.release();
    covsEM.clear();
    weightsEM.release();
    //check if file exist
    if(filePathManager->fileExist("gmm")&&filePathManager->fileExist("pca")){
        qDebug()<<"GMM and PCA files already exist, restoring from File...";
        st->setStatus("Restoring GMM and PCA...");
        restoreGMM(meansEM,covsEM,weightsEM);
        restorePCA(pca);
    }else{
        //TODO: clean the old data which is produced by gmm and PCA with different setting.
        std::vector<std::vector<std::string> > dataGMM = dp->getRandomData(config.numWordsTranGMM);
        cv::Mat descs4PCA;
        std::vector<cv::Mat> descsInRegion;
        qDebug()<<"Computing Local Descriptor...";
        st->setStatus("Computing Local Descriptor...");
        //foreach image, extraction enrichted sift descriptor, and sign descriptor to region.
        foreach(auto data, dataGMM){
            std::string p = QFileInfo(QDir(QString::fromStdString(config.inputfolder)),QString::fromStdString(data[0])).filePath().toStdString();
            //std::cout<<p<<std::endl;
            cv::Mat im = DataPrepare::prepareIm(p, std::stoi(data[2]), config.heightIm);
            ESIFT* eSIFT = new ESIFTPhow();
            cv::Mat desc; //descriptor
            eSIFT->compute(im,desc);
            if(desc.rows == 0){
                //skip the sample that do not contain any keypoint.
                continue;
            }else{
                asignDesForPCA(desc.clone(),descs4PCA,ins4PCA);
                asignDesToRegion(desc.clone(),descsInRegion);
            }
        }
        qDebug()<<"Done.";
        //exist PCA?
        if(filePathManager->fileExist("pca")){
            qDebug()<<"PCA file already exists. Restoring from File...";
            st->setStatus("PCA file already exists. Restoring from File...");
            restorePCA(pca);
            qDebug()<<"Done.";
        }else{
            qDebug()<<"Computing PCA...";
            st->setStatus("Computing PCA...");
            //select maximum 2000000 feature from data.
            //std::cout<<descs4PCA.rows<<std::endl;
            /*if(descs4PCA.rows>ins4PCA){
                qDebug()<<"ramdomly choose "<<ins4PCA<<" of descriptor to compute PCA...";
                std::vector<int> ri = Util :: generateRan(ins4PCA, descs4PCA.rows);
                std::sort(ri.begin(), ri.end());
                cv::Mat descs4PCAt(ins4PCA, descs4PCA.cols, descs4PCA.type());
                for(int i=0; i<ins4PCA; ++i){
                    descs4PCA.row(ri[i]).copyTo(descs4PCAt.row(i));
                }
                descs4PCA = descs4PCAt;
                qDebug()<<"Done."<<descs4PCA.rows;
            }*/
            CV_Assert(descs4PCA.isContinuous());
            pca = cv::PCA(descs4PCA.colRange(0,128).clone(),cv::Mat(),CV_PCA_DATA_AS_ROW,config.dimPCA);
            //write out pca.
            writeoutPCA(pca);
            qDebug()<<"Done.";
        }
        if(filePathManager->fileExist("gmm")){
            qDebug()<<"GMM file already exists. Restoring from File...";
            st->setStatus("GMM file already exists. Restoring from File...");
            restoreGMM(meansEM,covsEM,weightsEM);
            qDebug()<<"Done.";
        }else{
            qDebug()<<"Training GMM...";
            st->setStatus("Training GMM...");
            foreach(cv::Mat m, descsInRegion){
                cv::Mat xy = m.colRange(128,130);
                m = m.colRange(0,128);
                pca.project(m,m);
                m = m.t();
                xy = xy.t();
                m.push_back(xy);
                m = m.t();
                CV_Assert(m.isContinuous());
                gmmWithVlfeat(m,meansEM,covsEM,weightsEM);
                //gmmWithOpenCV(m,meansEM,covsEM,weightsEM);
            }
            //normalize the weightEM
            weightsEM = weightsEM/(cv::sum(weightsEM)[0]);
            writeoutGMM(meansEM,covsEM,weightsEM);
        }
    }
    qDebug()<<"Done. EM model has number of Clusters: "<<covsEM.size()<<" and by PCA reduced Dimension: "<< meansEM.cols <<".";
    st->setStatus("");
}

void PCAGMM :: writeoutPCA(const cv::PCA &pca)
{
    QString fn = QString::fromStdString(filePathManager->pcaPath); 
    cv::FileStorage fs(fn.toStdString(), cv::FileStorage::WRITE);
    fs<<"mean"<<pca.mean;
    fs<<"eigenvectors"<<pca.eigenvectors;
    fs<<"eigenvalues"<<pca.eigenvalues;
    fs.release();
}

void PCAGMM :: restorePCA(cv::PCA &pca){
    QString fn = QString::fromStdString(filePathManager->pcaPath); 
    cv::FileStorage fs(fn.toStdString(), cv::FileStorage::READ);
    if (!fs.isOpened())
    {
        throw std::runtime_error("PCAGMM::restorePCA_error: can not open file");
    }
    fs["mean"]>>pca.mean;
    fs["eigenvectors"]>>pca.eigenvectors;
    fs["eigenvalues"]>>pca.eigenvalues;
    fs.release();
}

void PCAGMM :: gmmWithOpenCV(const cv::Mat &s,
            cv::Mat &meansEM,
            std::vector<cv::Mat> &covsEM,
            cv::Mat &weightsEM)
{
    cv::TermCriteria term(CV_TERMCRIT_ITER | CV_TERMCRIT_EPS, 60, 1e-6);
    cv::EM em(config.clusterGMM,cv::EM::COV_MAT_SPHERICAL,term);
    em.train(s);
    cv::Mat means = em.get<cv::Mat>("means");
    meansEM.push_back(means);
    std::vector<cv::Mat> covs = em.get<std::vector<cv::Mat> >("covs");
    covsEM.insert(covsEM.end(), covs.begin(), covs.end());
    covs.clear();
    cv::Mat weights = em.get<cv::Mat>("weights");
    weights = weights.t();
    weightsEM.push_back(weights);
}

void PCAGMM :: gmmWithVlfeat(const cv::Mat &m,
            cv::Mat &meansEM,
            std::vector<cv::Mat> &covsEM,
            cv::Mat &weightsEM)
{
    double * means ;
    double * covariances ;
    double * priors ;
    vl_size dimension = static_cast<vl_size>(m.cols);
    vl_size numData = static_cast<vl_size>(m.rows);
    vl_size numClusters = static_cast<vl_size>(config.clusterGMM);
    cv::Mat mt = m.clone();
    mt.convertTo(mt,CV_64F);
    CV_Assert(m.isContinuous());

    // create a new instance of a GMM object for float data
    VlGMM *gmm = vl_gmm_new (VL_TYPE_DOUBLE, dimension, numClusters) ;
    // set the maximum number of EM iterations to 30
    vl_gmm_set_max_num_iterations (gmm, 30);
    // set the num of EM repetition to 2
    vl_gmm_set_num_repetitions (gmm, 2);
    // set the initialization to random selection
    vl_gmm_set_initialization (gmm, VlGMMRand);
    // cluster the data, i.e. learn the GMM
    vl_gmm_cluster (gmm, (double*)mt.data, numData);
    // get the means, covariances, and priors of the GMM
    means = (double *)vl_gmm_get_means(gmm);
    cv::Mat means1Grid = cv::Mat(numClusters,dimension,CV_64FC1,means);
    meansEM.push_back(means1Grid);
    covariances = (double *)vl_gmm_get_covariances(gmm);
    cv::Mat covs1Grid = cv::Mat(numClusters,dimension,CV_64FC1,covariances);
    for(int y=0;y<covs1Grid.rows; ++y){
        cv::Mat covs1Gau = cv::Mat::zeros(dimension,dimension,CV_64FC1);
        for(int x=0;x<covs1Grid.cols; ++x){
            covs1Gau.at<double>(x,x)=covs1Grid.at<double>(y,x);
        }
        covsEM.push_back(covs1Gau);
    }
    priors = (double *)vl_gmm_get_priors(gmm);
    cv::Mat weights = cv::Mat(numClusters,1,CV_64FC1,priors);
    weightsEM.push_back(weights);

    vl_gmm_delete(gmm);
    
}

void PCAGMM :: writeoutGMM(const cv::Mat &m,
            const std::vector<cv::Mat> &covs,
            const cv::Mat &weights){
    QString fn = QString::fromStdString(filePathManager->gmmPath); 
    cv::FileStorage fs(fn.toStdString(), cv::FileStorage::WRITE);
    fs<<"mean"<<m;
    for(size_t i=0; i<covs.size(); ++i){
        std::ostringstream oss;
        oss << i;
        std::string t = "covs_"+oss.str();
        fs << t << covs[i];
    }
    fs<<"weight"<<weights;
    fs.release();
}

void PCAGMM :: restoreGMM(cv::Mat &m,
            std::vector<cv::Mat> &covs,
            cv::Mat &weights){
    QString fn = QString::fromStdString(filePathManager->gmmPath); 
    cv::FileStorage fs(fn.toStdString(), cv::FileStorage::READ);
    if (!fs.isOpened())
    {
        throw std::runtime_error("PCAGMM::restoreGMM_error: can not open file");
    }
    fs["mean"]>>m;
    int vectorS = config.numSpatialX*config.numSpatialY*config.clusterGMM;
    for(size_t i=0; i<vectorS; ++i){
        std::ostringstream oos;
        cv::Mat mt;
        oos << i;
        std::string t = "covs_"+oos.str();
        fs[t] >> mt;
        covs.push_back(mt);
    }
    fs["weight"]>>weights;
    fs.release();
}

void PCAGMM :: asignDesToRegion(const cv::Mat &desc,
        std::vector<cv::Mat >&dr){
    int numSpatialX=config.numSpatialX;
    int numSpatialY=config.numSpatialY;
    dr.resize(numSpatialX*numSpatialY);
    for(int y=0; y<desc.rows; ++y){
        float xp = desc.at<float>(y,128);
        float yp = desc.at<float>(y,129);
        int regionX = static_cast<int>((xp+0.5)*numSpatialX);
        regionX = regionX<0?0:(regionX>(numSpatialX-1)?(numSpatialX-1):regionX);
        int regionY = static_cast<int>((yp+0.5)*numSpatialY);
        regionY = regionY<0?0:(regionY>(numSpatialY-1)?(numSpatialY-1):regionY);
        int ind = regionY*numSpatialX+regionX;
        dr[ind].push_back(desc.row(y).clone());
    }
}

void PCAGMM :: asignDesForPCA(const cv::Mat &desc,
            cv::Mat &dpca,
            const int numTrainPCA){
    if(desc.rows>numTrainPCA){
        throw std::runtime_error("PCAGMM::not enough number SIFT for train PCA");
    }
    cv::Mat descToInsert= desc.clone();
    if(dpca.rows+desc.rows<numTrainPCA){
        dpca.push_back(descToInsert);
        return;
    }
    else if(dpca.rows<numTrainPCA){
        dpca.push_back(descToInsert.rowRange(0,numTrainPCA-dpca.rows));
        descToInsert = descToInsert.rowRange(numTrainPCA-dpca.rows, descToInsert.rows);
    }
    std::vector<int> geneV = Util::generateRan(descToInsert.rows, numTrainPCA);
    for(int i=0; i<geneV.size();++i){
        descToInsert.row(i).copyTo(dpca.row(geneV[i]));
    }
}

}
