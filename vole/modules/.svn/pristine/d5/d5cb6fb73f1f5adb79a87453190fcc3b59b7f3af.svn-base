#ifndef ELSD_WRAPPER_H
#define ELSD_WRAPPER_H

#include <opencv2/core/core.hpp>
#include <vector>

// TODO revise:
// make the vectors class members and clean up code
class ElsdWrapper
{
public:    
    struct Line {
        //! Line start and end coordinates.
        cv::Vec4d line;
        //! Line width.
        double width;
//        //! Angle precision.
//        double precision;
//        //! Number of false alarms.
//        double NFA;
        cv::Point start() const {
            return cv::Point(line[0], line[1]);
        }
        cv::Point end() const {
            return cv::Point(line[2], line[3]);
        }
        double angle() const {
            return atan2(line[3]-line[1], line[2]-line[0]);
        }
        double length() const {
            return std::sqrt( ((line[3]-line[1])*(line[3]-line[1]))
                    / ((line[2]-line[0])*(line[2]-line[0])) );
        }
    };
    struct Ellipse {
        Ellipse() :
            center(0.0,0.0),
            axes(0.0,0.0),
            angle(0.0),
            ext1(0.0,0.0), ext2(0.0,0.0){}
        /// convert our ellipse to a polygon
        void ellipse2Poly(std::vector<cv::Point> & poly) const;
        inline double eccentricity() const {
            double div = std::abs(((axes[0]*axes[0])/(axes[1]*axes[1])));
            if ( div < 1 )
                return std::sqrt( 1 - div);
            else {
                return std::sqrt( 1 - 1./div );
            }
        }
        inline double width() const {
            return axes[0];
        }
        inline double height() const {
            return axes[1];
        }     

        cv::Point2d center;
        cv::Vec2d axes; // rx,ry
        double angle; // rotation angle
        cv::Point2d ext1; // first extremal point
        cv::Point2d ext2; // second extremal point
    };
    struct Circle : Ellipse {
        Circle() : start_angle(0.), end_angle(360.) {}
        double start_angle, end_angle;
    };

    ElsdWrapper( bool smooth = true,
                 double quant = 2.0,
                 double ang_th = 22.5,
                 double eps = 1.0,                 
                 int n_bins = 1024,
                 double max_grad = 255.0,
                 double density_th = 0.7 );

    void detect(const cv::Mat1d & img,
                std::vector<Line> & lines,
                std::vector<Circle> & circles,
                std::vector<Ellipse> & ellipses);
    void detect(const cv::Mat & img,
                std::vector<Line> & lines,
                std::vector<Circle> & circles,
                std::vector<Ellipse> & ellipses);
    void detect(const cv::Mat3b & img,
                std::vector<Line> & lines,
                std::vector<Circle>& circles,
                std::vector<Ellipse> & ellipses);
    void detect(const cv::Mat1b & img,
                std::vector<Line> & lines,
                std::vector<Circle> & circles,
                std::vector<Ellipse> & ellipses);

    /// draws lines, circles, ellipse in elsd
    void detectAndDraw( const cv::Mat & src,
                        cv::Mat & elsd,
                        bool draw_circles = true,
                        bool draw_lines = true,
                        bool draw_ellipses = true,
                        bool draw_centers = false,
                        bool draw_extremal_pts = false);
    void draw( const cv::Mat & src,
               const std::vector<Line> & lin,
               const std::vector<Circle> & circ,
               const std::vector<Ellipse> & ell,
               cv::Mat & elsd_image,
               double min_angle = 0.0,
               bool draw_centers = false,
               bool draw_extremal_pts = false ) const;
    void clip(const cv::Mat1b &mask,
              std::vector<ElsdWrapper::Line> &lines,
              std::vector<ElsdWrapper::Circle> &circles,
              std::vector<ElsdWrapper::Ellipse> &ellipses);
    void save(const std::vector<ElsdWrapper::Line> &lines,
              const std::vector<ElsdWrapper::Circle> &circles,
              const std::vector<ElsdWrapper::Ellipse> &ellipses,
              cv::FileStorage & fs) const;
    void load(const cv::FileNode & fs,
              std::vector<ElsdWrapper::Line> &lines,
              std::vector<ElsdWrapper::Circle> &circles,
              std::vector<ElsdWrapper::Ellipse> &ellipses);
private:
    void addLine(double *lin, bool smooth,
                 std::vector<Line> & lines);
    void addCircle(double *param, int *pext, bool smooth,
                   std::vector<Circle> & circles);
    void addEllipse(double *param, int *pext, bool smooth,
                    std::vector<Ellipse> & ellipses);
//    std::vector<Line> all_lines;
//    std::vector<Circle> all_circles;
//    std::vector<Ellipse> all_ellipses;

    /// Bound to the quantization error on the
    /// gradient norm.
    double quant;
    /// Gradient angle tolerance in degrees.
    double ang_th;
    double eps;
    /// Gaussian smooth or not
    /// kernel-params: scale = 0.8, sigma = 0.6
    bool smooth;
    int n_bins;
    double max_grad;
    double density_th;
};

#endif
