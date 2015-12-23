#pragma once

#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/features2d/features2d.hpp>

namespace puhma {

/*! \brief this Class is abstract class for the augmented SIFT Descriptor
 */
class ESIFT
{
public:
    virtual void compute(const cv::Mat& s,
            cv::Mat& desc) = 0;
protected:
    ///greedly compute the minimum size of Bounding Box of given image that contains around 95% energy.
    cv::Rect computeBoundingBox(const cv::Mat1b &s);
    ///for test
    void showBB(const cv::Mat &s, const cv::Rect &rect);
    ///sum the energy of one image(according to pixel value)
    double sumTheEnergy(const cv::Mat1b &source);
    /// l1-sqrt normalize the sift
    void normalize(cv::Mat &s);
};
/*! \brief this Class implementes the augmented SIFT Descriptor by using dense + SIFT
 */
class ESIFTDense : public ESIFT
{
public:
    /// compute enhanced SIFT descriptor by adding 2 dims of relative coordination
    void compute(const cv::Mat& s,
            cv::Mat& desc);
private:
    ///greedly compute the minimum size of Bounding Box of given image that contains around 95% energy.
    /*cv::Rect computeBoundingBox(const cv::Mat1b &s);
    void showBB(const cv::Mat &s, const cv::Rect &rect);
    double sumTheEnergy(const cv::Mat1b &source);*/
};

/*! \brief this Class implementes the augmented SIFT Descriptor by using phow */

class ESIFTPhow : public ESIFT
{
public:
    /// compute augmented dense SIFT by phow(vl_feat).
    void compute(const cv::Mat &s,
            cv::Mat& desc);
private:
    ///greedly compute the minimum size of Bounding Box of given image that contains around 95% energy.
    /*cv::Rect computeBoundingBox(const cv::Mat1b &s);
    void showBB(const cv::Mat &s, const cv::Rect &rect);
    double sumTheEnergy(const cv::Mat1b &source);*/
};
}
