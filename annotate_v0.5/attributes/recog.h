#pragma once

#include "util.h"
#include "attributes_model.h"
#include "lexicon.h"

namespace puhma {

class Recog {
public:
    Recog (const AttriConfig &c, const FileHandler *fpm, const DataPrepare *dp);
    Recog (const AttriConfig &c, const FileHandler *fpm);
    ~Recog (){}

    void evaluate();

    std::vector<std::string> getCandidates(cv::Mat attriRep);
private:
    const AttriConfig config;
    const FileHandler *filePathManager;
    Embedding embedding;
    Lexicon *lex;
    std::vector<std::vector<std::string> > data;

    int levenshtein(const std::string &gt, const int len_gt,
            const std::string &re, const int len_re);
    int minimum(const int a, const int b, const int c){
        if(a<b&&a<c)
            return a;
        else if(b<c)
            return b;
        else
            return c;
    }
};
    
} /* puhma */
