#pragma once

#include <opencv2/imgproc/imgproc.hpp>

#include "util.h"

namespace puhma{

class Embedding{
public:
    Embedding(){};
    ~Embedding(){};
    cv::Mat Watt;
    cv::Mat Wphoc;
    double reg;
    cv::Mat1d mAtt;
    cv::Mat1d mPhoc;
    void dump(const FileHandler *fpm);
    void restore(const FileHandler *fpm);
};

class AttriModel1D{
public:
    ///this constructor only can be called if Attributes model has already been extracted.
    AttriModel1D(int dim, const FileHandler *fpm);
    AttriModel1D(int dim, int dimFeat, int numOfData, const FileHandler *fpm);
    AttriModel1D(int dim, cv::Mat1d W, cv::Mat1d encodedFeat, const FileHandler *fpm, double B = 0, int numPosSamples = 0, int numIterat = 0, double rateVali = 0);
    ~AttriModel1D(){
        W.release();
        encodedFeat.release();
    }
    bool modelExtracted() const;
    void dump() const;
    void dumpOnline() const;
    void load();

    /// model, row := 1, col := dimension of feature
    cv::Mat1d W;
    cv::Mat1d encodedFeat;
    /// bias
    double B;
    int numPosSamples;
    int dim;

    int numIterat;
    double rateVali;
private:
    const FileHandler *fpm;
};

//TODO: structure of attriModel1d
class AttriModel{
public:
    AttriModel(const int dimOfAtt, const int dimOfFeat, const FileHandler *fpm);
    AttriModel(const AttriModel &am);
    ~AttriModel(){
        W.release();
        B.release();
    };
    //check if
    /// restore attriModel
    void reload();
    /// writeout attriModel
    void dump() const;
    void dumpOnline() const;
    /// update Model
    void update(const AttriModel1D &am);

    /// W for model, row as dimension of attribute space, cols as dimension of feature space.
    cv::Mat1d W; 
    /// B for bias
    cv::Mat1d B;
private:
    const FileHandler *filePathManager;
};

}
