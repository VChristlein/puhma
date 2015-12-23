#include "attributes_learning.h"
#include "cca.h"
#include "eva.h"
#include "attrispace.h"

#include <numeric>
#include <QDebug>
#include <QFile>
#include <QDir>
#include <QString>
#include <QStringList>
#include <QRegExp>

namespace puhma {
AttributesLearning :: AttributesLearning(const AttriConfig &c, const FileHandler *fpm, const DataPrepare * dp, Statistic *const st)
    : config(c),filePathManager(fpm),st(st)
{
    dataTrain = dp->getDivision("train");
    dataTest = dp->getDivision("test");
    dataValidation = dp->getDivision("vali");
    dataOnline = dp->getDivision("online");
    fv = new FisherV(config, filePathManager, st);
    tp = new TransPhoc(config, filePathManager);
    //int dim = (tp->getSize()).width;
    //int dimFV = (fv->getSize()).width;
    //attModel = new AttriModel(dim, dimFV, filePathManager);
}

AttributesLearning :: ~AttributesLearning(){}

void AttributesLearning :: evalComSubWithOnline()
{
    qDebug()<<"Evaluate with Online Learning Method...";
    int threReComputeCS = 3;
    bool didReComputeCS = false; //flag to recalibrate
    double npRate = 2.0; //negative / postive ratio
    int dim = tp->getSize().width;
    //data for training
    int dimFV = fv->getSize().width;
    int numOfData = dataTrain.size()+dataValidation.size();
    std::vector<double> rateAttri(dim); //rate for validation
    std::vector<std::string> swords;
    std::vector<int> idxUsed;
    std::vector<AttriModel1D *> attriModelsToUp(dim);
    //check if csr is learned
    if(!(filePathManager->fileExist("csrModel")))
        throw std::runtime_error("Attri_learn :: eval : csrModel do not exist");
    else
        embedding.restore(filePathManager);

    //std::vector<double> modelrate(dim);
    
    qDebug()<<"Preparing data...";
    qDebug()<<"     Loading attrimodels...";
    for(int i=0; i<attriModelsToUp.size(); ++i){
        attriModelsToUp[i] = new AttriModel1D(i, filePathManager);
        //modelrate[i] = attriModelsToUp[i]->rateVali;
    }
    AttriModel *attModel = new AttriModel(dim, dimFV, filePathManager);
    attModel->reload();
    qDebug()<<"     Done.";
    /*std::string fnt = (QFileInfo(QDir(QString::fromStdString(config.outputdir))
            , QString::fromStdString("eva_rate.txt")).filePath()).toStdString();
    Util::dumpVector(fnt,modelrate);*/ //now shall use statistic instead

    qDebug()<<"     Pareparing data for training purpose...";
    cv::Mat features = cv::Mat::zeros(numOfData, dimFV, CV_32F);
    cv::Mat phocs = cv::Mat::zeros(numOfData, dim, CV_8U);
    //load the train and validation feature/phoc 
    for(int i=0;i < dataTrain.size(); ++i){
        std::string fn = dataTrain[i][0];
        fv->fetchOneFeature(fn).copyTo(features.row(i));
        tp->fetchOnePhocFromFile(fn).copyTo(phocs.row(i));
    }
    for(int i=0;i < dataValidation.size(); ++i){
        std::string fn = dataValidation[i][0];
        fv->fetchOneFeature(fn).copyTo(features.row(dataTrain.size()+i));
        tp->fetchOnePhocFromFile(fn).copyTo(phocs.row(dataTrain.size()+i));
    }
    cv::Mat1b msk = cv::Mat1b::zeros(phocs.size());
    phocs = (phocs>msk);
    phocs = phocs/255;
    msk.release();
    //extract idx of negativ / postive for each dimension
    std::vector<std::vector<int> > positiveidxPerDim(dim);
    std::vector<std::vector<int> > negativeidxPerDim(dim);
    for(int x=0; x<phocs.cols; ++x){
        for(int y=0; y<phocs.rows; ++y){
            if(phocs.at<uchar>(y,x)>0){
                positiveidxPerDim[x].push_back(y);
            }else{
                negativeidxPerDim[x].push_back(y);
            }
        }
    }
    qDebug()<<"     Done";

    //data for validation
    qDebug()<<"     Pareparing data for valuation purpose...";
    cv::Mat featureOn_val = cv::Mat::zeros(dataOnline.size(),dimFV,CV_32F);
    cv::Mat phocOn_val = cv::Mat::zeros(dataOnline.size(),dim,CV_8U);
    for(int i=0; i< dataOnline.size(); ++i){
        std::string fn = dataOnline[i][0];
        fv->fetchOneFeature(fn).copyTo(featureOn_val.row(i));
        tp->fetchOnePhocFromFile(fn).copyTo(phocOn_val.row(i));
    }
    msk = cv::Mat1b::zeros(phocOn_val.size());
    phocOn_val = (phocOn_val>msk);
    phocOn_val = phocOn_val/255;
    msk.release();
    //extract idx of postive for each dimension in online set
    std::vector<std::vector<int> > positiveidxPerDimVal(dim);
    for(int x=0; x<phocOn_val.cols; ++x){
        for(int y=0; y<phocOn_val.rows; ++y){
            if(phocOn_val.at<uchar>(y,x)>0){
                positiveidxPerDimVal[x].push_back(y);
            }
        }
    }
    //do evaluation once to generate rate.
    featureOn_val.convertTo(featureOn_val, CV_64F);
    for(int i=0; i<attriModelsToUp.size(); ++i){
        cv::Mat1d rep_val = (attriModelsToUp[i]->W)*featureOn_val.t() + (attriModelsToUp[i]->B);
        cv::Mat phoc_val1d = phocOn_val.col(i).clone();
        phoc_val1d.convertTo(phoc_val1d, CV_64F);
        rateAttri[i]=AttriSpace::modelMap(rep_val, phoc_val1d.t());
        rep_val.release();
        phoc_val1d.release();
        //qDebug()<<rateAttri[i];
    }
    qDebug()<<"     Done.";

    qDebug()<<"     Loading data for evaluation and online learning purpose...";
    //trainbuffer
    std::vector<std::list<int> > positiveInstanceList(dim);
    std::vector<std::list<int> > negativeInstanceList(dim);
    //data for evaluation
    cv::Mat featureTe = cv::Mat::zeros(dataTest.size(),dimFV,CV_32F);
    cv::Mat phocTe = cv::Mat::zeros(dataTest.size(),dim,CV_8U);
    cv::Mat wordClsTe = cv::Mat::zeros(dataTest.size(), 1, CV_32S);
    std::vector<std::string> labelTe(dataTest.size());
    for(int i=0; i< dataTest.size(); ++i){
        fv->fetchOneFeature(dataTest[i][0]).copyTo(featureTe.row(i));
        tp->fetchOnePhocFromFile(dataTest[i][0]).copyTo(phocTe.row(i));
        wordClsTe.at<int>(i,0) = std::stoi(dataTest[i][3]);
        labelTe[i] = dataTest[i][1];
    }
    msk = cv::Mat1b::zeros(phocTe.size());
    phocTe = (phocTe>msk);
    phocTe = phocTe/255;
    msk.release();
    cv::Mat1d attriRepTr = AttriSpace::getRep("train", filePathManager);
    cv::Mat1d attriRepTe = AttriSpace::getRep("test", filePathManager);
    cv::Mat1d repTeClo = attriRepTe.clone();
    Util::normalizePerRow(repTeClo);
    Util::meanCenterPerCol(repTeClo, embedding.mAtt);
    cv::Mat1d repTeEval_CCA = embedding.Watt.rowRange(0,config.dimCCA)*(repTeClo.t());
    repTeEval_CCA = repTeEval_CCA.t();
    Util::normalizePerRow(repTeEval_CCA);
    cv::Mat phocTeEval;
    phocTe.convertTo(phocTeEval, CV_64F);
    phocTeEval = phocTeEval *2.0 -1.0;
    Util::normalizePerRow(phocTeEval);
    Util::meanCenterPerCol(phocTeEval, embedding.mPhoc);
    cv::Mat1d phocTeEval_CCA = embedding.Wphoc.rowRange(0,config.dimCCA)*(phocTeEval.t());
    phocTeEval_CCA = phocTeEval_CCA.t();
    Util::normalizePerRow(phocTeEval_CCA);

    //generate a random index of dataTest.
    std::vector<int> idxTest(dataTest.size());
    std::iota(idxTest.begin(),idxTest.end(),0);
    Util::generateRan(idxTest);
    std::vector<double> mAPqbe;
    std::vector<double> p1qbe;
    std::vector<double> mAPqbs;
    std::vector<double> p1qbs;
    std::vector<double> mAPqbeAcc;
    std::vector<double> p1qbeAcc;
    std::vector<double> mAPqbsAcc;
    std::vector<double> p1qbsAcc;
    std::vector<double> mAPqbeAcc_old;
    std::vector<double> p1qbeAcc_old;
    std::vector<double> mAPqbsAcc_old;
    std::vector<double> p1qbsAcc_old;
    //preparing stop words
    if(config.sw != ""){
        QFile file(QString::fromStdString(config.sw));
        if(!file.open(QIODevice::ReadOnly | QIODevice::Text)) {  
            throw std::runtime_error("Evaluate:: computemAP : can not open file of stopwords.");
        }
        QTextStream in(&file);
        QRegExp rxs("[,]");
        while(!in.atEnd()){
            QString line = in.readLine();
            QStringList words = line.split(rxs, QString::SkipEmptyParts);
            for(QString word: words){
                swords.push_back(word.toStdString());
            }
        }
    }
    qDebug()<<"     Done.";
    qDebug()<<"Done.";
    
    //generate evaluate result of non-online for evaluation purpose. 
    evalComSubWithOffline(idxTest,
            repTeEval_CCA, phocTeEval_CCA, wordClsTe, labelTe,
            swords,
            mAPqbeAcc_old, p1qbeAcc_old, mAPqbsAcc_old, p1qbsAcc_old);

    qDebug()<<"Start Evaluating online learning method...";
    int count = 0;
    int upTime = 0;
    cv::Mat wordClsTe_mark = wordClsTe.clone();
    for(int idx: idxTest){
        //at this moment, representations of CS space are already updated.
        cv::Mat1d oneRepEval_CCA = repTeEval_CCA.row(idx).clone();
        cv::Mat1d onePhocEval_CCA = phocTeEval_CCA.row(idx).clone();

        //do qbe and qbs use this data
        cv::Mat1d mAP, p1;
        //QBE
        int performed;
        bool needTraining= false;
        performed = Evaluate :: computemAP2(config, idx, oneRepEval_CCA, repTeEval_CCA, wordClsTe_mark, labelTe, swords, mAP, p1);
        double qbe_val_map;
        double qbe_val_p1;
        if(performed > 0){
            qbe_val_map = cv::mean(mAP)[0];
            qbe_val_p1 = cv::mean(p1)[0];
            mAPqbe.push_back(qbe_val_map);
            p1qbe.push_back(qbe_val_p1);
            if(qbe_val_p1<1)
                needTraining = true;
            //qDebug()<<"qbe --   test: (map:"<<100*qbe_val_map<<". p@1:"<<100*qbe_val_p1<<")";
        }/*else{
            //qDebug()<<"qbe --   n/a";
        }*/

        //QBS   ! since qbs use label as query content. No influence with online.
        performed = Evaluate :: computemAP2(config, idx, onePhocEval_CCA, repTeEval_CCA, wordClsTe_mark, labelTe, swords, mAP, p1, true);
        double qbs_val_map;
        double qbs_val_p1;
        if(performed > 0){
            qbs_val_map = cv::mean(mAP)[0];
            qbs_val_p1 = cv::mean(p1)[0];
            mAPqbs.push_back(qbs_val_map);
            p1qbs.push_back(qbs_val_p1);
            //if(qbs_val_p1<1)
            //    needTraining = true;
            //qDebug()<<"qbs --   test: (map:"<<100*qbs_val_map<<". p@1:"<<100*qbs_val_p1<<")";
        }/*else{
            //qDebug()<<"qbs --   n/a";
        }*/

        //mark used idx as used.
        oneRepEval_CCA = oneRepEval_CCA*0;
        oneRepEval_CCA.copyTo(repTeEval_CCA.row(idx));
        wordClsTe_mark.at<int>(idx,0) = -1;
        idxUsed.push_back(idx);

        oneRepEval_CCA.release();
        onePhocEval_CCA.release();

        //if p1 of qbe or qbs is not 1
        //  start learning
        if(needTraining){
            //qDebug()<<"Training request received..";
            //push positive and negative samples into buffer
            for(int x=0; x<phocTe.cols; x++){
                if(phocTe.at<uchar>(idx,x)>0){
                    positiveInstanceList[x].push_back(idx);
                    if(positiveInstanceList[x].size()>config.bufferSize){
                        positiveInstanceList[x].pop_front();
                    }
                    //training data prepare.
                    cv::Mat featureOnline;
                    cv::Mat phocOnline;
                    if(positiveidxPerDimVal[x].size()<1){
                        break;
                    }

                    for(int posidx: positiveInstanceList[x]){
                        featureOnline.push_back(featureTe.row(posidx).clone());
                    }
                    if(positiveInstanceList[x].size()<config.bufferSize){
                        int sizeFromTrainSet=config.bufferSize-positiveInstanceList[x].size();
                        std::vector<int> idxTrTemp = positiveidxPerDim[x];
                        Util::generateRan(idxTrTemp);
                        if(sizeFromTrainSet<idxTrTemp.size()){
                            idxTrTemp.resize(sizeFromTrainSet);
                        }
                        for(int posidx: idxTrTemp){
                            featureOnline.push_back(features.row(posidx));
                        }
                    }
                    int posSize=featureOnline.rows;
                    for(int negidx: negativeInstanceList[x]){
                        featureOnline.push_back(featureTe.row(negidx).clone());
                    }
                    if(negativeInstanceList[x].size()<posSize){
                        int sizeFromTrainSet=posSize-negativeInstanceList[x].size();
                        std::vector<int> idxTrTemp = negativeidxPerDim[x];
                        Util::generateRan(idxTrTemp);
                        if(sizeFromTrainSet<idxTrTemp.size()){
                            idxTrTemp.resize(sizeFromTrainSet);
                        }
                        for(int negidx: idxTrTemp){
                            featureOnline.push_back(features.row(negidx));
                        }
                    }

                    phocOnline=cv::Mat1b::zeros(featureOnline.rows,1);
                    cv::Mat1b posTemp = phocOnline.rowRange(0,posSize)+1;
                    posTemp.copyTo(phocOnline.rowRange(0,posSize));

                    cv::Mat phocOn_val1d = phocOn_val.col(x).clone();

                    bool doUp= false;
                    doUp = learnAttriWarm(attriModelsToUp[x],
                            featureOnline,
                            phocOnline,
                            featureOn_val,
                            phocOn_val1d,
                            rateAttri[x]);
                    featureOnline.release();
                    phocOnline.release();

                    if(doUp){
                        qDebug()<<"One update occurs...";
                        //update attriModel
                        attModel->update(*(attriModelsToUp[x]));
                        rateAttri[x]=(attriModelsToUp[x])->rateVali;
                        //update repTe
                        cv::Mat featureTeT;
                        featureTe.convertTo(featureTeT, CV_64F);
                        cv::Mat1d newRep = ((attriModelsToUp[x])->W) * (featureTeT.t()) + ((attriModelsToUp[x])->B);
                        featureTeT.release();
                        newRep = newRep.t();
                        newRep.copyTo(attriRepTe.col(x));
                        newRep.release();

                        cv::Mat featuresT;
                        features.convertTo(featuresT, CV_64F);
                        newRep = ((attriModelsToUp[x])->W) * (featuresT.t()) + ((attriModelsToUp[x])->B);
                        featuresT.release();
                        newRep = newRep.t();
                        cv::Mat1d oldRep = attriRepTr.col(x);
                        /*double itn = static_cast<double>((attriModelsToUp[x])->numIterat);
                        if(itn >1){
                            newRep = oldRep * (itn-1)/itn + newRep / itn;
                        }*/
                        newRep.copyTo(attriRepTr.col(x));
                        newRep.release();
                        oldRep.release();

                        repTeClo = attriRepTe.clone();
                        Util::normalizePerRow(repTeClo);
                        Util::meanCenterPerCol(repTeClo, embedding.mAtt);
                        repTeEval_CCA = embedding.Watt.rowRange(0,config.dimCCA)*(repTeClo.t());
                        repTeEval_CCA = repTeEval_CCA.t();
                        Util::normalizePerRow(repTeEval_CCA);

                        for(int usedIdx : idxUsed){
                            cv::Mat oneRepTe = repTeEval_CCA.row(usedIdx);
                            oneRepTe = oneRepTe*0;
                            oneRepTe.copyTo(repTeEval_CCA.row(usedIdx));
                        }
                        upTime ++;
                        if(upTime%threReComputeCS==0)
                            didReComputeCS = true;
                        qDebug()<<"Done.";
                    }
                }else{
                    negativeInstanceList[x].push_back(idx);
                    if(negativeInstanceList[x].size()>config.bufferSize*(int)npRate){
                        negativeInstanceList[x].pop_front();
                    }
                }
            }
        }

        if(didReComputeCS){
            bool updated = updateCS(attriRepTr,attriRepTe,phocs,phocTe,wordClsTe,labelTe);

            if(updated){
                phocTe.convertTo(phocTeEval, CV_64F);
                phocTeEval = phocTeEval *2.0 -1.0;
                Util::normalizePerRow(phocTeEval);
                Util::meanCenterPerCol(phocTeEval, embedding.mPhoc);
                phocTeEval_CCA = embedding.Wphoc.rowRange(0,config.dimCCA)*(phocTeEval.t());
                phocTeEval_CCA = phocTeEval_CCA.t();
                Util::normalizePerRow(phocTeEval_CCA);

                repTeClo = attriRepTe.clone();
                Util::normalizePerRow(repTeClo);
                Util::meanCenterPerCol(repTeClo, embedding.mAtt);
                repTeEval_CCA = embedding.Watt.rowRange(0,config.dimCCA)*(repTeClo.t());
                repTeEval_CCA = repTeEval_CCA.t();
                Util::normalizePerRow(repTeEval_CCA);

                for(int usedIdx : idxUsed){
                    cv::Mat oneRepTe = repTeEval_CCA.row(usedIdx);
                    oneRepTe = oneRepTe*0;
                    oneRepTe.copyTo(repTeEval_CCA.row(usedIdx));
                }
            }

            didReComputeCS = false;
        }

        //

        ++count;
        //if(count%config.bufferSize==0){
        if(count%20==0){
            double accMAPQBE = std::accumulate(mAPqbe.begin(),mAPqbe.end(),0.0);
            accMAPQBE /= mAPqbe.size();
            mAPqbeAcc.push_back(accMAPQBE);
            double accP1QBE = std::accumulate(p1qbe.begin(),p1qbe.end(),0.0);
            accP1QBE /= p1qbe.size();
            p1qbeAcc.push_back(accP1QBE);

            double accMAPQBS = std::accumulate(mAPqbs.begin(),mAPqbs.end(),0.0);
            accMAPQBS /= mAPqbs.size();
            mAPqbsAcc.push_back(accMAPQBS);
            double accP1QBS = std::accumulate(p1qbs.begin(),p1qbs.end(),0.0);
            accP1QBS /= p1qbs.size();
            p1qbsAcc.push_back(accP1QBS);

            //output one mAP
            int currentqbe = mAPqbeAcc.size()-1; 
            int currentqbs = mAPqbsAcc.size()-1;
            qDebug()<<"partial QBE -- test: (map:"<<100*accMAPQBE<<". p@1:"<<100*accP1QBE<<") -- old: (map:"<<100*mAPqbeAcc_old[currentqbe]<<". p@1:"<<100*p1qbeAcc_old[currentqbe]<<")";
            qDebug()<<"partial QBS -- test: (map:"<<100*accMAPQBS<<". p@1:"<<100*accP1QBS<<") -- old: (map:"<<100*mAPqbsAcc_old[currentqbs]<<". p@1:"<<100*p1qbsAcc_old[currentqbs]<<")";
            std::string fn = (QFileInfo(QDir(QString::fromStdString(config.outputdir))
                        , QString::fromStdString("qbe_map_acc.txt")).filePath()).toStdString();
            Util::dumpVector(fn,mAPqbeAcc);
            fn = (QFileInfo(QDir(QString::fromStdString(config.outputdir))
                        , QString::fromStdString("qbe_p1_acc.txt")).filePath()).toStdString();
            Util::dumpVector(fn,p1qbeAcc);
            fn = (QFileInfo(QDir(QString::fromStdString(config.outputdir))
                        , QString::fromStdString("qbs_map_acc.txt")).filePath()).toStdString();
            Util::dumpVector(fn,mAPqbsAcc);
            fn = (QFileInfo(QDir(QString::fromStdString(config.outputdir))
                        , QString::fromStdString("qbs_p1_acc.txt")).filePath()).toStdString();
            Util::dumpVector(fn,p1qbsAcc);
        }
    }
    attModel->dumpOnline();
    dumpAttRepOnline("Test", attriRepTe);
    std::string fn = (QFileInfo(QDir(QString::fromStdString(config.outputdir))
            , QString::fromStdString("qbe_map_acc.txt")).filePath()).toStdString();
    Util::dumpVector(fn,mAPqbeAcc);
    fn = (QFileInfo(QDir(QString::fromStdString(config.outputdir))
            , QString::fromStdString("qbe_p1_acc.txt")).filePath()).toStdString();
    Util::dumpVector(fn,p1qbeAcc);
    fn = (QFileInfo(QDir(QString::fromStdString(config.outputdir))
            , QString::fromStdString("qbs_map_acc.txt")).filePath()).toStdString();
    Util::dumpVector(fn,mAPqbsAcc);
    fn = (QFileInfo(QDir(QString::fromStdString(config.outputdir))
            , QString::fromStdString("qbs_p1_acc.txt")).filePath()).toStdString();
    Util::dumpVector(fn,p1qbsAcc);
    //
    qDebug()<<"Done.";
}

void AttributesLearning :: evalComSubWithOffline(
        const std::vector<int> &idxList,
        const cv::Mat1d &rep,
        const cv::Mat1d &phoc,
        const cv::Mat &wordCls,
        const std::vector<std::string> &labelTe,
        const std::vector<std::string> &swords,
        std::vector<double> &mAPqbeAcc_old,
        std::vector<double> &p1qbeAcc_old,
        std::vector<double> &mAPqbsAcc_old,
        std::vector<double> &p1qbsAcc_old)
{
    qDebug()<<"Generating Evaluating results of non-online...";
    int count = 0;
    std::vector<double> mAPqbe_old;
    std::vector<double> p1qbe_old;
    std::vector<double> mAPqbs_old;
    std::vector<double> p1qbs_old;

    cv::Mat1d repTeEval_CCA = rep.clone();
    cv::Mat wordClsTe_old = wordCls.clone();
    for (int idx:idxList){
        cv::Mat1d mAP, p1;
        int performed;
        cv::Mat1d oneRepEval_CCA_old = repTeEval_CCA.row(idx).clone();
        performed = Evaluate :: computemAP2(config, idx, oneRepEval_CCA_old, repTeEval_CCA, wordClsTe_old, labelTe, swords, mAP, p1);
        double qbe_val_map_old;
        double qbe_val_p1_old;
        if(performed > 0){
            qbe_val_map_old = cv::mean(mAP)[0];
            qbe_val_p1_old = cv::mean(p1)[0];
            mAPqbe_old.push_back(qbe_val_map_old);
            p1qbe_old.push_back(qbe_val_p1_old);
        }
        cv::Mat1d onePhocEval_CCA_old = phoc.row(idx).clone();
        performed = Evaluate :: computemAP2(config, idx, onePhocEval_CCA_old, repTeEval_CCA, wordClsTe_old, labelTe, swords, mAP, p1, true);
        double qbs_val_map_old;
        double qbs_val_p1_old;
        if(performed > 0){
            qbs_val_map_old = cv::mean(mAP)[0];
            qbs_val_p1_old = cv::mean(p1)[0];
            mAPqbs_old.push_back(qbs_val_map_old);
            p1qbs_old.push_back(qbs_val_p1_old);
        }
        oneRepEval_CCA_old = oneRepEval_CCA_old*0;
        oneRepEval_CCA_old.copyTo(repTeEval_CCA.row(idx));
        wordClsTe_old.at<int>(idx,0) = -1;

        oneRepEval_CCA_old.release();
        onePhocEval_CCA_old.release();

        ++count;
        //if(count%config.bufferSize==0){
        if(count%20==0){
            double accMAPQBE_old = std::accumulate(mAPqbe_old.begin(),mAPqbe_old.end(),0.0);
            accMAPQBE_old /= mAPqbe_old.size();
            mAPqbeAcc_old.push_back(accMAPQBE_old);
            double accP1QBE_old = std::accumulate(p1qbe_old.begin(),p1qbe_old.end(),0.0);
            accP1QBE_old /= p1qbe_old.size();
            p1qbeAcc_old.push_back(accP1QBE_old);

            double accMAPQBS_old = std::accumulate(mAPqbs_old.begin(),mAPqbs_old.end(),0.0);
            accMAPQBS_old /= mAPqbs_old.size();
            mAPqbsAcc_old.push_back(accMAPQBS_old);
            double accP1QBS_old = std::accumulate(p1qbs_old.begin(),p1qbs_old.end(),0.0);
            accP1QBS_old /= p1qbs_old.size();
            p1qbsAcc_old.push_back(accP1QBS_old);
            std::string fn = (QFileInfo(QDir(QString::fromStdString(config.outputdir))
                        , QString::fromStdString("qbe_map_acc_old.txt")).filePath()).toStdString();
            Util::dumpVector(fn,mAPqbeAcc_old);
            fn = (QFileInfo(QDir(QString::fromStdString(config.outputdir))
                        , QString::fromStdString("qbe_p1_acc_old.txt")).filePath()).toStdString();
            Util::dumpVector(fn,p1qbeAcc_old);
            fn = (QFileInfo(QDir(QString::fromStdString(config.outputdir))
                        , QString::fromStdString("qbs_map_acc_old.txt")).filePath()).toStdString();
            Util::dumpVector(fn,mAPqbsAcc_old);
            fn = (QFileInfo(QDir(QString::fromStdString(config.outputdir))
                        , QString::fromStdString("qbs_p1_acc_old.txt")).filePath()).toStdString();
            Util::dumpVector(fn,p1qbsAcc_old);
        }

    }
    std::string fn = (QFileInfo(QDir(QString::fromStdString(config.outputdir))
            , QString::fromStdString("qbe_map_acc_old.txt")).filePath()).toStdString();
    Util::dumpVector(fn,mAPqbeAcc_old);
    fn = (QFileInfo(QDir(QString::fromStdString(config.outputdir))
            , QString::fromStdString("qbe_p1_acc_old.txt")).filePath()).toStdString();
    Util::dumpVector(fn,p1qbeAcc_old);
    fn = (QFileInfo(QDir(QString::fromStdString(config.outputdir))
            , QString::fromStdString("qbs_map_acc_old.txt")).filePath()).toStdString();
    Util::dumpVector(fn,mAPqbsAcc_old);
    fn = (QFileInfo(QDir(QString::fromStdString(config.outputdir))
            , QString::fromStdString("qbs_p1_acc_old.txt")).filePath()).toStdString();
    Util::dumpVector(fn,p1qbsAcc_old);
    qDebug()<<"Done.";
}

void AttributesLearning :: dumpAttRepOnline(const std::string &type, const cv::Mat1d &rep) const
{
    std::string fn;
    const cv::Mat1d *attRep;
    if(type == "train"){
        fn = filePathManager->attRepresTrainOnline;
    }else if(type == "vali"){
        fn = filePathManager->attRepresValidationOnline;
    }else if(type == "test"){
        fn = filePathManager->attRepresTestOnline;
    }else{
        throw std::runtime_error("AttriLearn :: dumpAttRep : type un-acceptable.");
    }
    cv::FileStorage fs(fn, cv::FileStorage::WRITE);
    fs << "attRep" << rep;
    fs.release();
}

/*void AttributesLearning :: reloadAttRep(std::string type){
    std::string fn;
    cv::Mat1d *attRep;
    if(type == "train"){
        fn = filePathManager->attRepresTrain;
        attRep = &attriRepTr;
    }else if(type == "vali"){
        fn = filePathManager->attRepresValidation;
        attRep = &attriRepVa;
    }else if(type == "test"){
        fn = filePathManager->attRepresTest;
        attRep = &attriRepTe;
    }else{
        throw std::runtime_error("AttriLearn :: reloadAttRep : type un-acceptable.");
    }
    cv::FileStorage fs(fn, cv::FileStorage::READ);
    if (!fs.isOpened())
    {
        throw std::runtime_error("AttriLearn :: reloadAttRep : can not open file");
    }
    fs["attRep"] >> (*attRep);
    fs.release();
}*/


/*void AttributesLearning :: dumpIdx(const int numAttri,
        const std::vector<int> idxP,
        const std::vector<int> idxN){
    std::string dirname = filePathManager->attriModelDir;
    std::string posfilename = "idxpos_"+std::to_string(numAttri)+".txt";
    std::string negfilename = "idxneg_"+std::to_string(numAttri)+".txt";
    QString posfn = QString::fromStdString(posfilename);
    QString negfn = QString::fromStdString(negfilename);
    QFileInfo filepos = QFileInfo(QDir(QString::fromStdString(dirname)),posfn);
    QFileInfo fileneg = QFileInfo(QDir(QString::fromStdString(dirname)),negfn);
    std::string posfp=filepos.filePath().toStdString();
    std::string negfp=fileneg.filePath().toStdString();

    std::ofstream output_file;
    output_file.open(posfp);
    std::copy(idxP.begin(), idxP.end(), std::ostream_iterator<int>(output_file, ","));
    output_file.close();
    output_file.open(negfp);
    std::copy(idxN.begin(), idxN.end(), std::ostream_iterator<int>(output_file, ","));
    output_file.close();
}*/

bool AttributesLearning :: learnAttriWarm(AttriModel1D* am1d,
            cv::Mat &featuresTr,
            cv::Mat &phocTr,
            cv::Mat &featuresVal,
            cv::Mat &phocVal,
            const double rate)
{
    bool re = false;
    int numOfTr = phocTr.rows;
    int numOfData = phocVal.rows;
    int dimOfFeat = featuresTr.cols;
    int dim = phocTr.cols;

    //prepare the model to update.
    AttriModel1D am(-1, dimOfFeat, numOfData, filePathManager);
    am.W = (am1d->W).clone();
    am.B = am1d->B;
    am.numIterat = am1d->numIterat;

    std::vector<int> randomIdx(numOfTr);
    std::iota(randomIdx.begin(),randomIdx.end(),0);
    cv::Mat tmp = cv::Mat::zeros(featuresTr.size(),featuresTr.type());
    cv::Mat tmpp = cv::Mat::zeros(phocTr.size(),phocTr.type());
    for(int i=0; i < numOfTr; ++i){
        featuresTr.row(randomIdx[i]).copyTo(tmp.row(i));
        phocTr.row(randomIdx[i]).copyTo(tmpp.row(i));
    }

    tmp.convertTo(tmp, CV_64F);
    tmpp.convertTo(tmpp, CV_64F);
    tmpp = tmpp*2.0-1.0;

    sgdTrainningWarm(tmp, tmpp, am.W, am.B, am.numIterat);

    tmp.release();
    tmpp.release();

    featuresVal.convertTo(tmp, CV_64F);
    phocVal.convertTo(tmpp, CV_64F);

    cv::Mat1d score = (am.W)*(tmp.t())+am.B;

    //std::cout<<score<<std::endl;

    double cmap = AttriSpace::modelMap(score, tmpp.t());

    tmp.release();
    tmpp.release();
                    
    //qDebug()<<"one attribute with old rate:"<<rate*100<<"% after training: ("<<cmap*100<<"% )";
    if(cmap > rate){
        (am1d->W) = am.W.clone();
        am1d->B = am.B;
        int oldIterat = am1d->numIterat;
        am1d->numIterat++;
        am1d->rateVali = cmap;
        re = true;
        am1d->dumpOnline();
    }

    return re;
}

void AttributesLearning :: sgdTrainningWarm(
            const cv::Mat &data,
            const cv::Mat &label,
            cv::Mat1d &model,
            double &bias,
            int &numIters,
            double epsilon,
            double bias_multiplier
            )
{
    int numdata = data.rows;
    std::vector<int> idxPermutation(numdata);
    std::iota(idxPermutation.begin(),idxPermutation.end(),0);
    cv::Mat1d dataL = data.clone();
    cv::Mat1d labelL = label.clone();
    cv::Mat1d previousScores;
    cv::Mat1d scores;
    double lambda = config.onlineLambda;
    int t=0;
    int maxNumIterations = static_cast<int>(10/lambda);
    double inner, gradient, rate, biasRate, p=1.0 ;
    double factor = 1.0 ;
    double biasFactor = 1.0 ; /* to allow slower bias learning rate */
    double biasLearningRate = 0.01;
    //int t0 = 2 ;
    if(numIters>0){
        previousScores = model*(dataL.t())+bias;
        scores = previousScores.clone();
        //t=100;
        //t=static_cast<int>(5.0/lambda);
        t=10*(dataTrain.size()+dataValidation.size());
        rate = 1.0/ (lambda * (t));
        biasRate = rate * biasLearningRate;
        //factor *= (1.0-lambda*rate);
        //biasFactor *= (1.0-lambda*biasRate);
    }else{
        previousScores = cv::Mat1d::zeros(1, numdata);
        previousScores += (- std::numeric_limits<double>::infinity());
        scores = cv::Mat1d::zeros(previousScores.size());
    }

    for (; true; ++t){
        if (t % numdata == 0) {
            /* once a new epoch is reached (all data have been visited),
               change permutation */
            Util::generateRan(idxPermutation);
        }

        /* pick a sample and compute update */
        int idx = idxPermutation[t % numdata];
        //cv::Mat1d score1sample = model*(dataL.row(idx).t())*factor;
        inner = model.dot(dataL.row(idx))*factor;
        //score1sample.release();
        inner += biasFactor * bias_multiplier * bias;
        double gt = labelL(idx,0);
        gradient = p * ((inner * gt<1.0)?-gt:0.0);
        previousScores(0,idx) = scores(0,idx);
        scores(0,idx) = inner;

        rate = 1.0/ (lambda * (t));
        biasRate = rate * biasLearningRate;
        factor *= (1.0-lambda*rate);
        biasFactor *= (1.0-lambda*biasRate);

        if (gradient != 0) {
            model = model + (-gradient*rate/factor)*(dataL.row(idx));
            bias = bias + bias_multiplier*(-gradient*biasRate/biasFactor);
        }

        if ((t + 1) % numdata == 0 || t + 1 == maxNumIterations) {

            /* realize factor before computing statistics or completing training */
            model *= factor ;
            bias *= biasFactor;
            factor = 1.0/*-lambda*(1.0/ (1.0+lambda * t0))*/;
            biasFactor = 1.0/*-lambda * (1.0/ (1.0+lambda * t0)) * biasLearningRate*/ ;

            //_vl_svm_update_statistics (self) ;

            cv::Mat1d deltaScores = scores - previousScores;
            double scoresVariation =deltaScores.dot(deltaScores);

            scoresVariation = std::sqrt(scoresVariation) / numdata ;

            //self->statistics.elapsedTime = vl_get_cpu_time() - startTime ;
            //self->statistics.iteration = t ;
            //self->statistics.epoch = t / self->numData ;

            //satus
            //  1 : learning
            //  2 : converged
            //  3 : iterationsReached
            int status = 1 ;
            if (scoresVariation < epsilon) {
                status = 2 ;
            }
            else if (t + 1 == maxNumIterations) {
                status = 3 ;
            }

            if (status != 1) {
                bias = bias*bias_multiplier;
                break ;
            }
        }
    } /* next iteration */
}

bool AttributesLearning :: updateCS(
        const cv::Mat1d &attriRepTr,
        const cv::Mat1d &attriRepTe,
        const cv::Mat &phocs,
        const cv::Mat &phocsTe,
        const cv::Mat &wordClsTe,
        const std::vector<std::string> labelTe){
    qDebug()<<"Update the Common Subspace...";
    bool re=false;

    std::vector<int> trPermutation(attriRepTr.rows);
    std::iota(trPermutation.begin(), trPermutation.end(), 0);
    Util::generateRan(trPermutation);

    cv::Mat repFull = cv::Mat::zeros(attriRepTr.size(),attriRepTr.type());
    cv::Mat phocFull = cv::Mat::zeros(phocs.size(),phocs.type());
    cv::Mat wordClsFull = cv::Mat::zeros(trPermutation.size(),1,CV_32S);
    std::vector<std::string> labelFull(trPermutation.size());

    for(int i=0;i < trPermutation.size(); ++i){
        int idx = trPermutation[i];  //in attriRepTr, data from trainset comes first.
        attriRepTr.row(idx).copyTo(repFull.row(i));
        phocs.row(idx).copyTo(phocFull.row(i));
        if(idx < dataTrain.size()){
            wordClsFull.at<int>(i,0) = std::stoi(dataTrain[idx][3]);
            labelFull[i] = dataTrain[idx][1];
            //TODO: test here.
        }else{
            idx = idx-dataTrain.size();
            wordClsFull.at<int>(i,0) = std::stoi(dataValidation[idx][3]);
            labelFull[i] = dataValidation[idx][1];
        }
    }
    int nVal = static_cast<int>(std::floor(trPermutation.size()*0.3));
    cv::Mat repTr = repFull.rowRange(nVal, repFull.rows).clone();
    cv::Mat repVal = repFull.rowRange(0, nVal).clone();//64F
    cv::Mat phocTr = phocFull.rowRange(nVal, phocFull.rows).clone();
    cv::Mat phocVal = phocFull.rowRange(0, nVal).clone();//8U
    cv::Mat wordClsTr = wordClsFull.rowRange(nVal, wordClsFull.rows).clone();
    cv::Mat wordClsVal = wordClsFull.rowRange(0, nVal).clone();//32S
    std::vector<std::string> labelTr(labelFull.begin()+nVal, labelFull.end());
    std::vector<std::string> labelVal(labelFull.begin(), labelFull.begin()+nVal);

    wordClsFull.release();

    phocTr.convertTo(phocTr,CV_64F);
    phocVal.convertTo(phocVal,CV_64F);

    phocTr = phocTr * 2.0 - 1.0;
    phocVal = phocVal * 2.0 - 1.0;

    Util::normalizePerRow(repTr);
    Util::normalizePerRow(repVal);

    Util::normalizePerRow(phocTr);
    Util::normalizePerRow(phocVal);

    cv::Mat1d meanRepTr = Util::meanCenterPerCol(repTr);
    Util::meanCenterPerCol(repVal, meanRepTr);

    cv::Mat meanPhocTr = Util::meanCenterPerCol(phocTr);
    Util::meanCenterPerCol(phocVal, meanPhocTr);

    double bestscore = -1;
    double bestReg = 0.0001;
    double reg = 0.0001;
    for(; reg >= 0.000001; reg *= 0.1){
        cv::Mat Watt, Wphoc;
        CCA::compute(repTr, phocTr, reg, Watt, Wphoc);

        cv::Mat1d repVal_cca = Watt.rowRange(0,config.dimCCA)*(repVal.t()); //(dimCCA * numData)
        cv::Mat1d phocVal_cca = Wphoc.rowRange(0,config.dimCCA)*(phocVal.t());
        repVal_cca = repVal_cca.t();
        phocVal_cca = phocVal_cca.t();
        //L2 norm
        Util::normalizePerRow(repVal_cca);
        Util::normalizePerRow(phocVal_cca);

        //QBE
        cv::Mat1d mAP, p1;
        Evaluate:: computemAP(config, repVal_cca, repVal_cca, wordClsVal, labelVal, mAP, p1);
        double qbe_val_map = cv::mean(mAP)[0];
        double qbe_val_p1 = cv::mean(p1)[0];

        //QBS
        Evaluate:: computemAP(config, phocVal_cca, repVal_cca, wordClsVal, labelVal, mAP, p1, true);
        double qbs_val_map = cv::mean(mAP)[0];
        double qbs_val_p1 = cv::mean(p1)[0];

        //test

        if(qbe_val_map + qbs_val_map > bestscore){
            bestscore = qbe_val_map + qbs_val_map;
            bestReg = reg;
        }
    }

    repTr.release();
    repVal.release();
    phocTr.release();
    phocVal.release();
    wordClsTr.release();
    wordClsVal.release();

    phocFull.convertTo(phocFull,CV_64F);

    phocFull = phocFull * 2.0 - 1.0;

    Util::normalizePerRow(repFull);
    Util::normalizePerRow(phocFull);

    cv::Mat1d meanRep = Util::meanCenterPerCol(repFull);
    cv::Mat1d meanPhoc = Util::meanCenterPerCol(phocFull);

    cv::Mat Wx, Wy;
    CCA::compute(repFull, phocFull, bestReg, Wx, Wy);

    cv::Mat1d repTe = attriRepTe.clone();
    cv::Mat phocTe = phocsTe.clone();

    phocTe.convertTo(phocTe, CV_64F);

    phocTe = phocTe *2.0 -1.0;

    Util::normalizePerRow(repTe);
    Util::normalizePerRow(phocTe);

    cv::Mat repTe_old = repTe.clone();
    cv::Mat phocTe_old = phocTe.clone();

    Util::meanCenterPerCol(repTe_old, embedding.mAtt);
    Util::meanCenterPerCol(phocTe_old, embedding.mPhoc);

    cv::Mat1d repTe_CCA =embedding.Watt.rowRange(0,config.dimCCA)*(repTe_old.t());
    cv::Mat1d phocTe_CCA = embedding.Wphoc.rowRange(0,config.dimCCA)*(phocTe_old.t());

    repTe_CCA = repTe_CCA.t();
    phocTe_CCA = phocTe_CCA.t();
    Util::normalizePerRow(repTe_CCA);
    Util::normalizePerRow(phocTe_CCA);

    cv::Mat1d mAP, p1;
    //QBE
    Evaluate :: computemAP(config, repTe_CCA, repTe_CCA, wordClsTe, labelTe, mAP, p1);
    double qbe_val_map = cv::mean(mAP)[0];
    double qbe_val_p1 = cv::mean(p1)[0];
    //QBS
    Evaluate :: computemAP(config, phocTe_CCA, repTe_CCA, wordClsTe, labelTe, mAP, p1, true);
    double qbs_val_map = cv::mean(mAP)[0];
    double qbs_val_p1 = cv::mean(p1)[0];

    double oldrate = qbe_val_map+qbs_val_map;
    qDebug()<<"old rate:" <<qbe_val_map<<" "<<qbs_val_map<<"sum"<<oldrate;
    mAP.release();
    p1.release();

    repTe_old.release();
    phocTe_old.release();

    Util::meanCenterPerCol(repTe, meanRep);
    Util::meanCenterPerCol(phocTe, meanPhoc);

    repTe_CCA =Wx.rowRange(0,config.dimCCA)*(repTe.t());
    phocTe_CCA = Wy.rowRange(0,config.dimCCA)*(phocTe.t());

    repTe_CCA = repTe_CCA.t();
    phocTe_CCA = phocTe_CCA.t();
    Util::normalizePerRow(repTe_CCA);
    Util::normalizePerRow(phocTe_CCA);

    //QBE
    Evaluate :: computemAP(config, repTe_CCA, repTe_CCA, wordClsTe, labelTe, mAP, p1);
    qbe_val_map = cv::mean(mAP)[0];
    qbe_val_p1 = cv::mean(p1)[0];
    //QBS
    Evaluate :: computemAP(config, phocTe_CCA, repTe_CCA, wordClsTe, labelTe, mAP, p1, true);
    qbs_val_map = cv::mean(mAP)[0];
    qbs_val_p1 = cv::mean(p1)[0];
    qDebug()<<"new rate:" <<qbe_val_map<<" "<<qbs_val_map<<"sum"<<qbe_val_map+qbs_val_map;

    if(oldrate < qbe_val_map+qbs_val_map){
        embedding.Watt = Wx;
        embedding.Wphoc = Wy;
        embedding.reg = bestReg;
        embedding.mAtt = meanRep;
        embedding.mPhoc = meanPhoc;
        qDebug()<<"update sucess.";
        re = true;
    }

    qDebug()<<"Done.";
    return re;
}

}
