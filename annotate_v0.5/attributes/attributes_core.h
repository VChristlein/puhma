#pragma once

#include "attributes_config.h"
#include "util.h"
#include "statistic.h"
#include "recog.h"

#include <opencv2/imgproc/imgproc.hpp>
#include <QString>

namespace puhma {

/*! \brief attribute embeding processing
 * This class is the core-class for the attribute embeding processing
 */
class AttributeEmbed
{
public:
    AttributeEmbed();
    AttributeEmbed(AttriConfig &config);
    ~AttributeEmbed();

    //void run(const cv::Mat &img);
    /// prepare the data, include filter out illegal transcription or word image from dataset
    //and create division(training, validation , test) from dataset 
    void prepare();
    /// check the status, for annotation tool
    //0 for from start, 9 for train already finished.
    int check();
    /// label Embedding process, generate PHOC representation
    void labelEmbed();
    /// create high-order statistic Fisher vector of word images
    void fvRep();
    /// training attribute space
    void trainAS();
    /// calibrate attribute space and label space
    void calibrate();
    /// calibrate common subspace without evaluation
    void calibrateNoEva();
    /// recognition
    void recognition();
    /// method provided for annotation tool
    std::vector<std::string> recog4Annotation(cv::Mat img);
    /// learning phase. discarded
    void evaOnline();
    /// check if recognition model is ready
    bool isRecoReady();

    AttriConfig &config;

    const Statistic* getStatistic(){
        return st;
    }

    FileHandler *filePathManager;
    Statistic *st;
    DataPrepare *dataprepare;
    //void divideDataSet(std::vector<int> &idTrain, std::vector<int> &idTest, std::vector<int> &idVali);

private:
    Recog *reco4anno;
};

}
