#include "attributes_core.h"
#include "trans_phoc.h"
#include "esift.h"
#include "pcagmm.h"
#include "fisher.h"
#include "attributes_learning.h"
#include "attributes_model.h"
#include "attrispace.h"
#include "calibrate.h"
#include "statistic.h"

#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/features2d/features2d.hpp>
#include <opencv2/nonfree/features2d.hpp>
#include <opencv2/ml/ml.hpp>
#include <iostream>

#include <QDebug>
#include <QDir>
#include <QFileInfo>

namespace puhma{
    
AttributeEmbed :: AttributeEmbed(AttriConfig &c) 
    : config(c)
{
    filePathManager = new FileHandler(config);
    st = new Statistic(config);
    reco4anno = nullptr;
}

AttributeEmbed :: ~AttributeEmbed(){}
    

/*void AttributeEmbed :: run(const cv::Mat &img)
{
}*/

void AttributeEmbed :: prepare()
{
    dataprepare = new DataPrepare(config, filePathManager);
    dataprepare->prepare();
    dataprepare->prepareDivision();
}

int AttributeEmbed :: check(){
    int status = -1;
    if(filePathManager->fileExist("lexicon")&&
            filePathManager->fileExist("csrModel")&&
            filePathManager->fileExist("attModel")&&
            filePathManager->fileExist("pca")&&
            filePathManager->fileExist("gmm")&&
            filePathManager->fileExist("classes")&&
            filePathManager->dirExist("phoc")){
        prepare();
        status = 9;
    }else if(!filePathManager->fileExist("classes")||!filePathManager->fileExist("division")){
        status = 0;
    }
    else{
        prepare();
    }
    return status;
}

void AttributeEmbed :: labelEmbed()
{
    TransPhoc *phoccreator = new TransPhoc(config, filePathManager);
    phoccreator->compute(dataprepare->getData());
}

void AttributeEmbed :: fvRep()
{
    if(config.inputfolder == ""){
        throw std::runtime_error("AttriLearn :: AttributeEmbed : input folder not given.");
    }
    cv::PCA pca;
    cv::Mat meansEM;
    std::vector<cv::Mat> covsEM;
    cv::Mat weightsEM;
    PCAGMM *pcagmm = new PCAGMM(config, filePathManager, st); 
    pcagmm->get(dataprepare, pca, meansEM, covsEM, weightsEM);
    delete pcagmm;

    //fishervector
    FisherV *fv = new FisherV(config, filePathManager, st);
    fv->compute(dataprepare->getData(), pca, meansEM, covsEM, weightsEM);
}

void AttributeEmbed :: trainAS()
{
    AttriSpace *as = new AttriSpace(config, filePathManager, dataprepare, st);
    as->train();
}

void AttributeEmbed :: calibrate()
{
    Calibrate *cali = CalibrateFac::get()->initalCali(config, filePathManager, dataprepare);
    if(cali)
    {
        cali->compute();
        cali->evaluate();
    }
}

void AttributeEmbed :: calibrateNoEva()
{
    Calibrate *cali = CalibrateFac::get()->initalCali(config, filePathManager, dataprepare);
    if(cali){
        cali->compute();
    }
}

void AttributeEmbed :: recognition()
{
    Recog *reco = new Recog(config, filePathManager, dataprepare);
    reco->evaluate();
}

std::vector<std::string> AttributeEmbed :: recog4Annotation(cv::Mat img)
{
    if(reco4anno == nullptr){
        reco4anno = new Recog(config, filePathManager);
    }

    std::vector<std::string> candi = reco4anno->getCandidates(img);
    return candi;
    /*for(auto can: candi)
    {
        std::cout<<can<<std::endl;
    }*/
}

void AttributeEmbed :: evaOnline()
{
    //learning attribute embed by svm
    AttributesLearning *al = new AttributesLearning(config, filePathManager, dataprepare, st);
    al->evalComSubWithOnline();
}

bool AttributeEmbed :: isRecoReady()
{
    bool ready = false;
    if(filePathManager->fileExist("lexicon")&&
            filePathManager->fileExist("csrModel")&&
            filePathManager->fileExist("attModel")&&
            filePathManager->fileExist("pca")&&
            filePathManager->fileExist("gmm")&&
            filePathManager->fileExist("classes")&&
            filePathManager->dirExist("phoc")){
        ready = true;
    }
    return ready;
}


}
