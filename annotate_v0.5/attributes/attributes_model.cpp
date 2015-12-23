#include "attributes_model.h"

#include <QDir>
#include <QFileInfo>

namespace puhma{
void Embedding :: dump(const FileHandler *fpm){
    std::string fn = fpm->csrModelPath;
    cv::FileStorage fs(fn, cv::FileStorage::WRITE);
    fs << "reg" << reg;
    fs << "Watt" << Watt;
    fs << "Wphoc" << Wphoc;
    fs << "mAtt" << mAtt;
    fs << "mPhoc" << mPhoc;
    fs.release();
}

void Embedding :: restore(const FileHandler *fpm){
    std::string fn = fpm->csrModelPath;
    cv::FileStorage fs(fn, cv::FileStorage::READ);
    if (!fs.isOpened())
    {
        throw std::runtime_error("Embedding :: restore : can not open file");
    }
    fs["reg"] >> reg;
    fs["Watt"] >> Watt;
    fs["Wphoc"] >> Wphoc;
    fs["mAtt"] >> mAtt;
    fs["mPhoc"] >> mPhoc;
    fs.release();
}

AttriModel1D :: AttriModel1D(int dimnum, const FileHandler *fpm)
    : dim(dimnum), fpm(fpm){
    load();
}
AttriModel1D :: AttriModel1D(int dimnum, int dimFeat, int numOfData, const FileHandler *fpm)
    : dim(dimnum), fpm(fpm){
    W = cv::Mat1d::zeros(1, dimFeat);
    encodedFeat = cv::Mat1d::zeros(numOfData,1);
    B = 0;
    numPosSamples = 0;
    numIterat = 0;
    rateVali = 0;
}

AttriModel1D :: AttriModel1D(int dimnum, cv::Mat1d Wv, cv::Mat1d encodedFeat, const FileHandler *fpm, double bias, int numPosSamples, int numIterat, double rateVali)
    : dim(dimnum), W(Wv), encodedFeat(encodedFeat), B(bias), numPosSamples(numPosSamples), fpm(fpm), numIterat(numIterat), rateVali(rateVali){
}

bool AttriModel1D :: modelExtracted() const{
    std::string dirname = fpm->attriModelDir;
    std::string filename = "model_"+std::to_string(dim)+".yaml";
    QString fn = QString::fromStdString(filename);
    QFileInfo filemodel = QFileInfo(QDir(QString::fromStdString(dirname)),fn);
    return filemodel.exists();
}

void AttriModel1D :: dump() const{
    std::string dirname = fpm->attriModelDir;
    std::string filename = "model_"+std::to_string(dim)+".yaml";
    QString fn = QString::fromStdString(filename);
    QFileInfo filemodel = QFileInfo(QDir(QString::fromStdString(dirname)),fn);
    std::string fp=filemodel.filePath().toStdString();

    cv::FileStorage fs(fp, cv::FileStorage::WRITE);
    fs << "rate" << rateVali;
    fs << "interate" << numIterat;
    fs << "W" << W;
    fs << "B" << B;
    fs << "numPosSamples" << numPosSamples;
    fs << "encode" << encodedFeat;
    fs.release();
}

void AttriModel1D :: dumpOnline() const{
    std::string dirname = fpm->attriModelDir;
    std::string filename = "model_"+std::to_string(dim)+"_updated.yaml";
    QString fn = QString::fromStdString(filename);
    QFileInfo filemodel = QFileInfo(QDir(QString::fromStdString(dirname)),fn);
    std::string fp=filemodel.filePath().toStdString();

    cv::FileStorage fs(fp, cv::FileStorage::WRITE);
    fs << "rate" << rateVali;
    fs << "interate" << numIterat;
    fs << "W" << W;
    fs << "B" << B;
    fs << "numPosSamples" << numPosSamples;
    fs << "encode" << encodedFeat;
    fs.release();
}

void AttriModel1D :: load(){
    std::string dirname = fpm->attriModelDir;
    if(!(fpm->dirExist("attrimodel"))){
        throw std::runtime_error("AttriModel :: load : Models do not exist.");
    }

    std::string filename = "model_"+std::to_string(dim)+".yaml";
    QString fn = QString::fromStdString(filename);
    QFileInfo filemodel = QFileInfo(QDir(QString::fromStdString(dirname)),fn);

    if(!filemodel.exists()){
        throw std::runtime_error("AttriModel:: load : Model do not exist.");
    }
    
    std::string fp=filemodel.filePath().toStdString();

    cv::FileStorage fs(fp, cv::FileStorage::READ);
    if (!fs.isOpened())
    {
        throw std::runtime_error("AttriModel1D :: load : can not open file");
    }
    fs["rate"] >> rateVali;
    fs["interate"] >> numIterat;
    fs["W"] >> W;
    fs["B"] >> B;
    fs["numPosSamples"] >> numPosSamples;
    fs["encode"] >> encodedFeat;
    fs.release();
}

/*======================AttriModel=======================*/

AttriModel :: AttriModel(const int dimOfAtt, const int dimOfFeat, const FileHandler *fpm)
    : filePathManager(fpm) {
    W = cv::Mat1d::zeros(dimOfAtt, dimOfFeat);
    B = cv::Mat1d::zeros(1, dimOfAtt);
}

AttriModel :: AttriModel(const AttriModel &am){
    std::cout<<"called"<<std::endl;
    W = am.W.clone();
    B = am.W.clone();
}

void AttriModel :: update(const AttriModel1D &am){
    int dimNum = am.dim;
    if (dimNum < 0 || dimNum >= W.rows){
         throw std::runtime_error("AttriModel :: update : unexpect dimension of model.");
    }
    am.W.copyTo(W.row(dimNum));
    B(0, dimNum) = am.B;
} 

void AttriModel :: dump() const{
    std::string fn = filePathManager->attriModels;
    cv::FileStorage fs(fn, cv::FileStorage::WRITE);
    fs << "W" << W;
    fs << "B" << B;
    fs.release();
}

void AttriModel :: dumpOnline() const{
    std::string fn = filePathManager->attriModelsOnline;
    cv::FileStorage fs(fn, cv::FileStorage::WRITE);
    fs << "W" << W;
    fs << "B" << B;
    fs.release();
}

void AttriModel :: reload(){
    std::string fn = filePathManager->attriModels;
    cv::FileStorage fs(fn, cv::FileStorage::READ);
    if (!fs.isOpened())
    {
        throw std::runtime_error("AttriModel :: reload : can not open file");
    }
    fs["W"] >> W;
    fs["B"] >> B;
    fs.release();
}

}


