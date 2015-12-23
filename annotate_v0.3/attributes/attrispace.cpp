#include "attrispace.h"

#include <QDebug>
#include <QDir>

extern "C" {
#include<vl/svm.h>
}

namespace puhma{
AttriSpace :: AttriSpace(const AttriConfig &c,
        const FileHandler *fpm,
        const DataPrepare *dp,
        Statistic *sta)
    : config(c), filePathManager(fpm), st(sta)
{
    dataTrain = dp->getDivision("train");
    dataTest = dp->getDivision("test");
    dataValidation = dp->getDivision("vali");
    fv = new FV(config, filePathManager, st);
    tp = new TransPhoc(config, filePathManager);
    int dim = (tp->getSize()).width;
    int dimFV = (fv->getSize()).width;
    attModel = new AttriModel(dim, dimFV, filePathManager);
}

void AttriSpace :: train()
{
    //dim. of attribute
    int dim = tp->getSize().width;
    //dim. of fisher vector
    int dimFV = fv->getSize().width;
    //num of data
    int numOfData = dataTrain.size()+dataValidation.size();

    if(filePathManager->fileExist("attModel")){
        qDebug()<<"attribute space has already been learned.";
        /*qDebug()<<"reloading attributes representation...";
        reloadAttRep("train");
        reloadAttRep("vali");
        reloadAttRep("test");
        qDebug()<<"Done.";*/
    }else{
        qDebug()<<"Training attribute space...";
        st->setStatus("preparing...");
        //prepare statistic
        std::vector<double> lrate(dim,-1.0);
        (st->data)["learningrate"] = lrate;

        cv::Mat featuresTr = cv::Mat::zeros(numOfData, dimFV, CV_32F);
        cv::Mat featuresVal;
        cv::Mat phocTr = cv::Mat::zeros(numOfData, dim, CV_8U);
        cv::Mat phocVal;
        //mapping  idx -> numberOfData as [Tr Val]
        std::vector<int> idxTrMapping;
        std::vector<int> idxValMapping;

        //load the train and validation feature/phoc
        qDebug()<<"Loading features and phoc from training set...";
        for(int i=0;i < dataTrain.size(); ++i){
            std::string fn = dataTrain[i][0];
            fv->fetchOneFeature(fn).copyTo(featuresTr.row(i));
            tp->fetchOnePhocFromFile(fn).copyTo(phocTr.row(i));
            idxTrMapping.push_back(i);
        }
        qDebug()<<"Done";
        qDebug()<<"Loading features and phoc from validation set...";
        for(int i=0;i < dataValidation.size(); ++i){
            std::string fn = dataValidation[i][0];
            fv->fetchOneFeature(fn).copyTo(featuresTr.row(dataTrain.size()+i));
            tp->fetchOnePhocFromFile(fn).copyTo(phocTr.row(dataTrain.size()+i));
            idxTrMapping.push_back(dataTrain.size()+i);
        }
        qDebug()<<"Done";

        featuresTr.convertTo(featuresTr, CV_64F);
        featuresVal.convertTo(featuresVal, CV_64F);
        cv::Mat1b msk = cv::Mat1b::zeros(phocTr.size());
        phocTr = (phocTr>msk);
        phocTr = phocTr/255;
        msk.release();
        //dataprepare done.

        //if dir not exists, create it.
        std::string dirname = filePathManager->attriModelDir;
        if(!(filePathManager->dirExist("attrimodel"))){
            if(!(QDir(QString::fromStdString(dirname)).mkpath("."))){
                throw std::runtime_error("AttriSpace :: compute : can not create Dir.");
            }
        }

        //training
        for(int i=0; i<dim; ++i){
            st->setStatus("training attribute number "+std::to_string(i+1)+", total dimensation: "+std::to_string(dim));
            train1d(i, featuresTr, featuresVal,
                    phocTr, phocVal, idxTrMapping, idxValMapping);
        }

        //do clean
        featuresTr.release();
        featuresVal.release();
        phocTr.release();
        phocVal.release();
        qDebug()<<"Training Done.";

        //compute the attribute representation
        qDebug()<<"Create Attributes Representation of Train set...";
        st->setStatus("Generate attributes representation...");
        cv::Mat1d attriRepTr = cv::Mat1d::zeros(numOfData, dim);
        
        //std::cout<<attriRepTr.rows<<std::endl;
        for(int i=0; i< dim; ++i){
            AttriModel1D am(i, filePathManager);  
            if(am.rateVali>config.discardThreshold){
                attModel->update(am);
                //std::cout<<am.encodedFeat.rows<<std::endl;
                am.encodedFeat.copyTo(attriRepTr.col(i));
            }
        }

        dumpAttRep("train", attriRepTr);
        attriRepTr.release();
        qDebug()<<"Done.";

        qDebug()<<"Loading Features from test set";
        cv::Mat featuresTe(dataTest.size(), dimFV, CV_32F);
        for(int i=0; i< dataTest.size(); ++i){
            std::string fn = dataTest[i][0];
            fv->fetchOneFeature(fn).copyTo(featuresTe.row(i));
        }
        qDebug()<<"Done.";

        qDebug()<<"Calculating the attributes representation of Test set...";
        featuresTe.convertTo(featuresTe,CV_64F);
        cv::Mat1d teB = cv::repeat(attModel->B, featuresTe.rows, 1);
        cv::Mat1d attriRepTe = attModel->W * featuresTe.t();
        attriRepTe = attriRepTe.t()+teB;
        dumpAttRep("test", attriRepTe);
        attriRepTe.release();
        featuresTe.release();
        qDebug()<<"Done.";

        attModel->dump();
        st->setStatus("");
    }
}

void AttriSpace :: train1d(const int numAttri,
        cv::Mat &featuresTr, //1d
        cv::Mat &featuresVal,
        cv::Mat &phocTr, //1b
        cv::Mat &phocVal,
        std::vector<int> &idxTrMapping,
        std::vector<int> &idxValMapping)
{
    /*if(idxValMapping.size()!=0){
        CV_Assert(featuresTr.rows == phocTr.rows && featuresTr.rows == idxTrMapping.size() &&
                featuresVal.rows == phocVal.rows && featuresVal.rows == idxValMapping.size());
        featuresTr.push_back(featuresVal);
        featuresVal.resize(0);
        phocTr.push_back(phocVal);
        phocVal.resize(0);
        idxTrMapping.insert(idxTrMapping.end(),idxValMapping.begin(),idxValMapping.end());
        idxValMapping.clear();
        idxValMapping.shrink_to_fit();
    }else{*/
    CV_Assert(idxValMapping.size() == 0 && //data is postprocessed
            featuresTr.rows == phocTr.rows && //training data and transcriptor
            idxTrMapping.size() == featuresTr.rows);
    /*}*/

    int numOfData = featuresTr.rows;
    int dimOfFeat = featuresTr.cols;
    int dim = phocTr.cols;

    //check if file exists.
    AttriModel1D am(numAttri, dimOfFeat, numOfData, filePathManager);
    if(am.modelExtracted()){
        qDebug()<<"Attribute"<<numAttri<<":"<<QString::fromStdString(tp->getLabelat(numAttri))<<"already computed.";
        am.load();
    }else{
        qDebug()<<"Learning model of attribute"<<numAttri<<":"<<QString::fromStdString(tp->getLabelat(numAttri))<<"...";
        
        std::vector<int> idxPos;
        std::vector<int> idxNeg;
        for(int i=0; i< idxTrMapping.size(); ++i){
            if(phocTr.at<uchar>(i,numAttri)>0){
                idxPos.push_back(i);
            }else{
                idxNeg.push_back(i);
            }
        }
        
        //filter the attributes which has too low number of positive.
        if(idxPos.size()<2){
            qDebug()<<"Model of attribute"<<numAttri<<":"<<QString::fromStdString(tp->getLabelat(numAttri))<<"discarded, because of lack of data.";
            am.dump();
        }else{
            int N = 0;
            cv::Mat1i Np = cv::Mat1i::zeros(1, numOfData);
            cv::Mat1d W = cv::Mat1d::zeros(1, dimOfFeat);
            double B = 0;
            int numPosSamples = 0;

            int numPass = 2;
            int numIters = 5;
            double sumValiRate = 0;

            for(int i=0; i<numPass; ++i){
                Util::generateRan(idxPos);
                Util::generateRan(idxNeg);

                //80% data for purpose of training, 20% for validation
                int nTrainPos = static_cast<int>(std::floor(idxPos.size()*0.8));
                int nValPos = idxPos.size()-nTrainPos;
                int nTrainNeg = static_cast<int>(std::floor(idxNeg.size()*0.8));
                int nValNeg = idxNeg.size()-nTrainNeg;

                for(int j=0; j<numIters; ++j){
                    std::vector<int> idxPost(idxPos.size());
                    std::vector<int> idxNegt(idxNeg.size());
                    int numTr = nTrainPos + nTrainNeg;

                    //assemble the validation set
                    std::vector<int> idxValSVM(idxPos.begin()+nTrainPos,idxPos.end());
                    std::sort(idxValSVM.begin(),idxValSVM.end());
                    std::vector<int> idxValNSVM(idxNeg.begin()+nTrainNeg,idxNeg.end());
                    std::sort(idxValNSVM.begin(),idxValNSVM.end());

                    idxValSVM.insert(idxValSVM.end(), idxValNSVM.begin(), idxValNSVM.end());
                    idxValNSVM.clear();
                    idxValNSVM.shrink_to_fit();

                    featuresVal = cv::Mat::zeros(idxValSVM.size(),dimOfFeat,CV_64F);
                    phocVal = cv::Mat::zeros(idxValSVM.size(),dim,CV_8U);
                    for(int k=0; k< idxValSVM.size(); ++k){
                        featuresTr.row(idxValSVM[k]).copyTo(featuresVal.row(k));
                        phocTr.row(idxValSVM[k]).copyTo(phocVal.row(k));
                        idxValMapping.push_back(idxTrMapping[idxValSVM[k]]);
                        
                        if(k<nValPos){
                            size_t idx=std::distance(idxPos.begin(),
                                    std::find(idxPos.begin(),idxPos.end(),idxValSVM[k]));
                            idxPost[idx]= numTr+k;
                        }else{
                            size_t idx=std::distance(idxNeg.begin(),
                                    std::find(idxNeg.begin(),idxNeg.end(),idxValSVM[k])); 
                            idxNegt[idx] = numTr+k;
                        }

                    }
                    idxValSVM.clear();
                    idxValSVM.shrink_to_fit();

                    //assemble the training set
                    std::vector<int> idxTrainSVM(idxPos.begin(),idxPos.begin()+nTrainPos);
                    std::vector<int> idxTrainNSVM(idxNeg.begin(),idxNeg.begin()+nTrainNeg);
                    idxTrainSVM.insert(idxTrainSVM.end(), idxTrainNSVM.begin(), idxTrainNSVM.end());
                    idxTrainNSVM.clear();
                    idxTrainNSVM.shrink_to_fit();
                    std::sort(idxTrainSVM.begin(),idxTrainSVM.end());

                    for(int k=0; k< idxTrainSVM.size(); ++k){
                        featuresTr.row(idxTrainSVM[k]).copyTo(featuresTr.row(k));
                        phocTr.row(idxTrainSVM[k]).copyTo(phocTr.row(k));
                        idxTrMapping[k]=idxTrMapping[idxTrainSVM[k]];
                        if(phocTr.at<uchar>(k,numAttri)>0){
                            size_t idx=std::distance(idxPos.begin(),
                                    std::find(idxPos.begin(),idxPos.end(),idxTrainSVM[k]));
                            CV_Assert(idx != idxPos.size());
                            idxPost[idx] = k;
                        }else{
                            size_t idx=std::distance(idxNeg.begin(),
                                    std::find(idxNeg.begin(),idxNeg.end(),idxTrainSVM[k])); 
                            CV_Assert(idx != idxNeg.size());
                            idxNegt[idx] = k;
                        }
                    }

                    //postprocess
                    featuresTr.resize(idxTrainSVM.size());
                    phocTr.resize(idxTrainSVM.size());
                    idxTrMapping.resize(idxTrainSVM.size());
                    idxTrMapping.shrink_to_fit();
                    idxTrainSVM.clear();
                    idxTrainSVM.shrink_to_fit();
                    idxPos = idxPost;
                    idxNeg = idxNegt;
                    idxPost.clear();
                    idxPost.shrink_to_fit();
                    idxNegt.clear();
                    idxNegt.shrink_to_fit();
                    
                    numPosSamples += nTrainPos;

                    //prepare labels for training
                    cv::Mat phocTr1Dim = phocTr.col(numAttri).clone();
                    phocTr1Dim.convertTo(phocTr1Dim, CV_64F);
                    phocTr1Dim = phocTr1Dim*2-1;

                    if(!(featuresTr.isContinuous() && phocTr1Dim.isContinuous()))
                        throw std::runtime_error("AttriSpace::learn_Attri_error: matirx data not continue.");

                    //try lambda from 0.001 to 0.00001
                    double lambda = 0.001;
                    double bestmap = 0;
                    double bestlam = lambda;
                    AttriModel1D *tmpModel = new AttriModel1D(numAttri, dimOfFeat, numOfData,filePathManager);
                    for(int c = 0; c<3; ++c){
                        VlSvm * svm = vl_svm_new(VlSvmSolverSdca,
                                (double const *)(featuresTr.data), (vl_size)dimOfFeat, (vl_size)featuresTr.rows,
                                (double const *)(phocTr1Dim.data),
                                lambda);
                        vl_svm_set_epsilon(svm, 0.001);
                        vl_svm_set_bias_multiplier(svm, 0.1);
                        vl_svm_set_max_num_iterations(svm, 10/lambda);
                        vl_svm_train(svm);
                        const double *model = vl_svm_get_model(svm);
                        double bias = vl_svm_get_bias(svm);


                        cv::Mat1d Wv(1, dimOfFeat); //, model);
                        CV_Assert(Wv.isContinuous());
                        memcpy(Wv.data, model, sizeof(double)*dimOfFeat);

                        //process validation data
                        cv::Mat1d score = Wv*(featuresVal.t())+bias;
                        cv::Mat phocVal1Dim =  phocVal.col(numAttri).t();
                        phocVal1Dim.convertTo(phocVal1Dim, CV_64F);

                        double cmap = modelMap(score, phocVal1Dim);
                        phocVal1Dim.release();

                        if(cmap>bestmap){
                            tmpModel->W = Wv.clone();
                            tmpModel->B = bias;
                            bestmap = cmap;
                            bestlam = lambda;
                        }
                        lambda*=0.1;
                        vl_svm_delete(svm);
                        Wv.release();
                        score.release();
                    }
                    
                    phocTr1Dim.release();

                    qDebug()<<"it"<<j+1<<"pass"<<i+1<<"("<<bestmap*100<<"%) using"<<nTrainPos<<"positive samples, with lambda:"<<bestlam;

                    sumValiRate += bestmap;

                    ++N;
                    cv::Mat1d sc= (tmpModel->W)*(featuresVal.t())+tmpModel->B;
                    for(size_t ct=0; ct<idxValMapping.size(); ++ct){
                        Np(0,idxValMapping[ct])+=1;
                        am.encodedFeat(idxValMapping[ct],0)+=sc(0,ct);
                    }

                    W += tmpModel->W;
                    B += tmpModel->B;

                    delete tmpModel;

                    sc.release();

                    featuresTr.push_back(featuresVal);
                    featuresVal.resize(0);
                    phocTr.push_back(phocVal);
                    phocVal.resize(0);
                    idxTrMapping.insert(idxTrMapping.end(),idxValMapping.begin(),idxValMapping.end());
                    idxValMapping.clear();
                    idxValMapping.shrink_to_fit();

                    std::rotate(idxPos.begin(), idxPos.begin()+nValPos, idxPos.end());
                    std::rotate(idxNeg.begin(), idxNeg.begin()+nValNeg, idxNeg.end());
                }
            }

            am.numIterat+=1;
            am.rateVali = sumValiRate;
     
            am.W = W;
            am.B = B;
            am.numPosSamples = numPosSamples;
            cv::Mat1d Np1d;
            Np.convertTo(Np1d, CV_64F);
            //std::cout<<Np<<std::endl;
            if(N!=0){
                am.W = am.W/N;
                am.B = am.B/N;
                am.encodedFeat = am.encodedFeat.mul(1.0/(Np1d.t()));
                am.numPosSamples = static_cast<int>(std::ceil(numPosSamples/N));
                am.rateVali = am.rateVali/N;
            }
            am.dump();

            //add valirate to static

            W.release();
            Np.release();
            Np1d.release();

            qDebug()<<"Done";
        }
        idxPos.clear();
        idxPos.shrink_to_fit();
        idxNeg.clear();
        idxPos.shrink_to_fit();
    }
    (st->data)["learningrate"][numAttri] = am.rateVali;
    st->dumpData("learningrate");
}

void AttriSpace :: dumpAttRep(const std::string &type, const cv::Mat1d &score) const{
    std::string fn;
    const cv::Mat1d *attRep;
    if(type == "train"){
        fn = filePathManager->attRepresTrain;
    /*}else if(type == "vali"){//discarded
        fn = filePathManager->attRepresValidation;
        attRep = &attriRepVa;*/
    }else if(type == "test"){
        fn = filePathManager->attRepresTest;
    }else{
        throw std::runtime_error("AttriSpace :: dumpAttRep : type un-acceptable.");
    }
    cv::FileStorage fs(fn, cv::FileStorage::WRITE);
    fs << "attRep" << score;
    fs.release();
}

double AttriSpace :: modelMap(const cv::Mat1d &score, const cv::Mat1d &label){
    CV_Assert(score.size()==label.size());
    cv::Mat idxM;
    cv::sortIdx(score, idxM, CV_SORT_EVERY_ROW+CV_SORT_DESCENDING);
    cv::Mat1d sortedLabel(label.size());
    //std::cout<<idxM.type()<<std::endl; //4
    for(int x=0; x< idxM.cols; ++x){
        sortedLabel(0,x) = label(0,idxM.at<int>(0,x));
    }
    //std::cout<<sortedLabel<<std::endl;
    cv::Mat1d cumsumLabel = sortedLabel.clone();
    for(int x=1;x<sortedLabel.cols;++x){
        cumsumLabel(0,x)=sortedLabel(0,x)+cumsumLabel(0,x-1);
    }
    cv::Mat1d acc = cumsumLabel.mul(sortedLabel);
    for(int x=0; x< acc.cols; ++x){
        acc(0,x) = acc(0,x)/(x+1);
    }
    double N = (cv::sum(label))[0];
    //CV_Assert(N > 0);
    //qDebug()<<N;
    double re =0;
    if(N>0)
        re = (cv::sum(acc))[0]/N;
    return re;
}

cv::Mat1d AttriSpace :: getRep(std::string type, const FileHandler *fpm){
    cv::Mat1d re;
    if(!fpm->fileExist("attModel"))
        throw std::runtime_error("AttriSpace :: getRep : attribute score do not exists in system");
    else{
        std::string fn;
        if(type == "train"){
            fn = fpm->attRepresTrain;
        /*}else if(type == "vali"){
            fn = filePathManager->attRepresValidation;
            attRep = &attriRepVa;*/ //discarded
        }else if(type == "test"){
            fn = fpm->attRepresTest;
        }else{
            throw std::runtime_error("AttriSpace :: getRep : type un-acceptable.");
        }
        cv::FileStorage fs(fn, cv::FileStorage::READ);
        if (!fs.isOpened())
        {
            throw std::runtime_error("AttriSpace :: getRep : can not open file");
        }
        fs["attRep"] >> re;
        fs.release();
    }
    return re;
}

}
