#include "recog.h"

#include <qdebug.h>
#include <numeric>

#include "attrispace.h"

namespace puhma {

Recog :: Recog(const AttriConfig &c, const FileHandler *fpm, const DataPrepare *dp) : filePathManager(fpm), config(c){
    if(!(filePathManager->fileExist("csrModel"))){
        throw std::runtime_error("Recog :: Recog : CSR model do not exist. Run calibration with CSR first.");
    }else{
        qDebug()<<"restoring Learned Common Subspace...";
        embedding.restore(filePathManager);
        qDebug()<<"Done.";
    }
    lex = Lexicon::get(config, filePathManager);
    int old = lex->getBookSize();
    data = dp->getDivision("test");
    //add new instance from test dataset into lexicon 
    qDebug()<<"Adding new instances from Testset into Lexicon...";
    for(auto onedata : data){
        lex->addWord(onedata[1], onedata[0], std::stoi(onedata[3]));
    }
    lex->dump();
    qDebug()<<lex->getBookSize()-old<<" instances added.";
}

Recog :: Recog(const AttriConfig &c, const FileHandler *fpm) : filePathManager(fpm), config(c){
    if(!(filePathManager->fileExist("csrModel"))){
        throw std::runtime_error("Recog :: Recog : CSR model do not exist. Run calibration with CSR first.");
    }else{
        qDebug()<<"restoring Learned Common Subspace...";
        embedding.restore(filePathManager);
        qDebug()<<"Done.";
    }
    lex = Lexicon::get(config, filePathManager);
    qDebug()<<"Lexicon has "<<lex->getBookSize()<<"instances.";
}

std::vector<std::string> Recog :: getCandidates(cv::Mat attRep){
    std::vector<std::string> re;

    Util::normalizePerRow(attRep);
    Util::meanCenterPerCol(attRep, embedding.mAtt);
    cv::Mat1d attRep_CSR = embedding.Watt.rowRange(0,config.dimCCA)*(attRep.t());
    attRep_CSR = attRep_CSR.t();
    Util::normalizePerRow(attRep_CSR);

    //lexion phoc
    cv::Mat phocs;
    std::vector<std::string> words;
    lex->getPhocOfBook(phocs, words);
    cv::Mat1b msk = cv::Mat1b::zeros(phocs.size());
    phocs = phocs>msk;
    phocs = phocs/255;
    phocs.convertTo(phocs, CV_64F);
    phocs = phocs * 2.0 - 1.0;
    Util::normalizePerRow(phocs);
    Util::meanCenterPerCol(phocs, embedding.mPhoc);
    cv::Mat1d phocs_CSR = embedding.Wphoc.rowRange(0, config.dimCCA)*(phocs.t());
    phocs_CSR = phocs_CSR.t();
    Util::normalizePerRow(phocs_CSR);

    cv::Mat1d score = attRep_CSR * (phocs_CSR.t());

    cv::Mat1i scoreIdx(score.size());
    cv::sortIdx(score,scoreIdx,CV_SORT_EVERY_ROW+CV_SORT_DESCENDING);
    for(int c=0; c<5; ++c){
        //std::cout<<words[scoreIdx[c]]<<std::endl;
        re.push_back(words[scoreIdx(0,c)]);
    }
    return re;
}
    
void Recog :: evaluate(){
    qDebug()<<"Evaluating recognition performance...";
    cv::Mat1d attRep = AttriSpace::getRep("test", filePathManager);
    Util::normalizePerRow(attRep);
    Util::meanCenterPerCol(attRep, embedding.mAtt);
    cv::Mat1d attRep_CSR = embedding.Watt.rowRange(0,config.dimCCA)*(attRep.t());
    attRep_CSR = attRep_CSR.t();
    Util::normalizePerRow(attRep_CSR);

    //lexion phoc
    cv::Mat phocs;
    std::vector<std::string> words;
    lex->getPhocOfBook(phocs, words);
    cv::Mat1b msk = cv::Mat1b::zeros(phocs.size());
    phocs = phocs>msk;
    phocs = phocs/255;
    phocs.convertTo(phocs, CV_64F);
    phocs = phocs * 2.0 - 1.0;
    Util::normalizePerRow(phocs);
    Util::meanCenterPerCol(phocs, embedding.mPhoc);
    cv::Mat1d phocs_CSR = embedding.Wphoc.rowRange(0, config.dimCCA)*(phocs.t());
    phocs_CSR = phocs_CSR.t();
    Util::normalizePerRow(phocs_CSR);

    //TODO: filter out useless word.

    std::vector<int> p1full(data.size());
    std::vector<float> cerfull(data.size());

    for(size_t pos=0; pos< data.size(); ++pos){
        cv::Mat1d feat = attRep_CSR.row(pos).clone();
        std::string gt = data[pos][1];

        cv::Mat1d score = feat * (phocs_CSR.t());
        std::vector<int> rnum(score.cols);
        std::iota(std::begin(rnum), std::end(rnum), 0);
        Util::generateRan(rnum);
        cv::Mat1d rscore = cv::Mat1d::zeros(score.size());
        for(int c=0; c<rnum.size(); ++c){
            rscore(0,c) = score(0,rnum[c]);
        }
        cv::Mat1i scoreIdx(rscore.size());
        cv::sortIdx(rscore,scoreIdx,CV_SORT_EVERY_ROW+CV_SORT_DESCENDING);
        std::vector<int> I(rnum.size());
        for(int x=0; x<scoreIdx.cols; ++x){
            I[x] = rnum[scoreIdx(0,x)];
        }

        if (gt == words[I[0]])
            p1full[pos] = 1;
        else{
            p1full[pos] = 0;
            int leng_gt = gt.length();
            int leng_word = words[I[0]].length();
            int maxLeng = (leng_gt>=leng_word) ? leng_gt : leng_word;
            cerfull[pos] = static_cast<float>(levenshtein(gt, leng_gt, words[I[0]], leng_word))/static_cast<float>(maxLeng);
        }
    }

    float p1full_mean = std::accumulate(p1full.begin(),p1full.end(),0.0)/p1full.size()*100;
    float cerfull_mean = std::accumulate(cerfull.begin(), cerfull.end(),0.0)/cerfull.size()*100;
    qDebug()<<"lexicon   -- p@1:"<< p1full_mean <<". cer:"<< cerfull_mean <<".";
}

int Recog :: levenshtein(const std::string &gt, const int len_gt, 
        const std::string &re, const int len_re){
    /*std::vector<int> column(gt.length()+1);
    std::iota(std::begin(column), std::end(column), 0);

    for (int x = 1; x <= re.length(); ++x){
        column[0]= x;
        for (int y = 1, int lastdiag = x-1; y <= gt.length() ; ++y){
            olddiag = column[y];

        }
    }*/
    int cost;
    /* base case: empty strings */
    if (len_gt == 0) return len_re;
    if (len_re == 0) return len_gt;
 
    /* test if last characters of the strings match */
    if (gt[len_gt-1] == re[len_re-1])
        cost = 0;
    else
        cost = 1;
 
  /* return minimum of delete char from s, delete char from t, and delete char from both */
    return minimum(levenshtein(gt, len_gt - 1, re, len_re    ) + 1,
            levenshtein(gt, len_gt    , re, len_re - 1) + 1,
            levenshtein(gt, len_gt - 1, re, len_re - 1) + cost);
}
    
} /* puhma */
