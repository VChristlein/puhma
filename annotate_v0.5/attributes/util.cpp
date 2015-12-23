#include <random>
#include <map>
#include <iterator>
#include <fstream>
#include <iterator>
#include <cmath>

#include <boost/algorithm/string.hpp>
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QStringList>
#include <QRegExp>
#include <QTextStream>
#include <QDebug>

#include <opencv2/imgproc/imgproc.hpp>


#include "util.h"
#include "fisher.h"
#include "trans_phoc.h"

namespace puhma{

/*--------------------Util-------------------------*/

void Util :: nanTo0f(cv::Mat &m){
    for(int y=0; y< m.rows; ++y){
        float *onerow = m.ptr<float>(y);
        for(int x=0; x<m.cols; ++x){
            if(std::isnan(onerow[x])){
                onerow[x] = 0;
            }
        }
    }
}

void Util :: nanTo0d(cv::Mat &m){
    for(int y=0; y< m.rows; ++y){
        double *onerow = m.ptr<double>(y);
        for(int x=0; x<m.cols; ++x){
            if(std::isnan(onerow[x])){
                onerow[x] = 0;
            }
        }
    }
}

cv::Mat1d Util :: meanCenterPerCol(cv::Mat &s){
    if(s.channels()!=1)
        throw std::runtime_error("Util :: meanCenterPerCol : only 1 channel matrix is supported.");
    if(s.type()!=CV_64F)
         throw std::runtime_error("Util :: meanCenterPerCol : only double type supported.");
    cv::Mat1d re = cv::Mat1d::zeros(1, s.cols);
    for(int x = 0; x < s.cols; ++x ){
        double meanOfCol = cv::mean(s.col(x).clone())[0];
        cv::Mat temp = s.col(x).clone()-meanOfCol;
        temp.copyTo(s.col(x));
        re(0,x)= meanOfCol;
    }
    return re;
}

void Util :: meanCenterPerCol(cv::Mat &s, const cv::Mat &mean){
    if(s.channels()!=1 && mean.channels()!=1)
        throw std::runtime_error("Util :: meanCenterPerCol : only 1 channel matrix is supported.");
    if(s.type()!=CV_64F || mean.type()!=CV_64F)
        throw std::runtime_error("Util :: meanCenterPerCol : only double type supported.");
    CV_Assert(s.cols == mean.cols);
    CV_Assert(mean.rows==1);
    for(int x = 0; x < s.cols; ++x ){
        double meanOfCol = mean.at<double>(0,x);
        s.col(x) = s.col(x)-meanOfCol;
    }
}

void Util :: normalizePerRow(cv::Mat &matToNor){
    //L2 normalize
    if(matToNor.channels()!=1)
        throw std::runtime_error("Util :: meanCenterPerCol : only 1 channel matrix is supported.");
    if(matToNor.type()!=CV_64F)
        throw std::runtime_error("Util :: meanCenterPerCol : only double type supported.");
    for(int y = 0; y < matToNor.rows; ++ y){
        double normOfRow = cv::norm(matToNor.row(y).clone());
        cv::Mat temp = (matToNor.row(y).clone())/normOfRow;
        temp.copyTo(matToNor.row(y));
    }
    nanTo0d(matToNor);
}

void Util :: dumpVector(const std::string path, const std::vector<double> vec){
    std::ofstream output_file;
    output_file.open(path);
    std::copy(vec.begin(), vec.end(), std::ostream_iterator<double>(output_file, "\n"));
    output_file.close();

}

std::vector<int> Util :: generateRan(const int l, const int s){
    std::vector<int> r;
    std::vector<int> t;
    for(int i=0; i<s; ++i){
        t.push_back(i);
    }
    //uint32_t seed_val;
    std::random_device rd;
    std::mt19937 rng(rd());
    std::uniform_int_distribution<int> dis(0,s-l-1);
    //rng.seed(seed_val);
    for(int i=0; i<l; ++i){
        std::swap(t[s-i-1],t[dis(rng)]);
    }
    for(int i=0; i<l; ++i){
        r.push_back(t[s-i-1]);
    }
    return r;
}

void Util :: generateRan(std::vector<int> &vec){
    std::random_device rd;
    std::mt19937 rng(rd());
    std::uniform_int_distribution<int> dis(0,vec.size()-1);
    for(int i=0; i<vec.size(); ++i){
        std::iter_swap(vec.begin()+i, vec.begin()+dis(rng));
    }
}

/*--------------------DataPrepare-------------------------*/

DataPrepare :: DataPrepare(const AttriConfig &c, 
        const FileHandler *fpm) 
    : config(c),filePathManager(fpm){}

DataPrepare :: ~DataPrepare(){}

void DataPrepare :: prepare()
{
    qDebug()<<"Preparing Dataset...";
    if(!filePathManager->dirExist("out")){
        if(!QDir(QString::fromStdString(filePathManager->outPath)).mkpath(".")){
            throw std::runtime_error("DataPrepare::prepare error: can not create Dir.");
        }
    }
    if(filePathManager->fileExist("classes")&&filePathManager->fileExist("data")){
        qDebug()<<"GT already parsed, reloading informations...";
        restoreData();
        restoreClasses();
    }else{
        //create a sub folder that name as input folder.
                if(config.dataset == "IAM"){
            if(config.transcriptionfile == ""){
                throw std::runtime_error("Util_DataPrepare:: prepare : GT file not be provided.");
            }
            QFile file(QString::fromStdString(config.transcriptionfile));

            if(!file.open(QIODevice::ReadOnly | QIODevice::Text)) {  
                throw std::runtime_error("Util_DataPrepare:: prepare : can not open file groundtruth.");
            } 
            QTextStream in(&file);  
            QStringList lineList;
            while( !in.atEnd()){  
                QString line = in.readLine();
                lineList.append(line);
            }
            QRegExp rxf("^[a-z][^ ]+[ ]ok");
            lineList = lineList.filter(rxf);
            QRegExp rxs("[ ]");// match a comma or a space
            data.clear();
            //prepare the map
            classes.clear();
            int class_count = 0;

            foreach(QString entry, lineList){
                QStringList line = entry.split(rxs, QString::SkipEmptyParts);
                for(int i=9; i< line.size(); ++i){
                    line[8].append(line[i]);
                }
                QRegExp rxw("[a-zA-Z]+");
                if(config.digitalInPHOC){
                    rxw = QRegExp("[a-zA-Z0-9]+");
                }
                if(rxw.indexIn(line[8]) != -1) {
                    //filter out images with too small width.
                    int ckwidth = line[5].toInt();
                    int ckheight = line[6].toInt();
                    if((ckwidth*(config.heightIm)/ckheight)<15){
                        //qDebug()<<line[0];
                        continue;
                    }

                    //use '-' replace the unused characters
                    QRegExp rxi("[^a-zA-Z]");
                    if(config.digitalInPHOC){
                        rxi = QRegExp("[^a-zA-Z0-9]");
                    }
                    line[8].replace(rxi, "-");
                    std::string gtst = line[8].toStdString();
                    boost::to_upper(gtst);
                    line[8] = QString::fromStdString(gtst);

                    // assign the classnumber
                    if(classes.find(line[8].toStdString())==classes.end()){
                        classes.insert(std::pair<std::string, int>(line[8].toStdString(),class_count));
                        ++class_count;
                    }

                    std::vector<std::string> oword;
                    oword.push_back(line[0].toStdString()+".png");
                    oword.push_back(line[8].toStdString());
                    oword.push_back(line[2].toStdString());
                    oword.push_back(std::to_string(classes[line[8].toStdString()]));
                    data.push_back(oword);

                }
            }
            //write out the data
            //std::cout<<classes.size()<<std::endl;
            writeoutData();
            writeoutClasses();
        }else if(config.dataset == "PUHMA"||config.dataset == "USER"){
            if(config.inputfolder == ""){
                throw std::runtime_error("Util_DataPrepare:: prepare : input folder not provided.");
            }
            QDir inputF(QString::fromStdString(config.inputfolder)); 
            if(!inputF.exists())
            {
                throw std::runtime_error("Util_DataPrepare:: prepare : given inputfolder do not exist.");
            }
            inputF.setFilter(QDir::Files | QDir::NoSymLinks);
            QFileInfoList fileList = inputF.entryInfoList();

            int file_count = fileList.count();
            if(file_count <= 0)
            {
                throw std::runtime_error("Util_DataPrepare:: prepare : no file exists in the folder.");
            }

            data.clear();
            classes.clear();
            int class_count = 0;
            QRegExp rxs;
            if(config.dataset == "PUHMA")
                rxs.setPattern("_wort_");
            else
                rxs.setPattern("__");

            QStringList string_list;
            for(int i=0; i< file_count; ++i)
            {
                QFileInfo file_info = fileList.at(i);
                QString suffix = file_info.suffix();
                if(QString::compare(suffix, QString("png"), Qt::CaseInsensitive) == 0)   
                {   
                    QString basename = file_info.baseName();
                    QStringList info = basename.split(rxs, QString::SkipEmptyParts);
                    if(info.count()>=2&&info[1].length()>0){
                        QRegExp rxw("[a-zA-Z]+");
                        if(config.digitalInPHOC){
                            rxw = QRegExp("[a-zA-Z0-9]+");
                        }
                        if(rxw.indexIn(info[1]) != -1){
                            //filter out small file
                            /*cv::Mat img = cv::imread(file_info.filePath().toStdString());
                            if((img.cols*(config.heightIm)/img.rows)<15){
                                continue;
                            }*/

                            QRegExp rxi("[^a-zA-Z]");
                            if(config.digitalInPHOC){
                                rxi = QRegExp("[^a-zA-Z0-9]");
                            }
                            info[1].replace(rxi, "-");
                            std::string gtst = info[1].toStdString();
                            boost::to_upper(gtst);

                            // assign the classnumber
                            if(classes.find(gtst)==classes.end()){
                                classes.insert(std::pair<std::string, int>(gtst,class_count));
                                ++class_count;
                            }

                            std::vector<std::string> oword;
                            oword.push_back(file_info.fileName().toStdString());
                            oword.push_back(gtst);
                            oword.push_back("-1");
                            oword.push_back(std::to_string(classes[gtst]));
                            data.push_back(oword);
                        }
                    }
                }
            }
            //write out the data
            std::cout<<classes.size()<<std::endl;
            writeoutData();
            writeoutClasses();
            qDebug()<<"Done.";
        }else{
            throw std::runtime_error("Util_DataPrepare::dataset_headless: unknown dataset.");
        }
    }
    qDebug()<<"Done. Find "<<data.size()<<" instances.";
}

void DataPrepare :: prepareDivision(){
    if(data.size()<=0)
        throw std::runtime_error("DataPrepare :: prepareDivision : call prepare first.");
    idxTrain.clear();
    idxTest.clear();
    idxValidation.clear();

    if(filePathManager->fileExist("division")){
        qDebug()<<"Data have already been divided, reloading...";
        restoreDivision(filePathManager->trainSetPath, idxTrain);
        restoreDivision(filePathManager->testSetPath, idxTest);
        restoreDivision(filePathManager->validationSetPath, idxValidation);
        restoreDivision(filePathManager->onlineSetPath,idxOnline);
        
    }else{
        qDebug()<<"Dividing dataset into 4 sub-dataset...";
        //if divisionfile not given, create division randomly
        if(config.divisionTrainfile == ""||config.divisionTestfile == ""||config.divisionValifile == ""){
            qDebug()<<"division index file not find, divide randomly...";
            std::vector<int> t(data.size()), r;
            std::iota(t.begin(),t.end(),0);

            int blockSize = static_cast<int>(std::floor(data.size()*0.4));

            //train
            idxTrain = segVec(blockSize, t);

            //validation
            blockSize = blockSize/2;
            idxValidation = segVec(blockSize, t);
            idxOnline = segVec(blockSize, t);

            idxTest = t;
        }
        //if division files are given, create division accordingly.
        else{
            std::vector<std::string> setTrain = readDivisionIdxFile(config.divisionTrainfile);
            std::vector<std::string> setTest = readDivisionIdxFile(config.divisionTestfile);
            std::vector<std::string> setVali = readDivisionIdxFile(config.divisionValifile);
            std::vector<std::vector<std::string> > setsOfD {setTrain,
                setTest,
                setVali};
            std::vector<std::vector<int> *> setsOfId {&idxTrain,
                &idxTest,
                &idxValidation
            };

            for(int i=0; i<data.size(); ++i){
                bool found = false;
                std::string ifilename = data[i][0];
                for(int j=0; j<setsOfD.size(); ++j){
                    found = fileInSet(ifilename, setsOfD[j]);
                    if(found){
                        (*setsOfId[j]).push_back(i);
                        break;
                    }
                }
                //sample not find in division file, add to online set.
                if(!found){
                    idxOnline.push_back(i);
                }
            }
            //std::cout<< idxTrain.size()<<" "<<idxTest.size()<<" "<<idxValidation.size()<<" "<<idxOnline.size()<<std::endl;
        }

        writeoutDivision(filePathManager->trainSetPath, idxTrain);
        writeoutDivision(filePathManager->testSetPath, idxTest);
        writeoutDivision(filePathManager->validationSetPath, idxValidation);
        writeoutDivision(filePathManager->onlineSetPath, idxOnline);
    }
    //qDebug()<<idxTrain.size()+idxValidation.size()+idxTest.size()+idxOnline.size();
    qDebug()<<"Done.";
    qDebug()<<"Training set:"<<idxTrain.size()<<"instances.";
    qDebug()<<"Validation set:"<<idxValidation.size()<<"instances.";
    qDebug()<<"Test set"<<idxTest.size()<<"instances.";
    qDebug()<<"Online set"<<idxOnline.size()<<"instances.";
}

std::vector<std::string> DataPrepare :: readDivisionIdxFile(const std::string path){
    std::vector<std::string> re;
    //train
    QFile file(QString::fromStdString(path));
    if(!file.open(QIODevice::ReadOnly | QIODevice::Text)) {  
        throw std::runtime_error("DataPrepare :: restoreDivision : can not open file.");
    }
    QTextStream in(&file);  
    while( !in.atEnd()){  
        QString oline = in.readLine();
        oline = oline.trimmed();
        if(oline.length() > 0)
            re.push_back(oline.toStdString());
    }
    return re;
}

bool DataPrepare :: fileInSet(const std::string filename, const std::vector<std::string> set)
{
    bool re=false;
    for(std::string reg:set){
        size_t found = filename.find(reg);
        if(found != std::string::npos){
            re = true;
            break;
        }
    }
    return re;
}

std::vector<int> DataPrepare :: segVec(const int segSize,
        std::vector<int> &vec){
    Util :: generateRan(vec);
    std::vector<int> re(vec.begin(),vec.begin()+segSize);
    std::sort(re.begin(),re.end());
    vec.erase(vec.begin(),vec.begin()+segSize);
    std::sort(vec.begin(),vec.end());
    return re;
}

void DataPrepare :: writeoutDivision(std::string path,
        const std::vector<int> &idxList) const
{
    std::ofstream output_file;
    output_file.open(path);
    std::copy(idxList.begin(), idxList.end(), std::ostream_iterator<int>(output_file, ","));
    output_file.close();
    qDebug()<<"File: "<<QString::fromStdString(path)<<" has been written.";
}

void DataPrepare :: restoreDivision(const std::string path, 
        std::vector<int> &idxList){
    //QString filename;
    std::vector<int> temp;
    //train
    QFile file(QString::fromStdString(path));
    if(!file.open(QIODevice::ReadOnly | QIODevice::Text)) {  
        throw std::runtime_error("DataPrepare :: restoreDivision : can not open file.");
    }
    QTextStream in(&file);  
    QRegExp rxs("[,]");
    QString oline = in.readLine();
    QStringList idx = oline.split(rxs, QString::SkipEmptyParts);
    for(auto i:idx)
        temp.push_back(i.toInt());
    idxList = temp;
    file.close();
}


std::vector<std::vector<std::string> > DataPrepare :: getData() const{
    return data;
}

std::vector<std::vector<std::string> > DataPrepare :: getRandomData(int number) const{
    qDebug()<<"Randomly choosing image to train GMM...";
    auto tmp(data);
    std::vector<std::vector<std::string> > re;
    if(number > tmp.size()){
        number = tmp.size();
    }
    std::vector<int> t = Util :: generateRan(number, tmp.size());
    for(int ind:t){
        re.push_back(tmp[ind]);
    }
    qDebug()<<"Done.";
    return re;
}

std::vector<std::vector<std::string> > DataPrepare :: getDivision(std::string type) const
{
    std::vector<std::vector<std::string> > re;
    std::vector<int> target;
    if(type == "train")
        target = idxTrain;
    else if(type == "test")
        target = idxTest;
    else if(type == "vali")
        target = idxValidation;
    else if(type == "online")
        target = idxOnline;
    else
        throw std::runtime_error("DataPrepare :: getDivision : unsupported type.");

    for(int idx : target)
        re.push_back(data[idx]);

    return re;
}

void DataPrepare :: writeoutData(){
    //QString filename = assembleFileName("data");
    QString filename = QString :: fromStdString(filePathManager->dataPath);
    std::ofstream output_file(filename.toStdString());
    for(size_t i=0; i<data.size(); ++i){
        std::copy(data[i].begin(), data[i].end(), std::ostream_iterator<std::string>(output_file, ";"));
        output_file<<std::endl;
    }
    output_file.close();
    qDebug()<<"File: "<<filename<<" has been written.";
}

void DataPrepare :: restoreData(){
    data.clear();
    //QString filename = assembleFileName("data");
    QString filename = QString :: fromStdString(filePathManager->dataPath);
    QFile file(filename);
    if(!file.open(QIODevice::ReadOnly | QIODevice::Text)) {  
        throw std::runtime_error("Util_DataPrepare::restore_data_headless: can not open file.");
    }
    QTextStream in(&file);  
    QRegExp rxs("[;]");
    while( !in.atEnd()){  
        QString oline = in.readLine();
        QStringList line = oline.split(rxs, QString::SkipEmptyParts);
        /*if(line.size()!=3){
            qDebug()<<"called";
        }*/
        std::vector<std::string> oword;
        oword.push_back(line[0].toStdString());
        oword.push_back(line[1].toStdString());
        oword.push_back(line[2].toStdString());
        oword.push_back(line[3].toStdString());
        data.push_back(oword);
    }
}

void DataPrepare :: writeoutClasses(){
    //QString filename = assembleFileName("classes");
    QString filename = QString :: fromStdString(filePathManager->classesPath);
    std::ofstream output_file(filename.toStdString());
    for(auto it = classes.begin(); it != classes.end() ; ++it){
        output_file<<it->first;
        output_file<<";";
        output_file<<it->second;
        output_file<<std::endl;
    }
    output_file.close();
    qDebug()<<"File: "<<filename<<" has been written.";
}

void DataPrepare :: restoreClasses(){
    classes.clear();
    //QString filename = assembleFileName("classes");
    QString filename = QString :: fromStdString(filePathManager->classesPath);
    QFile file(filename);
    if(!file.open(QIODevice::ReadOnly | QIODevice::Text)) {  
        throw std::runtime_error("Util_DataPrepare::restore_classes_headless: can not open file.");
    }
    QTextStream in(&file);  
    QRegExp rxs("[;]");
    while( !in.atEnd()){  
        QString oline = in.readLine();
        QStringList line = oline.split(rxs, QString::SkipEmptyParts);
        /*if(line.size()!=2){
            qDebug()<<"called";
        }*/
        classes.insert(std::pair<std::string, int>(line[0].toStdString(),line[1].toInt()));
    }
}

cv::Mat DataPrepare :: prepareIm(const std::string path,
        const int gtThreshold,
        const int targetHeight){
    cv::Mat im = cv::imread(path);
    //convert to 1 channel;
    if(im.channels()==3){
        cv::cvtColor(im,im,CV_BGR2GRAY);
    }else if(im.channels()!=1){
        throw std::runtime_error("AttributeEmbed::channel number error: channel neither 3 nor 1");
    }
    //resize the img
    double scalarF = (double)targetHeight/(double)im.rows;
    int scaledW = static_cast<int>(scalarF*im.cols);
    cv::resize(im,im, cv::Size(scaledW, targetHeight));
    //convert to binary?
    /*if(gtThreshold!=-1)
        cv::threshold(im, im, gtThreshold, 255, cv::THRESH_BINARY | cv::THRESH_OTSU );*/
    return im;
}


void DataPrepare :: prepareIm(cv::Mat &oimg,
        //const int gtThreshold,
        const int targetHeight){
    //convert to 1 channel;
    if(oimg.channels()==3){
        cv::cvtColor(oimg,oimg,CV_BGR2GRAY);
    }else if(oimg.channels()!=1){
        throw std::runtime_error("AttributeEmbed::channel number error: channel neither 3 nor 1");
    }
    //resize the img
    double scalarF = (double)targetHeight/(double)oimg.rows;
    int scaledW = static_cast<int>(scalarF*oimg.cols);
    cv::resize(oimg,oimg, cv::Size(scaledW, targetHeight));
    //convert to binary?
    /*if(gtThreshold!=-1)
        cv::threshold(im, im, gtThreshold, 255, cv::THRESH_BINARY | cv::THRESH_OTSU );*/
}

/*----------------------FileHandler-----------------------*/
FileHandler :: FileHandler(const AttriConfig &c)
    : config(c)
{
    update();
}

void FileHandler :: update()
{
    std::string ndata = config.dataset;
    std::string ide1 = std::to_string(config.clusterGMM)+"_"+std::to_string(config.numSpatialX)+"_"+std::to_string(config.numSpatialY);
    boost::algorithm::to_upper(ndata);

    //subfolder for output
    outPath = config.outputdir + "/" + QDir(QString::fromStdString(config.inputfolder)).dirName().toStdString()+"_"+ndata;
    outPath = (QDir(QDir::cleanPath(QString::fromStdString(outPath))).path()).toStdString();

    //classesPath
    classesPath = "classes_"+ndata+(config.digitalInPHOC?"_digital":"")+".txt";
    classesPath = (QFileInfo(QDir(QString::fromStdString(outPath))
            , QString::fromStdString(classesPath)).filePath()).toStdString();
    
    //dataPath
    dataPath = "data_"+ndata+(config.digitalInPHOC?"_digital":"")+".txt";
    dataPath = (QFileInfo(QDir(QString::fromStdString(outPath))
            , QString::fromStdString(dataPath)).filePath()).toStdString();

    //pcaPath
    pcaPath = "pca_"+ndata+"_"+(std::to_string(config.dimPCA))+".yaml";
    pcaPath = (QFileInfo(QDir(QString::fromStdString(outPath))
            , QString::fromStdString(pcaPath)).filePath()).toStdString();

    //gmmPath
    gmmPath = "gmm_"+ndata+"_"+(std::to_string(config.dimPCA))+"_"+ide1+".yaml";
    gmmPath = (QFileInfo(QDir(QString::fromStdString(outPath))
            , QString::fromStdString(gmmPath)).filePath()).toStdString();

    //trainSetPath
    trainSetPath = "train_"+ndata+".txt";
    trainSetPath = (QFileInfo(QDir(QString::fromStdString(outPath))
            , QString::fromStdString(trainSetPath)).filePath()).toStdString();

    //testSetPath
    testSetPath = "test_"+ndata+".txt";
    testSetPath = (QFileInfo(QDir(QString::fromStdString(outPath))
            , QString::fromStdString(testSetPath)).filePath()).toStdString();

    //validationSetPath
    validationSetPath = "vali_"+ndata+".txt";
    validationSetPath = (QFileInfo(QDir(QString::fromStdString(outPath))
            , QString::fromStdString(validationSetPath)).filePath()).toStdString();

    //onlineSetPath
    onlineSetPath = "online_"+ndata+".txt";
    onlineSetPath = (QFileInfo(QDir(QString::fromStdString(outPath))
            , QString::fromStdString(onlineSetPath)).filePath()).toStdString();

    //File of score from TrainSet
    attRepresTrain = "attRepresTr_"+ndata+(config.discardThreshold>0?("_"+(std::to_string(config.discardThreshold*100))):"")+(config.doReduce?"_reduced":"")+".yaml";
    attRepresTrain = (QFileInfo(QDir(QString::fromStdString(outPath))
            , QString::fromStdString(attRepresTrain)).filePath()).toStdString();

    //file of score from validationset
    attRepresValidation = "attRepresVa_"+ndata+(config.discardThreshold>0?("_"+(std::to_string(config.discardThreshold*100))):"")+(config.doReduce?"_reduced":"")+".yaml";
    attRepresValidation = (QFileInfo(QDir(QString::fromStdString(outPath))
            , QString::fromStdString(attRepresValidation)).filePath()).toStdString();

    //file of score from testset
    attRepresTest = "attRepresTe_"+ndata+(config.discardThreshold>0?("_"+(std::to_string(config.discardThreshold*100))):"")+(config.doReduce?"_reduced":"")+".yaml";
    attRepresTest = (QFileInfo(QDir(QString::fromStdString(outPath))
            , QString::fromStdString(attRepresTest)).filePath()).toStdString();


    attRepresTrainOnline = "attRepresTr_"+ndata+(config.discardThreshold>0?("_"+(std::to_string(config.discardThreshold*100))):"")+(config.doReduce?"_reduced":"")+"_online.yaml";
    attRepresTrainOnline = (QFileInfo(QDir(QString::fromStdString(outPath))
            , QString::fromStdString(attRepresTrainOnline)).filePath()).toStdString();

    //file of score from validationset
    attRepresValidationOnline = "attRepresVa_"+ndata+(config.discardThreshold>0?("_"+(std::to_string(config.discardThreshold*100))):"")+(config.doReduce?"_reduced":"")+"_online.yaml";
    attRepresValidationOnline = (QFileInfo(QDir(QString::fromStdString(outPath))
            , QString::fromStdString(attRepresValidationOnline)).filePath()).toStdString();

    //file of score from testset
    attRepresTestOnline = "attRepresTe_"+ndata+(config.discardThreshold>0?("_"+(std::to_string(config.discardThreshold*100))):"")+(config.doReduce?"_reduced":"")+"_online.yaml";
    attRepresTestOnline = (QFileInfo(QDir(QString::fromStdString(outPath))
            , QString::fromStdString(attRepresTestOnline)).filePath()).toStdString();

    //file of attriModels
    attriModels = "attModel_"+ndata+(config.discardThreshold>0?("_"+(std::to_string(config.discardThreshold*100))):"")+(config.doReduce?"_reduced":"")+".yaml";
    attriModels = (QFileInfo(QDir(QString::fromStdString(outPath))
            , QString::fromStdString(attriModels)).filePath()).toStdString();

    //file of attriModels Online version
    attriModelsOnline = "attModel_"+ndata+(config.discardThreshold>0?("_"+(std::to_string(config.discardThreshold*100))):"")+(config.doReduce?"_reduced":"")+"_online.yaml";
    attriModelsOnline = (QFileInfo(QDir(QString::fromStdString(outPath))
            , QString::fromStdString(attriModelsOnline)).filePath()).toStdString();

    ///file of csrModel
    csrModelPath = "csrModel_"+ndata+(config.discardThreshold>0?("_"+(std::to_string(config.discardThreshold*100))):"")+(config.doReduce?"_reduced":"")+".yaml";
    csrModelPath = (QFileInfo(QDir(QString::fromStdString(outPath))
            , QString::fromStdString(csrModelPath)).filePath()).toStdString();

    //phocDir
    phocDir = outPath+"/"+"phoc_"+ndata+ (config.digitalInPHOC?"_digital":"");
    phocDir = (QDir(QDir::cleanPath(QString::fromStdString(phocDir))).path()).toStdString();

    //featsDir
    featsDir = outPath+"/"+"feature_"+ndata+"_"+(std::to_string(config.dimPCA))+"_"+ide1;
    featsDir = (QDir(QDir::cleanPath(QString::fromStdString(featsDir))).path()).toStdString();

    //attriModelDir
    attriModelDir = outPath+"/"+"am_"+ndata+"_"+ide1+"_"+(config.doWeight?"_weighted":"");
    attriModelDir = (QDir(QDir::cleanPath(QString::fromStdString(attriModelDir))).path()).toStdString();

    lexiconPath = "Lexicon_"+ndata+".txt";
    lexiconPath = (QFileInfo(QDir(QString::fromStdString(outPath))
            , QString::fromStdString(lexiconPath)).filePath()).toStdString();
}

FileHandler :: ~FileHandler(){}

bool FileHandler :: fileExist(std::string filetype) const{
    bool re=false;
    if(filetype == "classes"){
        re = QFileInfo(QString::fromStdString(classesPath)).exists();
    }else if(filetype == "data"){
        re = QFileInfo(QString::fromStdString(dataPath)).exists();
    }else if(filetype == "gmm"){
        re = QFileInfo(QString::fromStdString(gmmPath)).exists();
    }else if(filetype == "pca"){
        re = QFileInfo(QString::fromStdString(pcaPath)).exists();
    }else if(filetype == "division"){
        re = (QFileInfo(QString::fromStdString(trainSetPath)).exists())&&
            (QFileInfo(QString::fromStdString(testSetPath)).exists())&&
            (QFileInfo(QString::fromStdString(validationSetPath)).exists())&&
            (QFileInfo(QString::fromStdString(onlineSetPath)).exists());
    }else if(filetype == "attModel"){
        re = (QFileInfo(QString::fromStdString(attRepresTrain)).exists())&&
/*(QFileInfo(QString::fromStdString(attRepresValidation)).exists())&&*/ //discarded
            (QFileInfo(QString::fromStdString(attRepresTest)).exists())&&
            (QFileInfo(QString::fromStdString(attriModels)).exists());
    }else if(filetype == "csrModel"){
        re = QFileInfo(QString::fromStdString(csrModelPath)).exists();
    }else if(filetype == "lexicon"){
        re = QFileInfo(QString::fromStdString(lexiconPath)).exists();
    }
    else{
        throw std::runtime_error("Util_FileHandler::check_file_exists_headless: not support file type.");
    }
    return re;
}

bool FileHandler :: dirExist(std::string dirtype) const{
    bool re=false;
    if(dirtype == "phoc"){
        re = QDir(QString::fromStdString(phocDir)).exists();
    }else if(dirtype == "feature"){
        re = QDir(QString::fromStdString(featsDir)).exists();
    }else if(dirtype == "attrimodel"){
        re = QDir(QString::fromStdString(attriModelDir)).exists();
    }else if(dirtype == "out"){
        re = QDir(QString::fromStdString(outPath)).exists();
    }
    else{
        throw std::runtime_error("Util_FileHandler::check_dir_exists_headless: not support dir type.");
    }
    return re;
}

}
