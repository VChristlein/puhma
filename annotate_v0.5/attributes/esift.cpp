#include "esift.h"

#include "opencv2/highgui/highgui.hpp"
#include <opencv2/nonfree/features2d.hpp>
#include <iostream>
#include <cmath>

extern "C" {
    #include <vl/dsift.h>
}

namespace puhma{

void ESIFTDense :: compute(const cv::Mat& s,
        cv::Mat & desc){
    //compute dsift using densefeaturedetector and siftfeaturedesciptor.
    cv::Ptr<cv::FeatureDetector> detector;
    detector = cv::Ptr<cv::FeatureDetector> (new cv::DenseFeatureDetector(2.0f, 6, 1.5, 3));
    std::vector<cv::KeyPoint> kp;
    detector->detect(s,kp);
    auto sift = new cv::SIFT(0,3,0.005,10);
    sift->compute(s,kp,desc);

    normalize(desc);

    //compute boundingbox include 95% energy.
    cv::Rect bb = computeBoundingBox(s);
    //showBB(s, bb);
    int cx = bb.x+bb.width/2;
    int cy = bb.y+bb.height/2;
    cv::Mat descextend(2,kp.size(),desc.type());
    for(size_t i=0; i<kp.size(); ++i){
        descextend.at<float>(0,i)=static_cast<float>(kp[i].pt.x-cx)/static_cast<float>(bb.width);//x
        descextend.at<float>(1,i)=static_cast<float>(kp[i].pt.y-cy)/static_cast<float>(bb.height);//y
    }
    desc = desc.t();
    desc.push_back(descextend);
    desc = desc.t();
};

void ESIFTPhow :: compute(const cv::Mat &s,
            cv::Mat& desc){

    CV_Assert(s.channels() == 1);
    // convert image to float
    cv::Mat1f img_f;
    s.convertTo(img_f,CV_32F);
    img_f /= 255.0;

    std::vector<cv::KeyPoint> keypoints;

    std::vector<int> steps {3};

    std::vector<int> binsize {2,4,6,8,10,12};

    // sift parameters
    float contrast_tau = 0.005;

    std::vector<int> all_n_kpts;
    std::vector<VlDsiftFilter*> all_dsift;
    std::vector<std::vector<int> > all_high_contrast;
    int n_feature;
    double window_size = 1.5;


    for( size_t bs = 0; bs < binsize.size(); bs++ ){
        cv::Mat1f cur_img = img_f;

        float magnif = 6; // in standard-sift this would be 3
        float sigma = binsize[bs] / magnif;
        cv::GaussianBlur(img_f, cur_img, cv::Size(0,0), sigma);
        // alternatively scale down images

        for( size_t st = 0; st < steps.size(); st++ ) {
            VlDsiftFilter* dsift = vl_dsift_new_basic(s.cols, s.rows, binsize[bs], steps[st]);
            vl_dsift_set_window_size(dsift, window_size);

            vl_dsift_set_flat_window(dsift, true);

            // extract features
            vl_dsift_process(dsift, (float*) cur_img.data);

            // feature dimension, standard = 128
            if( bs == 0 && st == 0)
                n_feature = vl_dsift_get_descriptor_size(dsift);

            // get number of keypoints (and thus number of descriptors)
            int num_kpts = vl_dsift_get_keypoint_num(dsift);
            all_n_kpts.push_back( num_kpts );

            // flag all descriptors having low contrast
            const VlDsiftKeypoint * kpts = vl_dsift_get_keypoints(dsift);
            std::vector<int> high_contrast;
            for ( size_t k = 0; k < num_kpts; k++){
                if( kpts[k].norm >= contrast_tau )  {
                    high_contrast.push_back(k);
                    // convert to OpenCV-Keypoint
                    keypoints.push_back( cv::KeyPoint(kpts[k].x, kpts[k].y, kpts[k].s) );
                }
            }
            all_high_contrast.push_back(high_contrast);
            all_dsift.push_back(dsift);
        }
    }


    //qDebug() << "extracted" << keypoints.size() << "descriptors - now copy them";
    desc.create(keypoints.size(), n_feature, CV_32F);
    CV_Assert(desc.isContinuous());

    int cur_kpt = 0;
    int cur_dsift = 0;
    for( size_t st = 0; st < steps.size(); st++ ) {
        for( size_t bs = 0; bs < binsize.size(); bs++ ){
            const float * descr = vl_dsift_get_descriptors(all_dsift[cur_dsift]);
            for( size_t k = 0; k < all_high_contrast[cur_dsift].size(); k++) {
                memcpy(desc.row(cur_kpt).data,
                        descr + all_high_contrast[cur_dsift][k]*n_feature,
                        sizeof(float)* n_feature);
                cur_kpt++;
            }
            // clean up
            vl_dsift_delete( all_dsift[cur_dsift] );
            cur_dsift++;
        }
    }

    //normalize(desc);

    //compute boundingbox include 95% energy.
    cv::Rect bb = computeBoundingBox(s);
    //showBB(s, bb);
    int cx = bb.x+bb.width/2;
    int cy = bb.y+bb.height/2;
    cv::Mat descextend(2,keypoints.size(),desc.type());
    if(keypoints.size()>0){
        for(size_t i=0; i<keypoints.size(); ++i){
            descextend.at<float>(0,i)=static_cast<float>(keypoints[i].pt.x-cx)/static_cast<float>(bb.width);//x
            descextend.at<float>(1,i)=static_cast<float>(keypoints[i].pt.y-cy)/static_cast<float>(bb.height);//y
        }
        desc = desc.t();
        desc.push_back(descextend);
        desc = desc.t();
    }
}

void ESIFT :: showBB(const cv::Mat &s, const cv::Rect &rect){
    cv::Scalar color = cv::Scalar(128);
    cv::Mat tmp = s.clone();
    cv::rectangle(tmp,rect,color,2);
    cv::imshow("test",tmp);
    cv::waitKey();
}

cv::Rect ESIFT :: computeBoundingBox(const cv::Mat1b &source){
    cv::Rect out(cv::Point(0,0),source.size());
    double sumOfEnergy=0;
    cv::Mat1b s = source.clone();
    //sum up all point contains energy.
    sumOfEnergy = sumTheEnergy(s);
    if(sumOfEnergy <= 20){
        //std::cout<<"called"<<std::endl;
        return out;
    }

    cv::threshold(s, s, 128, 255, cv::THRESH_BINARY | cv::THRESH_OTSU );

    double threshold = sumOfEnergy*0.95;
    //until the enery inside rectangle is equal or smaller than threshold.
    while(sumOfEnergy>threshold){
        cv::Rect d(0,0,0,0);
        cv::Rect tm(0,0,0,0);
        double de=sumOfEnergy;
        double te = de;
        //get the sum of left, right most/top, bottom 1 collumn/row;
        //compare to get minmum,
        if(out.width>1){
        //left;
            tm = cv::Rect(out.x,out.y,1,out.height);
            te = sumTheEnergy(s(tm));
            if(te<de){
                d = tm;
                de = te;
            }
            //right;
            tm = cv::Rect(out.x+out.width-1,out.y,1,out.height);
            te = sumTheEnergy(s(tm));
            if(te<de){
                d = tm;
                de = te;
            }
        }
        if(out.height>1){
            //top;
            tm = cv::Rect(out.x,out.y,out.width,1);
            te =  sumTheEnergy(s(tm));
            if(te<de){
                d = tm;
                de = te;
            }
            //bottom;
            tm = cv::Rect(out.x,out.y+out.height-1,out.width,1);
            te =  sumTheEnergy(s(tm));
            if(te<de){
                d = tm;
                de = te;
            }
        }
        //assign the new boundary of the rectangle
        out-=cv::Size(d.width==1,d.height==1);
        out+=cv::Point2i(d.x==out.x&&d.width==1, d.y==out.y&&d.height==1);
        sumOfEnergy-=de;
    }
    return out;
}

double ESIFT :: sumTheEnergy(const cv::Mat1b &source){
    double r=0;
    for(int y=0;y<source.rows;++y){
        const uchar* onerow = source.ptr<uchar>(y);
        for(int x=0;x<source.cols;++x){
            r += (255-static_cast<double>(onerow[x]));
        }
    }
    return r;
}

void ESIFT :: normalize(cv::Mat &s){
    cv::Mat s_n = s.clone();

    //l1-norm+sqrt
    for(int y=0; y< s_n.rows; ++y){
        double denorm = cv::norm(s_n.row(y),cv::NORM_L1);
        if(denorm != 0){
            s_n.row(y) = s_n.row(y)/static_cast<float>(denorm);
        }
    }
    cv::sqrt(s_n,s_n);
    s = s_n.clone();
}

}
