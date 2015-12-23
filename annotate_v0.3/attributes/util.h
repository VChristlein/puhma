#pragma once

#include "attributes_config.h"

#include <QString>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>

namespace puhma {

class FileHandler;

/*! \brief This Class provide a bunch of static function used all around.
 */
class Util
{
public:
    Util(){};
    ~Util(){};
    /// randomly choose one child collection(lenth l) from set(length s).
    static std::vector<int> generateRan(const int l,const int s);
    /// shuffle the vector.
    static void generateRan(std::vector<int> &vec);
    /// asign nan to 0 (float version)
    static void nanTo0f(cv::Mat &m);
    /// asign nan to 0 (double version)
    static void nanTo0d(cv::Mat &m);
    /// meanCenterPerCol (col as one dimension) (Mean value calculated by itself)
    static cv::Mat1d meanCenterPerCol(cv::Mat &s);
    /// meanCenterPerCol (Mean value given)
    static void meanCenterPerCol(cv::Mat &s, const cv::Mat &mean);
    /// l2 normalize per row(row as one sample)
    static void normalizePerRow(cv::Mat &s);
    /// dump a vector(double) into file.
    static void dumpVector(const std::string fn, const std::vector<double> vec);
};

/*! \brief This Class provide a common interface for data prepare.
 */
class DataPrepare
{
public:
    DataPrepare(const AttriConfig & c, const FileHandler *fpm);
    ~DataPrepare();
    /// prepare&proces dataset(whether count digitial will deliver different result)
    void prepare();
    ///prepare the Divisions of dataset.
    void prepareDivision();
    /// return the data
    std::vector<std::vector<std::string> > getData() const;
    /// return given number of random choose data
    std::vector<std::vector<std::string> > getRandomData(int number) const;
    /// return the data of choosen division
    std::vector<std::vector<std::string> > getDivision(std::string type) const;

    /// prepare image
    static cv::Mat prepareIm(const std::string path,
            const int gtThreshold,
            const int height); 
    static void prepareIm(cv::Mat &img,
            //const int gtThreshold,
            const int height); 
private:
    /*
     * each data has following structure:
     * [0]: filename
     * [1]: transcription
     * [2]: graylevel to binarize the line containing this word
     * [3]: classnumber
     */
    std::vector<std::vector<std::string> > data;
    /// key:transcription value:classnumber
    std::map<std::string, int> classes;

    std::vector<int> idxTrain; 
    std::vector<int> idxTest; 
    std::vector<int> idxValidation; 
    std::vector<int> idxOnline; 

    ///divide given vector into 2 parts randomly.
    std::vector<int> segVec(const int segSize,
            std::vector<int> &vec);

    ///read division file
    std::vector<std::string> readDivisionIdxFile(const std::string path);
    ///check if one file is in one division
    bool fileInSet(const std::string filename, const std::vector<std::string> set);
    
    void writeoutData();
    void restoreData();

    void writeoutClasses();
    void restoreClasses();

    void writeoutDivision(std::string path, const std::vector<int> &idxList) const;
    void restoreDivision(const std::string path, std::vector<int> &idxList);

    const AttriConfig config;
    const FileHandler *filePathManager;
};

/*! \brief This Class manager the file/dir path.
 */
class FileHandler
{
public:
    FileHandler(const AttriConfig & c);
    ~FileHandler();

    std::string classesPath;
    std::string dataPath;
    std::string pcaPath;
    std::string gmmPath;
    std::string trainSetPath;
    std::string testSetPath;
    std::string validationSetPath;
    std::string onlineSetPath;
    std::string attRepresTrain;
    std::string attRepresValidation;
    std::string attRepresTest;
    std::string attRepresTrainOnline;
    std::string attRepresValidationOnline;
    std::string attRepresTestOnline;
    std::string attriModels;
    std::string attriModelsOnline;
    std::string csrModelPath;

    std::string phocDir;
    std::string featsDir;
    std::string attriModelDir;
    
    std::string lexiconPath;

    ///check whether the given type of file is exist.
    bool fileExist(std::string filetype) const;
    ///check whether the given type of folder is exist.
    bool dirExist(std::string dirtype) const;
private:
    const AttriConfig config;
};

}
