#include "recogThread.h"
#include <QDebug>
#include "util.h"
#include "fisher.h"

RecogThread::RecogThread(RecogModel **rm, puhma::AttributeEmbed *ae, cv::Mat img, RecogResult *rr)
    :rm(rm), ae(ae), img(img), rr(rr){
}

void RecogThread::run(){
    if(*rm == nullptr){
        *rm = new RecogModel(ae);
    }
    cv::Mat desc;
    puhma::DataPrepare::prepareIm(img, ae->config.heightIm);
    puhma::FV *fv = new puhma::FV(ae->config, ae->filePathManager, ae->st);
    cv::Mat fisher = fv->computeOneFV(img, (*rm)->pca, (*rm)->meansEM, (*rm)->covsEM, (*rm)->weightsEM, desc);
    fisher.convertTo(fisher,CV_64F);    
    cv::Mat attriRep = (*rm)->attM->W * fisher.t();
    attriRep = attriRep.t()+(*rm)->attM->B;
    rr->candi = ae->recog4Annotation(attriRep);
}


