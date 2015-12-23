#include "cca.h"
#include "util.h"

#include <iostream>
#include <Eigen/Eigenvalues>

namespace puhma{

void CCA :: compute(const cv::Mat &att, const cv::Mat &phoc, const double reg,
        cv::Mat &Wx, cv::Mat &Wy){
    double reg2 = reg;

    int N = att.rows;
    int Datt = att.cols;
    int Dphoc = phoc.cols;

    if(att.type()!=CV_64F||phoc.type()!=CV_64F)
        throw std::runtime_error("CCA :: compute : only double type supported.");

    cv::Mat1d Cattatt = (att.t())*att/(double)N + (cv::Mat1d::eye(Datt,Datt))*(reg); //(Datt * Datt)
    cv::Mat1d Cphocphoc = (phoc.t())*phoc/(double)N + (cv::Mat1d::eye(Dphoc,Dphoc))*(reg2); //(Dphoc * Dphoc)
    cv::Mat1d Cattphoc = (att.t())*phoc/(double)N; //(Datt*Dphoc)
    cv::Mat1d Cphocatt = Cattphoc.t(); //(Dphoc*Datt)

    //cv::Mat1d solve1;
    //cv::Mat1d solve2;
    //cv::solve(Cattatt.clone(), Cattphoc.clone(), solve1, cv::DECOMP_SVD);
    //cv::solve(Cphocphoc.clone().t(), solve1.clone().t(), solve2, cv::DECOMP_SVD);

    //cv::Mat1d M = solve2.t()*Cphocatt;

    cv::Mat1d invCattatt;
    cv::Mat1d invCphocphoc;

    cv::invert(Cattatt, invCattatt, cv::DECOMP_SVD);
    cv::invert(Cphocphoc, invCphocphoc, cv::DECOMP_SVD);

    cv::Mat1d M = invCattatt * Cattphoc * invCphocphoc * Cphocatt; //(Datt * Datt)


    M = M.t();
    
    //eigenvector decomposite using eigen3
    CV_Assert(M.isContinuous());
    Eigen::Map<Eigen::MatrixXd > eigenM ((double *)(M.data),Datt,Datt);
    Eigen::EigenSolver<Eigen::MatrixXd> es;
    es.compute(eigenM, true);
    auto evct = es.eigenvectors();
    auto evalue = es.eigenvalues();
    //std::cout<<evct.real().col(0)<<std::endl;

    //back to cv
    cv::Mat1d evctByEigen (evct.rows(), evct.cols()*2, evct.real().data());
    cv::Mat1d evctCor = cv::Mat1d::zeros(evct.rows(), evct.cols());
    for(int x=0; x< evctCor.cols; ++x){
        evctByEigen.col(x*2).copyTo(evctCor.col(x));
    }
    //normalizePerRow(evctCor);

    cv::Mat1d evalueByEigen (evalue.rows(), 2, evalue.real().data());
    cv::Mat1d evalueCor = evalueByEigen.col(0);

    //eliminate the negative r.
    for(int y=0; y<evalueCor.rows; ++y){
        if(evalueCor.at<double>(y,0)<0){
            evalueCor.at<double>(y,0)=0;
        }
    }
    cv::sqrt(evalueCor, evalueCor);


    cv::Mat1d V;
    cv::flip(evctCor, V, 0);
    cv::flip(evalueCor,evalueCor,0);
    cv::Mat idxM;
    cv::sortIdx(evalueCor.clone(), idxM, CV_SORT_EVERY_COLUMN + CV_SORT_ASCENDING);
    
    cv::Mat1d evaluetemp = cv::Mat1d::zeros(evalueCor.size());
    for(int y=0; y< idxM.rows; ++y){
        int index = idxM.at<int>(y,0);
        evalueCor.row(index).copyTo(evaluetemp.row(y));
        V.row(index).copyTo(evctCor.row(y));
    }
    
    cv::flip(evaluetemp, evalueCor, 0);
    cv::flip(evctCor, evctCor, 0);

    //std::cout<<evalueCor<<std::endl;

    Wx = evctCor.clone();


    //cv::solve(Cphocphoc.clone(), Cphocatt.clone(), solve1, cv::DECOMP_SVD);
    //Wy = solve1 * Wx;

    cv::Mat1d Wyt = cv::repeat(evalueCor,1, Dphoc);

    Wy = invCphocphoc * Cphocatt * (Wx.t()); //(Dphoc * Datt);
    Wy = Wy.t();
    //Wy = Wy.mul(1.0/Wyt);
    normalizePerRow(Wy);
}
    

void CCA :: normalizePerRow(cv::Mat &matToNor){
    //L2 normalize
    if(matToNor.type()!=CV_64F)
        throw std::runtime_error("CCA :: normalizePerRow : only double type supported.");
    for(int y = 0; y < matToNor.rows; ++ y){
        double normOfRow = cv::norm(matToNor.row(y));
        cv::Mat temp = matToNor.row(y)/normOfRow;
        temp.copyTo(matToNor.row(y));
    }
    Util::nanTo0d(matToNor);
}

cv::Mat1d CCA :: meanCenter(cv::Mat &matToMean){
    //mean center
    if(matToMean.type()!=CV_64F)
         throw std::runtime_error("CCA :: meanCenter : only double type supported.");
    cv::Mat1d re = cv::Mat1d::zeros(1,matToMean.cols);
    for(int x = 0; x < matToMean.cols; ++x ){
        double meanOfCol = cv::mean(matToMean.col(x))[0];
        cv::Mat temp = matToMean.col(x)-meanOfCol;
        temp.copyTo(matToMean.col(x));
        re(0,x) = meanOfCol;
    }
    return re;
}

void CCA :: meanCenter(cv::Mat &matToMean, const cv::Mat &mean){
    CV_Assert(matToMean.type() == mean.type() && mean.type() == CV_64F);
    for(int x = 0; x < mean.cols; ++x){
        cv::Mat temp = matToMean.col(x)-mean.at<double>(0,x);
        temp.copyTo(matToMean.col(x));
    }
}

}
