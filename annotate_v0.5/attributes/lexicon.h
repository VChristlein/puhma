#pragma once

#include <opencv2/imgproc/imgproc.hpp>

#include <util.h>

namespace puhma {
// singleton pattern here.
class Lexicon {
public:
    virtual ~Lexicon (){book.clear();}

    static Lexicon *get(const AttriConfig &c, const FileHandler *fpm)
    {
        static Lexicon instance(c, fpm);
        return &instance;
    }
    ///insert one word and its phoc rep. into lexicon
    void addWord(std::string word, std::string fn, int cls);

    int getBookSize(){
        return book.size();
    }

    void getPhocOfBook(cv::Mat &phocs, std::vector<std::string> &words);

    ///dump the lexicon
    void dump();
    //TODO:remove one word? 
private:
    const AttriConfig config;
    const FileHandler *filePathManager;
    Lexicon (const AttriConfig &c, const FileHandler *fpm):
        filePathManager(fpm), config(c){
        reload();
    };
    Lexicon (const Lexicon &){};
    Lexicon &operator=(const Lexicon &){return *this;}
    ///<word, <classnumber, filename>>
    std::map<std::string, std::pair<int, std::string> > book;
    ///reload lexicon
    void reload();
};

} /* puhma */
