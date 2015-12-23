#include "lexicon.h"
#include "trans_phoc.h"

#include <fstream>
#include <QFile>
#include <QTextStream>
#include <QRegExp>
#include <QStringList>

namespace puhma {
void Lexicon :: addWord(std::string word, std::string fn, int cls){
    if(word == "-") return;
    if(book.find(word)==book.end()){
        std::pair<int, std::string> info(cls, fn);
        book.insert(std::pair<std::string, std::pair<int, std::string> >(word, info));
    }
}

void Lexicon :: reload(){
    if(filePathManager->fileExist("lexicon")){
        QFile file(QString::fromStdString(filePathManager->lexiconPath));
        if(!file.open(QIODevice::ReadOnly | QIODevice::Text)) {  
            throw std::runtime_error("Lexicon :: reload : can not open file.");
        }
        QTextStream in(&file);
        QRegExp rxs("[ ]");
        while(!in.atEnd()){
            QString oneline = in.readLine();
            QStringList infos = oneline.split(rxs, QString::SkipEmptyParts);
            if(infos.length() == 3){
                std::pair<int, std::string> info(infos[2].toInt(), infos[1].toStdString());
                book.insert(std::pair<std::string, std::pair<int, std::string> >(infos[0].toStdString(), info));
            }
        }
    }
}

void Lexicon :: getPhocOfBook(cv::Mat &phocs, std::vector<std::string> &words){
    TransPhoc tp(config,filePathManager);
    int dim = tp.getSize().width;
    phocs = cv::Mat::zeros(book.size(), dim, CV_8U);
    words.clear();
    for(auto page: book){
        words.push_back(page.first);
        tp.fetchOnePhocFromFile(page.second.second).copyTo(phocs.row(words.size()-1));
    }
}

void Lexicon :: dump(){
    std::ofstream output_file;
    output_file.open(filePathManager->lexiconPath);
    for(auto const &word : book){
        output_file<<word.first;
        output_file<<" "+word.second.second+" ";
        output_file<<word.second.first;
        output_file<<"\n";
    }
    output_file.close();
}

} /* puhma */
