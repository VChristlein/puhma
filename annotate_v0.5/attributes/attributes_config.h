#pragma once
#ifndef ATTRIBUTES_CONFIG_H
#define ATTRIBUTES_CONFIG_H

#include "vole_config.h"

namespace puhma{

class AttriConfig : public vole::Config
{
public:
    AttriConfig(const std::string& prefix = std::string());
    ~AttriConfig(){}

    // --- GENERAL
    /// some graphical visualization
	bool graphical;
	/// cmfd output file (can be the matrix or correlation map, etc)
	std::string inputfile;
	/// the output directory
	std::string outputdir;	
    /// input directory
    std::string inputfolder;
    
    /// transcription of the word image
    std::string transcription;
    /// file contains the transcriptions for all the word image <-- right now only support IAM
    std::string transcriptionfile;
    /// data to choose(IAM or PUHMA)
    std::string dataset;
    /// division files(right now only IAM)
    std::string divisionTrainfile;
    std::string divisionTestfile;
    std::string divisionValifile;

    /// whether compute PHOC or not.
    bool generatePHOC;
    /// whether use digital in PHOC or not.
    bool digitalInPHOC;

    /// whether compute fv or not.
    bool generateFV;
    /// height of image
    int heightIm;
    /// number of image to train gmm
    int numWordsTranGMM;
    /// number of PCA retain components
    int dimPCA;
    /// number of cluster in each region for GMM training
    int clusterGMM;
    /// number of regions along X axis
    int numSpatialX;
    /// number of regions along y axis
    int numSpatialY;

    // whether train attribute space or not
    bool trainAtt;
    /// unbalanced training
    bool doWeight;//not used yet

    /// list of stopword
    std::string sw;
    /// score calibrate method
    std::string calibrateMethod;
    /// Whether reduce the dimension
    bool doReduce;
    /// dimensions that CCA retains.
    int dimCCA;
    /// threshold rate to discard one attribute
    double discardThreshold;

    /// do the recognition task or not
    bool doRecog;

    /// whether online updating the attributes model
    bool doOnline;
    /// buffer size of online learning
    int bufferSize;
    /// Lambda used in online Learning.
    double onlineLambda;

    virtual std::string getString() const;

#ifdef WITH_BOOST
    void printConfig(void);
#endif // WITH_BOOST	


protected:
#ifdef WITH_BOOST
    virtual void initBoostOptions();
#endif // WITH_BOOST


};

}

#endif
