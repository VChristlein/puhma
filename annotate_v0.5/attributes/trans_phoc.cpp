#include "trans_phoc.h"
#include "puhma_common.h"

#include <opencv2/imgproc/imgproc.hpp>

#include <boost/algorithm/string.hpp>

#include <iostream>
#include <QString>
#include <QDebug>
#include <QFileInfo>
#include <QFile>
#include <QDir>

namespace puhma {

TransPhoc :: TransPhoc(const AttriConfig &c
        ,const FileHandler *fpm) 
    : config(c), filePathManager(fpm) {
        bgrams = "er in es ti te at on an en st al re is ed le ra ri li ar ng ne ic or nt ss ro la se de co ca ta io it si us ea ac el ma na ni tr ch di ia et to un ns ll ec me lo sc ol as he ly ce nd il pe sa mi rs ve ou th sp ur om ha sh nc";
        bigramnum = 50;
        charInterval = 26;
        if(config.digitalInPHOC){
            charInterval = 36;
        }
        sizeOfPHOC = cv::Size((2+3+4+5)*charInterval+bigramnum*2,1);
        labels4Dimension = std::vector<std::string>(sizeOfPHOC.width,"-");
        for(int i=0; i<labels4Dimension.size()-bigramnum*2; ++i){
            int shift = i%charInterval;
            char labelc = 'A'+shift;
            if(config.digitalInPHOC && shift >= 26){
                labelc = '0'+(shift-26);
            }
            std::string label({labelc});
            labels4Dimension[i] = label;
        }
};

TransPhoc :: ~TransPhoc() {};

void TransPhoc :: compute(const std::vector<std::vector<std::string> > &data)
{
    //get Dir
    std::string dirname = filePathManager->phocDir;
    //mkdir if dir not exists
    if(!filePathManager->dirExist("phoc")){
        if(!QDir(QString::fromStdString(dirname)).mkpath(".")){
            throw std::runtime_error("TransPhoc::getPhocs error: can not create Dir.");
        }
    }

    qDebug()<<"Processing PHOC..."; 
    for(auto datum: data){
        std::string fn = datum[0]+".yaml";
        if(!(QFileInfo(QDir(QString::fromStdString(dirname))
                ,QString::fromStdString(fn)).exists())){
            //std::cout<<datum[1]<<std::endl;
            cv::Mat1b phoc = getOnePhoc(datum[1]);
            writeoutOnePhoc(phoc,dirname,datum[0]);
        }
    }
    qDebug()<<"Done.";
}

cv::Mat1b TransPhoc :: getOnePhoc(const std::string _trans){
    //init the 1*604 dim vector
    cv::Mat1b re = cv::Mat1b::zeros(sizeOfPHOC);

    std::string trans = _trans;

    boost::algorithm::to_lower(trans);
    
    for(size_t i=0; i< trans.size(); ++i){
        char p = trans.at(i);
        int code = -1;
        /*if(p>='0' && p<='9'){
            code = p-'0';
        }else */
        if(p>='a' && p<='z'){
            code = (p-'a');
        }else if(config.digitalInPHOC){
            if(p>='0' && p<='9'){
                code = 26+(p-'0');
            }
        }
        if(code>=0 && code<charInterval){
            //test
            //std::cout<<p<<std::endl;
            //std::cout<<"pos:"<<i<<" code:"<<code<<std::endl;
            signThePhoc(code, trans.size(), i, re);
        }
    }
    
    for(size_t i=0; i< trans.size()-1; ++i){
        std::string pb = trans.substr(i,2);
        std::string::size_type n;
        n = bgrams.find(pb);
        if (n != std::string::npos && n%3 == 0 && n/3 < bigramnum) {
            //test
            //std::cout<<pb<<std::endl;
            signThePhoc(n/3, trans.size(), i, re, true); 
        } 
    }
    //std::cout<<re<<std::endl;

    //return the vector
    return re;
}

void TransPhoc :: signThePhoc(const int c,
        const int l,
        const int p,
        cv::Mat1b &ph,
        const bool bigram)
{
    CV_Assert(l != 0);
    int step = 1;
    if(bigram) step = 2;
    double occlowboundwhole = p*1.0/l;
    double occupperboundwhole = (p+step)*1.0/l;
    //level 2 to level 5
    int levelbound = 6;
    if(bigram) levelbound = 3;

    for(int level=2; level<levelbound; ++level){
        for(int region=0; region<level; ++region){
            double occlowbound = -1.0;
            double occupperbound = -1.0;
            double occlowboundregion = region*1.0/level;
            double occupperboundregion = (region+1)*1.0/level;
            if(occlowboundwhole <= occlowboundregion && 
                    occupperboundwhole >= occlowboundregion)
                occlowbound = occlowboundregion;
            else if(occlowboundwhole >= occlowboundregion &&
                    occupperboundregion >= occlowboundwhole)
                occlowbound = occlowboundwhole;
            if(occupperboundregion <= occupperboundwhole &&
                    occlowboundwhole <= occupperboundregion)
                occupperbound = occupperboundregion;
            else if(occupperboundregion >= occupperboundwhole &&
                    occupperboundwhole >= occlowboundregion)
                occupperbound = occupperboundwhole;
            if(occlowbound >= 0 && occupperbound >= 0 &&
                    occlowbound <= occupperbound){
                double occ = (occupperbound-occlowbound)/(occupperboundwhole-occlowboundwhole);
                if(occ >= 0.5){
                    //compute the location
                    int loc = 0;
                    if(!bigram){
                        for(int lll = 2; lll<level; ++lll){
                            loc += lll;
                        }
                        loc = (loc+region)*charInterval+c;
                    }else if(bigram){
                        loc = (2+3+4+5)*charInterval + bigramnum*region + c;
                    }
                    ph(0,loc) = ph(0,loc)+1;
                    //test
                    //qDebug() << "level: "<< level <<" region:" << region <<" location:" << loc;
                }
            }
        }
    }
}

void TransPhoc :: writeoutOnePhoc(const cv::Mat1b &s,
        const std::string dir,
        const std::string dn) const
{
    QString filename;
    filename = QString::fromStdString(dn)+".yaml";
    filename = QFileInfo(QDir(QString::fromStdString(dir))
                , filename).filePath();
    cv::FileStorage fs(filename.toStdString(), cv::FileStorage::WRITE);
    fs << "phoc" << s;
    fs.release();
}

cv::Mat1b TransPhoc :: restoreOnePhoc(const std::string dir,
            const std::string dn){
    QString filename;
    cv::Mat1b re;
    filename = QString::fromStdString(dn)+".yaml";
    filename = QFileInfo(QDir(QString::fromStdString(dir))
                , filename).filePath();
    cv::FileStorage fs(filename.toStdString(), cv::FileStorage::READ);
    if (!fs.isOpened())
    {
        throw std::runtime_error("TransPhoc::restorePhoc_error: can not open file");
    }
    fs["phoc"]>>re;
    return re;
}

cv::Mat1b TransPhoc :: fetchOnePhocFromFile(const std::string fn) const
{
    cv::Mat1b re;
    QString filep = QString::fromStdString(fn)+".yaml";
    filep = QFileInfo(QDir(QString::fromStdString(filePathManager->phocDir)), filep).filePath();
    cv::FileStorage fs(filep.toStdString(), cv::FileStorage::READ);
    if (!fs.isOpened())
    {
        std::cout<<filep.toStdString()<<std::endl;
        throw std::runtime_error("Attri_Learn::fetchPhoc_error: can not open file");
    }
    fs["phoc"]>>re;
    fs.release();
    return re;
}


}
