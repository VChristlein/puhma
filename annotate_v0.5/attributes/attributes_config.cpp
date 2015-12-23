#include "attributes_config.h"

#include <boost/algorithm/string.hpp>

using namespace boost::program_options;

namespace puhma{

AttriConfig :: AttriConfig(const std::string& prefix)
    : Config(prefix)
{
#ifdef WITH_BOOST
    initBoostOptions();
#endif // WITH_BOOST
}

std::string AttriConfig :: getString() const
{
	std::stringstream s;
	if (prefix_enabled){
		s << "[" << prefix << "]" << std::endl;
	}
	else {
		s << "input = " << inputfile << " # Image to process" << std::endl
          << "inputfolder = " << inputfolder << " # process multiple images" << std::endl
		  << "outputdir = " << outputdir << " # Working directory" << std::endl;
    }
    s << "transcription = " << transcription << " # Transcription of word image" << std::endl
      << "transcriptionfile = " << transcriptionfile << " # File contains transcription" << std::endl
      << "dataset = " << dataset << " # Handwriting dataset" << std::endl
      << "divisionTrainfile = " << divisionTrainfile << " # File indexes the trainset"<< std::endl
      << "divisionTestfile = " << divisionTestfile << " # File indexes the testset"<< std::endl
      << "divisionValifile = " << divisionValifile << " # File indexes the validationset"<< std::endl
      << "generatePHOC = " << generatePHOC << " # Whether compute PHOC or not" << std::endl
      << "digitalInPHOC = " << digitalInPHOC << " # Whether use digital in PHOC or not" << std::endl
      << "generateFV = " << generateFV << " # Whether generate FV or not" << std::endl
      << "heightIm = " << heightIm << " # Standard height of images" << std::endl
      << "numWordsTranGMM = " << numWordsTranGMM << " # Number of words to tranning GMM" << std::endl
      << "dimPCA = " << dimPCA << " # Number of PCA retained components" << std::endl
      << "clusterGMM = " << clusterGMM << " # Number of cluster in each region for GMM training" << std::endl
      << "numSpatialX = " << numSpatialX << " # Number of regions along X axis" << std::endl
      << "numSpatialY = " << numSpatialY << " # Number of regions along Y axis" << std::endl
      << "trainAtt = " << trainAtt << " # Whether training the attribute space" << std::endl
      << "doWeight = " << doWeight << " # Whether weight the unbalanced data trainning" << std::endl
      << "sw = " << sw << " # List of stopwords" <<std::endl
      << "calibrateMethod = " << calibrateMethod << " # Method to calibrate image score and label score."<<std::endl
      << "doReduce = " << doReduce << "# Whether reduce the attributes dimension."
      << "dimCAA = " << dimCCA << " # Dimensions that CCA retains" << std::endl
      << "discardThreshold =" << discardThreshold << "# Threshold of accuracy to discard." << std::endl
      << "doRecog = "<< doRecog << "# Whether do the recognition task." << std::endl
      << "doOnline =" << doOnline << " # Whether do the online updating of attributes model." << std::endl
      << "bufferSize =" << bufferSize << "# Positive Buffer Size of Online Learning." << std::endl
      << "onlineLambda = " << onlineLambda << "# Lambda used in online Learning."<< std::endl;
    return s.str();
}

void AttriConfig :: printConfig(void)
{
	std::cout << getString();
}

#ifdef WITH_BOOST
void AttriConfig :: initBoostOptions()
{
	if ( ! prefix_enabled ){
		options.add_options()
            (key("graphical"), bool_switch(&graphical)->default_value(false),
             "Show any graphical output during runtime")
            (key("input,I"), value(&inputfile)->default_value("input.png"),
             "Image to process")
            (key("inputfolder"), value(&inputfolder)->default_value(""),
             "Image folder to process")
            (key("output,O"), value(&outputdir)->default_value("/tmp/"),
             "Working directory")
            ;
	}
	options.add_options()
        (key("transcription,T"),value(&transcription)->default_value(""),
         "Transcription of word image")
        (key("transfile"),value(&transcriptionfile)->default_value(""),
         "File contains transcription")
        (key("phoc"),value(&generatePHOC)->default_value(false),
         "Whether compute PHOC or not.")
        (key("digital"),value(&digitalInPHOC)->default_value(false),
         "Whether use digital in PHOC or not")
        (key("fv"),value(&generateFV)->default_value(false),
         "Whether generate FV or not")
        (key("dataset"),value(&dataset)->default_value("PUHMA"),
         "Handwriting dataset")
        (key("traindivide"),value(&divisionTrainfile)->default_value(""),
         "File indexes trainset")
        (key("testdivide"),value(&divisionTestfile)->default_value(""),
         "File indexes testset")
        (key("validivide"),value(&divisionValifile)->default_value(""),
         "File indexes validationset")
        (key("heightIm"),value(&heightIm)->default_value(80),
         "Standard height of images")
        (key("numWord"),value(&numWordsTranGMM)->default_value(500),
         "Number of words to tranning GMM")
        (key("dimPCA"),value(&dimPCA)->default_value(30),//62 for non-IAMdataset
         "Number of PCA retained components")
        (key("clusterGMM"),value(&clusterGMM)->default_value(16),
         "Number of clusters in each region for GMM training")
        (key("numSpatialX"),value(&numSpatialX)->default_value(6),
         "Number of regions along X axis")
        (key("numSpatialY"),value(&numSpatialY)->default_value(2),
         "Number of regions along Y axis")
        (key("trainAtt"),value(&trainAtt)->default_value(false),
         "whether training the attribute space")
        (key("weight"),value(&doWeight)->default_value(false),
         "whether weight the unbalanced data trainning")
        (key("sw"),value(&sw)->default_value(""),
         "List of stopwords")
        (key("calibrateMethod"),value(&calibrateMethod)->default_value(""),
         "Method to calibrate image score and label score. can be 'no','cca','platt'.")
        (key("doReduce"),value(&doReduce)->default_value(false),
         "Whether reduce the attributes dimension.")
        (key("dimCCA"),value(&dimCCA)->default_value(80),
         "Dimensions that CCA retains")
        (key("discardRate"),value(&discardThreshold)->default_value(0),
         "Threshold rate to discard one attribute")
        (key("doRecog"),value(&doRecog)->default_value(false),
         "Whether do the recogniton task")
        (key("online"),value(&doOnline)->default_value(false),
         "Whether do the online updating of attributes model")
        (key("bufferSize"),value(&bufferSize)->default_value(50),
         "Positive Buffer Size of Online Learning.")
        (key("onlineLambda"),value(&onlineLambda)->default_value(0.00001),
         "# Lambda used in online Learning.")
        ;
}
#endif


}
