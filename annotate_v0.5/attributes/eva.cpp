#include "eva.h"

#include <limits>
#include <iostream>
#include <numeric>
#include <QFile>
#include <QStringList>
#include <QRegExp>
#include <QTextStream>


namespace puhma{

void Evaluate :: computemAP(const AttriConfig &config,
        const cv::Mat &queries,
        const cv::Mat &dataset,
        const cv::Mat &allClasses,
        const std::vector<std::string> &queriesWords,
        cv::Mat1d &mAP,
        cv::Mat1d &p1,
        const bool doqbs){
    cv::Mat1f allClassesToHist;
    allClasses.convertTo(allClassesToHist, CV_32F);

    double max;

    cv::minMaxLoc(allClasses, NULL, &max);
    int histSize = static_cast<int>(max+1);
    int histSizes[] = {histSize};
    float range[] = {0, static_cast<float>(histSize)};
    const float *ranges[] = {range};
    int channels[] = {0};
    //std::cout<<"max: "<<max<<std::endl;

    cv::MatND hist;

    cv::calcHist( &allClassesToHist, 1 , channels, cv::Mat(), hist, 1, histSizes, ranges, true, false);
    cv::Mat1f nRel= cv::Mat1f::zeros(allClasses.rows, 1);
    for(int y=0; y< allClasses.rows; ++y){
        nRel(y,0) = hist.at<float>(allClasses.at<int>(y,0),0);
    }

    std::vector<int> keep(nRel.rows);
    std::iota(keep.begin(), keep.end(), 0);

    int thresholdRel = 1;
    if(!doqbs) thresholdRel = 2;
    for(auto it = keep.begin(); it != keep.end();){
        if(nRel(*it,0)<thresholdRel)
            it = keep.erase(it);
        else
            ++it;
    }
    //If QBS, keep only one instance per query
    if(doqbs){
        std::map<int, int> clsMap;
        std::vector<int> keep2;
        for(int keepidx: keep){
            if(clsMap.find(allClasses.at<int>(keepidx,0))==clsMap.end()){
                clsMap.insert(std::pair<int,int>(allClasses.at<int>(keepidx,0),1));
                keep2.push_back(keepidx);
            }
        }
        keep = keep2;
    }
    
    //do stopword.
    if(config.sw != ""){
        QFile file(QString::fromStdString(config.sw));
        if(!file.open(QIODevice::ReadOnly | QIODevice::Text)) {  
            throw std::runtime_error("Evaluate:: computemAP : can not open file of stopwords.");
        }
        QTextStream in(&file);
        QRegExp rxs("[,]");
        std::vector<std::string> swords;
        while(!in.atEnd()){
            QString line = in.readLine();
            QStringList words = line.split(rxs, QString::SkipEmptyParts);
            for(QString word: words){
                swords.push_back(word.toStdString());
            }
        }
        for(auto it = keep.begin(); it != keep.end();){
            std::string query = queriesWords[*it];
            if(std::find(swords.begin(),swords.end(),query)!=swords.end()){
                it = keep.erase(it);
            }else{
                ++it;
            }
        }
    }

    //std::cout<<keep.size()<<std::endl;
    /*for(auto idx:keep){
        std::cout<< nRel(idx,0) <<" ";
    }
    std::cout<< std::endl;*/

    cv::Mat queriesE = cv::Mat::zeros(keep.size(),queries.cols,queries.type());
    cv::Mat queriesClass = cv::Mat::zeros(keep.size(),allClasses.cols,allClasses.type());

    cv::Mat nRelevantsPerQuery = cv::Mat::zeros(keep.size(),1, CV_32F);

    //Compute the number of relevants for each query
    for(int y=0; y< keep.size();++y){
        int idx = keep[y];
        queries.row(idx).copyTo(queriesE.row(y));
        allClasses.row(idx).copyTo(queriesClass.row(y));

        if(doqbs){
            nRelevantsPerQuery.at<float>(y,0) = nRel(idx,0);
        }else{
            nRelevantsPerQuery.at<float>(y,0) = nRel(idx,0)-1;
        }
    }
    
    nRelevantsPerQuery.convertTo(nRelevantsPerQuery, CV_32S);

    //compute score.
    cv::Mat score = queriesE * (dataset.t());
    if(!doqbs){
        computeState(score, queriesClass, allClasses, nRelevantsPerQuery, keep, mAP, p1);
        //computeState(queriesE, dataset, queriesClass, allClasses, nRelevantsPerQuery, keep, mAP, p1);
    }else{
        std::vector<int> nkeep(keep.size(), -1);
        computeState(score, queriesClass, allClasses, nRelevantsPerQuery, nkeep, mAP, p1);
        //std::cout<<"qbs start"<<std::endl;
        //computeState(queriesE, dataset, queriesClass, allClasses, nRelevantsPerQuery, nkeep, mAP, p1);
    }
}

int Evaluate :: computemAP2(const AttriConfig &config,
        const int idx,
        const cv::Mat &query,
        const cv::Mat &dataset,
        const cv::Mat &allClasses,
        const std::vector<std::string> &queriesWords,
        const std::vector<std::string> &swords,
        cv::Mat1d &mAP,
        cv::Mat1d &p1,
        const bool doqbs){
    cv::Mat1f allClassesToHist;
    allClasses.convertTo(allClassesToHist, CV_32F);

    double max;

    cv::minMaxLoc(allClasses, NULL, &max);
    int histSize = static_cast<int>(max+1);
    int histSizes[] = {histSize};
    float range[] = {0, static_cast<float>(histSize)};
    const float *ranges[] = {range};
    int channels[] = {0};
    //std::cout<<"max: "<<max<<std::endl;

    cv::MatND hist;

    cv::calcHist( &allClassesToHist, 1 , channels, cv::Mat(), hist, 1, histSizes, ranges, true, false);
    //cv::Mat1f nRel= cv::Mat1f::zeros(allClasses.rows, 1);
    /*for(int y=0; y< allClasses.rows; ++y){
        nRel(y,0) = hist.at<float>(allClasses.at<int>(y,0),0);
    }*/

    int thresholdRel = 1;
    if(!doqbs) thresholdRel = 2;
    if(hist.at<float>(allClasses.at<int>(idx,0),0)<thresholdRel)
        return 0;

    //do stopword.
    if(swords.size()>0){
        std::string queryWord = queriesWords[idx];
        if(std::find(swords.begin(),swords.end(),queryWord)!=swords.end()){
            return -1;
        }
    }

    cv::Mat queryE = cv::Mat::zeros(1,query.cols,query.type());
    cv::Mat queryClass = cv::Mat::zeros(1,allClasses.cols,allClasses.type());

    cv::Mat nRelevantsPerQuery = cv::Mat::zeros(1, 1, CV_32F);

    queryE = query.clone();
    queryClass = allClasses.row(idx).clone();
    if(doqbs){
        nRelevantsPerQuery.at<float>(0,0) = hist.at<float>(allClasses.at<int>(idx,0),0);
    }else{
        nRelevantsPerQuery.at<float>(0,0) = hist.at<float>(allClasses.at<int>(idx,0),0)-1;
    }
    
    nRelevantsPerQuery.convertTo(nRelevantsPerQuery, CV_32S);

    //compute score.
    cv::Mat score = queryE * (dataset.t());
    if(!doqbs){
        std::vector<int> keep{idx};
        computeState(score, queryClass, allClasses, nRelevantsPerQuery, keep, mAP, p1);
        //computeState(queriesE, dataset, queriesClass, allClasses, nRelevantsPerQuery, keep, mAP, p1);
    }else{
        std::vector<int> nkeep{-1};
        computeState(score, queryClass, allClasses, nRelevantsPerQuery, nkeep, mAP, p1);
        //std::cout<<"qbs start"<<std::endl;
        //computeState(queriesE, dataset, queriesClass, allClasses, nRelevantsPerQuery, nkeep, mAP, p1);
    }
    return 1;
}

///This part use almost the same code as almazan. only matlab part changed.
void Evaluate :: computeState(const cv::Mat &score, //
            const cv::Mat &queriesClass,
            const cv::Mat &allClasses,
            const cv::Mat &nRelevantsPerQuery,
            const std::vector<int> &keep,
            cv::Mat1d &pMap,
            cv::Mat1d &pP1){
    int nQueries = score.rows; //how many queries in full dataset
    int nDataset = score.cols; //how many data we have.

    pMap = cv::Mat1d::zeros(nQueries, 1);
    pP1 = cv::Mat1d::zeros(nQueries, 1);
    cv::Mat bestIdx = cv::Mat::zeros(nQueries, 1, CV_32S); 
    for(int i=0; i< nQueries; ++i){
        std::vector<int> rank(nRelevantsPerQuery.at<int>(i,0));
        int nRelevants = 0;
        //
        int qclass = queriesClass.at<int>(i,0);

        double bestS = -99999;
        //int bestJ = -1;
        int p1 = 0;
        for (int j=0; j < nDataset; ++j)
        {
            double s = score.at<double>(i,j);
            if(keep[i] != j && s > bestS){
                bestS = s;
                ////bestJ = j;
                p1 = (allClasses.at<int>(j,0) == qclass?1:0);
                bestIdx.at<int>(i,0) = j;
            }
            // If it is from the same class and it is not the query idx, it is a relevant one. 
            // Compute how many on the dataset get a better score and how many get an equal one, excluding itself and the query.
            if(allClasses.at<int>(j,0) == qclass
                    && keep[i]!=j)
            {
                int better = 0;
                int equal = 0;

                for (int k=0; k < nDataset; ++k){
                    if(k!=j && keep[i] !=k){
                        double s2 = score.at<double>(i,k);
                        if(s2>s) ++better;
                        else if(s2 == s) ++equal;
                    }
                }

                rank[nRelevants] = better+
                    static_cast<int>(std::floor(equal/2.0));
                ++nRelevants;
            }
        }
        //std::cout<<"bs:"<<allClasses.at<int>(bestJ,0)<<" "<<allClasses.at<int>(keep[i],0)<<std::endl;

        std::sort(rank.begin(), rank.end());
        //rank.resize(nRelevantsPerQuery.at<int>(i,0));
        pP1(i,0) = p1;

        //get mAp
        for(int j =0; j <rank.size(); ++j){
            double prec_at_k = static_cast<double>(j+1)/static_cast<double>(rank[j]+1);
            pMap(i,0) += prec_at_k;
        }
        pMap(i,0) = pMap(i,0)/rank.size();
    }
}

void Evaluate :: computePhocClassNumber(
        const cv::Mat &rep,
        const cv::Mat &phocs,
        const cv::Mat &labels){
    cv::Mat p = phocs.clone();
    //filtered version
    /*cv::Mat redrep;
    cv::reduce(rep, redrep, 0, CV_REDUCE_SUM);
    int cc = 0;
    for(int x=0; x<rep.cols; ++x){
        if(redrep.at<double>(0,x) == 0){
            p.col(x) = p.col(x)*0;
            //std::cout<<p.col(x)<<std::endl;
            cc++;
        }
    }
    std::cout<< cc<< std::endl;*/
    cv::Mat p2 = cv::Mat::zeros(p.size(),p.type());
    cv::Mat p3 = p2.colRange(52,p2.cols)+1;
    p3.copyTo(p2.colRange(52,p2.cols));
    p = p.mul(p2);
    p2.release();
    p3.release();
    int count = 0;

    std::map<int, int> clsMap;
    for(int y=0; y<labels.rows; ++y){
        if(clsMap.find(labels.at<int>(y,0))==clsMap.end())
            clsMap.insert(std::pair<int,int>(labels.at<int>(y,0),1));
    }
    int numOfClass = clsMap.size();
    clsMap.clear();

    cv::Mat temp;
    cv::reduce(p,temp, 1, CV_REDUCE_SUM, CV_32S);
    for(int y=0; y<temp.rows; ++y){
        if(temp.at<int>(y,0)==0){
            if(clsMap.find(labels.at<int>(y,0))==clsMap.end() && clsMap.size()!=0){
                count++;
                clsMap.insert(std::pair<int,int>(labels.at<int>(y,0),1));
            }else if(clsMap.find(labels.at<int>(y,0))==clsMap.end() && clsMap.size()==0){
                clsMap.insert(std::pair<int,int>(labels.at<int>(y,0),1));
            }
        }
    }
    temp.release();

    for(int y=0; y<p.rows-1; ++y){
        if(cv::sum(p.row(y))[0]!=0 && clsMap.find(labels.at<int>(y,0))==clsMap.end()){
            for(int yt = y+1; yt<p.rows; ++yt){
                cv::Mat oneRow;
                cv::bitwise_xor(p.row(y),p.row(yt),oneRow);
                if(cv::sum(oneRow)[0]==0){
                    if(labels.at<int>(y,0) != labels.at<int>(yt,0)&&clsMap.find(labels.at<int>(yt,0))==clsMap.end()){
                        count++;
                        clsMap.insert(std::pair<int,int>(labels.at<int>(yt,0),1));
                    }
                }
            }
            clsMap.insert(std::pair<int,int>(labels.at<int>(y,0),1));
        }
    }

    std::cout<<"class number:"<<numOfClass<<". class abondoned "<<count<<std::endl;
    
    
}

}

