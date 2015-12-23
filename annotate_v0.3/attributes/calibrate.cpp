#include "calibrate.h"
#include "attrispace.h"
#include "eva.h"
#include "cca.h"
#include "lexicon.h"

#include "platt.h"

#include <numeric>
#include <QDebug>

namespace puhma{
/*------------------------------calibrateFac--------------------------------------*/
CalibrateFac::CalibrateFac(){
    registerCalimethod("cca", &CSR::initialize);
    registerCalimethod("platt", &Platt::initialize);
    registerCalimethod("no", &Direct::initialize);
}

//register calibration method
void CalibrateFac::registerCalimethod(const std::string &calimethod, initalCaliFn pfnIntial){
    m_FactoryMap[calimethod] = pfnIntial;
}

//return the instance of calibartion method
Calibrate *CalibrateFac::initalCali(const AttriConfig &c, const FileHandler *fpm, const DataPrepare *dp){
    FactoryMap::iterator it = m_FactoryMap.find(c.calibrateMethod);
    if( it != m_FactoryMap.end() )
        return it->second(c, fpm, dp);
    return NULL;
}
/*------------------------------Calibrate-----------------------------------------*/
Calibrate::Calibrate(const AttriConfig &c, const FileHandler *fpm, const DataPrepare *dp)
    :config(c), filePathManager(fpm)
{
    tp = new TransPhoc(config, filePathManager);
    dataTest = dp->getDivision("test");
    dataTrain = dp->getDivision("train");
    dataValidation = dp->getDivision("vali");
}
/*------------------------------CSR-----------------------------------------------*/
void CSR::compute(){
    if(filePathManager->fileExist("csrModel")){
        qDebug()<<"restoring Learned Common Subspace...";
        embedding.restore(filePathManager);
        qDebug()<<"Done.";
    }else{
        //prepare data.
        qDebug()<<"Learning Common Subspace...";
        //TransPhoc tp(config, filePathManager);
        int dim = tp->getSize().width;
        cv::Mat1d attriRepTr = AttriSpace::getRep("train", filePathManager);
        //use train+vali as full train set(notice that Amazan use only train set.)
        std::vector<int> trPermutation(attriRepTr.rows);
        std::vector<std::string> fns(attriRepTr.rows);
        std::iota(trPermutation.begin(), trPermutation.end(), 0);
        Util::generateRan(trPermutation);
        cv::Mat repFull = cv::Mat::zeros(attriRepTr.size(),attriRepTr.type());
        cv::Mat phocFull = cv::Mat::zeros(trPermutation.size(),dim,CV_8U);
        cv::Mat wordClsFull = cv::Mat::zeros(trPermutation.size(),1,CV_32S);
        std::vector<std::string> labelFull(trPermutation.size());
        for(int i=0;i < trPermutation.size(); ++i){
            int idx = trPermutation[i];  //in attriRepTr, data from trainset comes first.
            attriRepTr.row(idx).copyTo(repFull.row(i));
            if(idx < dataTrain.size()){
                fns[i] = dataTrain[idx][0];
                tp->fetchOnePhocFromFile(fns[i]).copyTo(phocFull.row(i));
                wordClsFull.at<int>(i,0) = std::stoi(dataTrain[idx][3]);
                labelFull[i] = dataTrain[idx][1];
            }else{
                idx = idx-dataTrain.size();
                fns[i] = dataValidation[idx][0];
                tp->fetchOnePhocFromFile(fns[i]).copyTo(phocFull.row(i));
                wordClsFull.at<int>(i,0) = std::stoi(dataValidation[idx][3]);
                labelFull[i] = dataValidation[idx][1];
            }
        }
        //Evaluate :: computePhocClassNumber(attriRepTr, phocFull, wordClsFull);
        cv::Mat1b msk = cv::Mat1b::zeros(phocFull.size());
        phocFull = (phocFull>msk);
        phocFull = phocFull/255;
        //create train & validation partition
        int nVal = static_cast<int>(std::floor(trPermutation.size()*0.3));
        cv::Mat repTr = repFull.rowRange(nVal, repFull.rows).clone();
        cv::Mat repVal = repFull.rowRange(0, nVal).clone();//64F
        cv::Mat phocTr = phocFull.rowRange(nVal, phocFull.rows).clone();
        cv::Mat phocVal = phocFull.rowRange(0, nVal).clone();//8U
        cv::Mat wordClsTr = wordClsFull.rowRange(nVal, wordClsFull.rows).clone();
        cv::Mat wordClsVal = wordClsFull.rowRange(0, nVal).clone();//32S
        std::vector<std::string> labelTr(labelFull.begin()+nVal, labelFull.end());
        std::vector<std::string> labelVal(labelFull.begin(), labelFull.begin()+nVal);

        //preprocess
        phocTr.convertTo(phocTr,CV_64F);
        phocVal.convertTo(phocVal,CV_64F);
        phocFull.convertTo(phocFull,CV_64F);
        phocTr = phocTr * 2.0 - 1.0;
        phocVal = phocVal * 2.0 - 1.0;
        phocFull = phocFull * 2.0 - 1.0;
        Util::normalizePerRow(repTr);
        Util::normalizePerRow(repVal);
        //CCA::normalizePerRow(repFull);
        Util::normalizePerRow(phocTr);
        Util::normalizePerRow(phocVal);
        //CCA::normalizePerRow(phocFull);
        cv::Mat1d meanRepTr = Util::meanCenterPerCol(repTr);
        Util::meanCenterPerCol(repVal, meanRepTr);
        //CCA::meanCenter(repVal);
        cv::Mat meanPhocTr = Util::meanCenterPerCol(phocTr);
        Util::meanCenterPerCol(phocVal, meanPhocTr);
        //CCA::meanCenter(phocVal);
        //CCA::normalizePerRow(repTr);
        //CCA::normalizePerRow(repVal);
        //CCA::normalizePerRow(repFull);
        //CCA::normalizePerRow(phocTr);
        //CCA::normalizePerRow(phocVal);
        //CCA::normalizePerRow(phocFull);

        //compute using cca
        double bestscore = -1;
        double bestReg = 0.0001;
        double reg = 0.0001;
        for(; reg >= 0.000001; reg *= 0.1){
            qDebug()<<"Regulation:"<<reg<<". computing projection matrices...";
            cv::Mat Watt, Wphoc;
            CCA::compute(repTr, phocTr, reg, Watt, Wphoc);
            qDebug()<<"Done.";

            //evaluation
            qDebug()<<"Preparing Validation...";
            //embed val
            cv::Mat1d repVal_cca = Watt.rowRange(0,config.dimCCA)*(repVal.t()); //(dimCCA * numData)
            cv::Mat1d phocVal_cca = Wphoc.rowRange(0,config.dimCCA)*(phocVal.t());
            repVal_cca = repVal_cca.t();
            phocVal_cca = phocVal_cca.t();
            //L2 norm
            Util::normalizePerRow(repVal_cca);
            Util::normalizePerRow(phocVal_cca);
            qDebug()<<"Done";

            //QBE
            qDebug()<<"Validating...";
            cv::Mat1d mAP, p1;
            Evaluate:: computemAP(config, repVal_cca, repVal_cca, wordClsVal, labelVal, mAP, p1);
            double qbe_val_map = cv::mean(mAP)[0];
            double qbe_val_p1 = cv::mean(p1)[0];

            //QBS
            Evaluate:: computemAP(config, phocVal_cca, repVal_cca, wordClsVal, labelVal, mAP, p1, true);
            double qbs_val_map = cv::mean(mAP)[0];
            double qbs_val_p1 = cv::mean(p1)[0];

            //test
            qDebug()<<"QBE: -- val: (map:"<<100*qbe_val_map<<", p@1:"<<100*qbe_val_p1<<")";
            qDebug()<<"QBS: -- val: (map:"<<100*qbs_val_map<<", p@1:"<<100*qbs_val_p1<<")";

            if(qbe_val_map + qbs_val_map > bestscore){
                bestscore = qbe_val_map + qbs_val_map;
                bestReg = reg;
            }
        }

        qDebug()<<"Best qbe map result on validation:"<<100*bestscore/2<<"map with reg"<<bestReg;
        qDebug()<<"Computting Embedding model...";

        Util::normalizePerRow(repFull);
        Util::normalizePerRow(phocFull);

        cv::Mat1d meanRep = Util::meanCenterPerCol(repFull);
        cv::Mat1d meanPhoc = Util::meanCenterPerCol(phocFull);

        //CCA::normalizePerRow(repFull);
        //CCA::normalizePerRow(phocFull);

        cv::Mat Wx, Wy;
        CCA::compute(repFull, phocFull, bestReg, Wx, Wy);
        embedding.Watt = Wx;
        embedding.Wphoc = Wy;
        embedding.reg = bestReg;
        embedding.mAtt = meanRep;
        embedding.mPhoc = meanPhoc;

        embedding.dump(filePathManager);

        qDebug()<<"Done.";

        //updating lexicon with training dataset
        qDebug()<<"Updating Lexicon...";
        Lexicon *lex = Lexicon::get(config, filePathManager);
        for(size_t i=0; i<labelFull.size(); ++i){
            lex->addWord(labelFull[i], fns[i], wordClsFull.at<int>(i,0));
        }
        lex->dump();
        qDebug()<<"Done.";
    }
}

void CSR::evaluate(){
    qDebug()<<"Evaluating learned common subspace...";

    //TransPhoc tp(config, filePathManager);
    int dim = tp->getSize().width;

    cv::Mat1d repTe = AttriSpace::getRep("test", filePathManager);
    cv::Mat phocTe = cv::Mat::zeros(dataTest.size(), dim, CV_8U);
    cv::Mat wordClsTe = cv::Mat::zeros(dataTest.size(), 1, CV_32S);
    std::vector<std::string> labelTe(dataTest.size());
    for(int i=0; i< dataTest.size(); ++i){
        tp->fetchOnePhocFromFile(dataTest[i][0]).copyTo(phocTe.row(i));
        wordClsTe.at<int>(i,0) = std::stoi(dataTest[i][3]);
        labelTe[i] = dataTest[i][1];
    }

    //perprocess
    cv::Mat1b msk = cv::Mat1b::zeros(phocTe.size());
    phocTe = (phocTe>msk);
    phocTe = phocTe/255;
    phocTe.convertTo(phocTe, CV_64F);
    phocTe = phocTe *2.0 -1.0;
    Util::normalizePerRow(repTe);
    Util::normalizePerRow(phocTe);
    Util::meanCenterPerCol(repTe, embedding.mAtt);
    Util::meanCenterPerCol(phocTe, embedding.mPhoc);
    //CCA::meanCenter(repTe);
    //CCA::meanCenter(phocTe);

    //CCA::normalizePerRow(repTe);
    //CCA::normalizePerRow(phocTe);

    cv::Mat1d repTe_CCA = embedding.Watt.rowRange(0,config.dimCCA)*(repTe.t());
    cv::Mat1d phocTe_CCA = embedding.Wphoc.rowRange(0,config.dimCCA)*(phocTe.t());

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

    qDebug()<<"qbe --   test: (map:"<<100*qbe_val_map<<". p@1:"<<100*qbe_val_p1<<")";
    qDebug()<<"qbs --   test: (map:"<<100*qbs_val_map<<". p@1:"<<100*qbs_val_p1<<")";

    qDebug()<<"Done.";
}
/*------------------------------Platt---------------------------------------------*/
void Platt::compute(){
    qDebug()<<"Calibarating using platt...";
    //prepare
    int dim = tp->getSize().width;
    cv::Mat1d repFull = AttriSpace::getRep("train", filePathManager);
    cv::Mat phocFull = cv::Mat::zeros(repFull.rows,dim,CV_8U);
    for(int i=0;i < repFull.rows; ++i){
        if(i < dataTrain.size()){
            tp->fetchOnePhocFromFile(dataTrain[i][0]).copyTo(phocFull.row(i));
        }else{
            int idx = i-dataTrain.size();
            tp->fetchOnePhocFromFile(dataValidation[idx][0]).copyTo(phocFull.row(i));
        }
    }

    //preprocess
    //Util::normalizePerRow(repFull);
    //Util::meanCenterPerCol(repFull);
    cv::Mat1b msk = cv::Mat1b::zeros(phocFull.size());
    phocFull = (phocFull>msk);
    phocFull = phocFull/255;
    
    platts = cv::Mat1d::zeros(2, dim);

    for(int idxDim=0; idxDim<dim; ++idxDim){
        double A=0,B=0;
        cv::Mat1d decision1d = repFull.col(idxDim);
        cv::Mat1i label1d;
        (phocFull.col(idxDim).clone()).convertTo(label1d, CV_32S);
        platt_calibrate(decision1d, label1d, A, B);
        platts(0,idxDim) = A;
        platts(1,idxDim) = B;
    }
    qDebug()<<"Done";
}

void Platt::evaluate()
{
    qDebug()<<"Evaluating calibration...";

    //prepare
    int dim = tp->getSize().width;
    cv::Mat1d repTe = AttriSpace::getRep("test", filePathManager);
    cv::Mat phocTe = cv::Mat::zeros(dataTest.size(), dim, CV_8U);
    cv::Mat wordClsTe = cv::Mat::zeros(dataTest.size(), 1, CV_32S);
    std::vector<std::string> labelTe(dataTest.size());
    for(int i=0; i< dataTest.size(); ++i){
        tp->fetchOnePhocFromFile(dataTest[i][0]).copyTo(phocTe.row(i));
        wordClsTe.at<int>(i,0) = std::stoi(dataTest[i][3]);
        labelTe[i] = dataTest[i][1];
    }

    //preprocess
    //cv::Mat1b msk = cv::Mat1b::zeros(phocTe.size());
    //phocTe = (phocTe>msk);
    //phocTe = phocTe/255;
    //Util::normalizePerRow(repTe);
    //Util::meanCenterPerCol(repTe);
    cv::Mat1d repTePlatts = cv::Mat1d::zeros(repTe.size());
    for(int x=0; x<repTePlatts.cols; ++x){
        double A = platts(0,x);
        double B = platts(1,x);
        for(int y=0; y<repTePlatts.rows; ++y){
            repTePlatts(y,x) = sigmoid_predict(repTe(y,x),A,B);
        }
    }

    Util::normalizePerRow(repTePlatts);

    phocTe.convertTo(phocTe, CV_64F);
    Util::normalizePerRow(phocTe);

    //evaluate
    cv::Mat1d mAP, p1;
    //QBE
    Evaluate :: computemAP(config, repTePlatts, repTePlatts, wordClsTe, labelTe, mAP, p1);
    double qbe_val_map = cv::mean(mAP)[0];
    double qbe_val_p1 = cv::mean(p1)[0];
    //QBS
    Evaluate :: computemAP(config, phocTe, repTePlatts, wordClsTe, labelTe, mAP, p1, true);
    double qbs_val_map = cv::mean(mAP)[0];
    double qbs_val_p1 = cv::mean(p1)[0];

    qDebug()<<"qbe --   test: (map:"<<100*qbe_val_map<<". p@1:"<<100*qbe_val_p1<<")";
    qDebug()<<"qbs --   test: (map:"<<100*qbs_val_map<<". p@1:"<<100*qbs_val_p1<<")";

    qDebug()<<"Done.";
}

/*------------------------------Direct--------------------------------------------*/
void Direct::evaluate()
{
    qDebug()<<"Evaluating direct comparison...";

    //TransPhoc tp(config, filePathManager);
    int dim = tp->getSize().width;

    //prepare.
    cv::Mat1d repTe = AttriSpace::getRep("test", filePathManager);
    cv::Mat phocTe = cv::Mat::zeros(dataTest.size(), dim, CV_8U);
    cv::Mat wordClsTe = cv::Mat::zeros(dataTest.size(), 1, CV_32S);
    std::vector<std::string> labelTe(dataTest.size());
    for(int i=0; i< dataTest.size(); ++i){
        tp->fetchOnePhocFromFile(dataTest[i][0]).copyTo(phocTe.row(i));
        wordClsTe.at<int>(i,0) = std::stoi(dataTest[i][3]);
        labelTe[i] = dataTest[i][1];
    }
    cv::Mat1b msk = cv::Mat1b::zeros(phocTe.size());
    phocTe = (phocTe>msk);
    phocTe = phocTe/255;

    //preprocess
    Util::normalizePerRow(repTe);
    phocTe.convertTo(phocTe, CV_64F);
    //phocTe = phocTe *2.0 -1.0;
    //Util::normalizePerRow(phocTe);
    Util::meanCenterPerCol(repTe);
    Util::meanCenterPerCol(phocTe);
    Util::normalizePerRow(repTe);
    Util::normalizePerRow(phocTe);

    //evaluate
    cv::Mat1d mAP, p1;
    //QBE
    Evaluate :: computemAP(config, repTe, repTe, wordClsTe, labelTe, mAP, p1);
    double qbe_val_map = cv::mean(mAP)[0];
    double qbe_val_p1 = cv::mean(p1)[0];
    //QBS
    Evaluate :: computemAP(config, phocTe, repTe, wordClsTe, labelTe, mAP, p1, true);
    double qbs_val_map = cv::mean(mAP)[0];
    double qbs_val_p1 = cv::mean(p1)[0];

    qDebug()<<"qbe --   test: (map:"<<100*qbe_val_map<<". p@1:"<<100*qbe_val_p1<<")";
    qDebug()<<"qbs --   test: (map:"<<100*qbs_val_map<<". p@1:"<<100*qbs_val_p1<<")";

    qDebug()<<"Done.";
}

}
