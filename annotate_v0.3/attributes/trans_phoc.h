#pragma once

#include "attributes_config.h"
#include "util.h"

#include <opencv2/imgproc/imgproc.hpp>
#include <vector>
#include <string>

namespace puhma {

/*! \brief This Class implement the PHOC representation in
 *         Almazan, Word Spotting and Recognition with Embedded Attributes.
 *
 *  PHOC: pyramidal histogram of character. 604 dimensions.
 */
class TransPhoc 
{
public:
    TransPhoc(const AttriConfig &c, const FileHandler *fpm);
    ~TransPhoc();
    
    /// encoding transcriptions into PHOC representation
    void compute(const std::vector<std::vector<std::string> > &d);

    /// encoding one transcription into PHOC representation
    /*! \brief  from level 2 to level 5.
     *          each level counts the appearance of a-z 
     *              (if consider digital also 0-9)
     *          addition counts extra english bigram.
     */
    cv::Mat1b getOnePhoc(const std::string transcription);

    /// restore one PHOC rep from one file.
    cv::Mat1b fetchOnePhocFromFile(const std::string fn) const;

    const AttriConfig config;

    /// return size of PHOC
    const cv::Size getSize(){return sizeOfPHOC;};

    std::string getLabelat(int ndim) const
    {return labels4Dimension[ndim];};

private:
    cv::Size sizeOfPHOC;
    std::string bgrams;
    /// how many bigram is used.
    int bigramnum;
    /// how many characters be learned in each region.
    int charInterval;
    /// label for each dimension
    std::vector<std::string> labels4Dimension;
    ///assign the PHOC according to code number, word length and letter position
    /*! note:encode be asc2. otherwise :(
     */
    void signThePhoc(const int code,
            const int length,
            const int position,
            cv::Mat1b &phoc,
            const bool bigram = false);

    const FileHandler *filePathManager;
    /// dump one PHOC
    void writeoutOnePhoc(const cv::Mat1b &s,
            const std::string dir,
            const std::string dn) const;
    /// restore one PHOC
    cv::Mat1b restoreOnePhoc(const std::string dir,
            const std::string dn);
};
}
