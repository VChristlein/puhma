#ifndef PLATT_H
#define PLATT_H
#include <opencv2/core/core.hpp>

namespace puhma {

// From: Lin et al. "A Note on Platt's Probabilistic Outputs for Support Vector Machines"
void platt_calibrate (const cv::Mat1d & decision,
                      const cv::Mat1i & labels,
                      double & A, double & B);
// Wrapper for python
std::vector<double> platt_calibrate_vec(const cv::Mat1d & decision,
                                        const cv::Mat1i & labels);
double sigmoid_predict(double decision_value,
                       double A, double B);


///////
// Would only work with TBB instead of opencv
// since I need (simple) locks for newf!
//////

//class ParallelPlatt: public cv::ParallelLoopBody
//{
//public:
//    ParallelPlatt( const cv::Mat & _decision,
//                   const std::vector<double> & _t,
//                   double _newA,
//                   double _newB)
//        : decision(_decision),
//          t(_t),
//          newA(_newA),
//          newB(_newB)
//    {
//       newf = 0.0;
//    }

//    void operator()( const cv::Range & range ) const
//    {
//          double tmp = 0.0;
//        for( int i = range.start; i < range.end; i++) {
//            double fApB = decision(i,0)*newA + newB;
//            if (fApB >= 0)
//                tmp += t[i]*fApB + log(1+exp(-fApB));
//            else
//                tmp += (t[i]-1)*fApB + log(1+exp(fApB));
//        }
          // TODO: lock here newf and add tmp to it
//    }
//private:
//    const cv::Mat1f & decision;
//    const std::vector<double> & t;
//    double newf;
//    double newA;
//    double newB;
//};

} // namespace puhma

#endif // FISHER_H
