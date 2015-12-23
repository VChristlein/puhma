#include <opencv2/imgproc/imgproc.hpp>
#include <iostream>

#include "elsd_wrapper.h"

extern "C" {
#include "elsd.h"
#include "valid_curve.h"
#include "process_curve.h"
#include "process_line.h"
}

void ElsdWrapper::Ellipse:: ellipse2Poly(std::vector<cv::Point> & poly) const
{
    // let's get points of complete ellipse
    std::vector<cv::Point> pts;
    cv::ellipse2Poly(center,cv::Size(axes[0],axes[1]),angle,0,360,1,pts);

    // whole ellipse
    if ( ext1 == cv::Point2d(0.0,0.0)
         && ext2 == cv::Point2d(0.0,0.0) )
    {
        poly = pts;
        return;
    }

    // get closest points on ellipse to given extremal points
    double min1 = std::numeric_limits<double>::max();
    double min2 = std::numeric_limits<double>::max();
    int min_ind1 = 0;
    int min_ind2 = 0;
    for( size_t i = 0; i < pts.size(); i++ ) {
        double n1 = cv::norm(cv::Point2d(pts[i].x,pts[i].y) - ext1);
        if ( n1 < min1 ) {
            min_ind1 = i;
            min1 = n1;
        }
        double n2 = cv::norm(cv::Point2d(pts[i].x,pts[i].y) - ext2);
        if ( n2 < min2 ) {
            min_ind2 = i;
            min2 = n2;
        }
    }

    // copy points to poly
    if ( min_ind2 == min_ind1 ) {
        poly = pts;
    }
    else if ( min_ind2 < min_ind1 ) {
        poly.insert(poly.begin(), pts.begin()+min_ind1, pts.end());
        poly.insert(poly.end(), pts.begin(), pts.begin()+min_ind2+1);
    } else {
        poly.insert(poly.begin(), pts.begin()+min_ind1, pts.begin()+min_ind2+1);
    }
}

ElsdWrapper:: ElsdWrapper( bool _smooth,
                           double _quant,
                           double _ang_th,
                           double _eps,
                           int _n_bins,
                           double _max_grad,
                           double _density_th )
    : smooth(_smooth),
      quant(_quant),
      ang_th(_ang_th),
      eps(_eps),
      n_bins(_n_bins),
      max_grad(_max_grad),
      density_th(_density_th)
{}


void ElsdWrapper:: addLine(double *lin,
                           bool smooth,
                           std::vector<Line> & lines)
{
    if (smooth) {
        lin[0] *= 1.25;
        lin[1] *= 1.25;
        lin[2] *= 1.25;
        lin[3] *= 1.25;
        lin[4] *= 1.25;
    }

    Line l;
    l.line[0] = lin[0];
    l.line[1] = lin[1];
    l.line[2] = lin[2];
    l.line[3] = lin[3];
    l.width = lin[4];

    lines.push_back(l);
}

void ElsdWrapper:: addCircle(double * param,
                             int * pext,
                             bool smooth,
                             std::vector<Circle> & circles)
{
    if (smooth) {
        param[0] *= 1.25; param[1] *= 1.25;
        param[2] *= 1.25; param[3] *= 1.25;
        pext[0] *= 1.25; pext[1] *= 1.25;
        pext[2] *= 1.25; pext[3] *= 1.25;
    }
    double ang_start = atan2(pext[1]-param[1],pext[0]-param[0]);
    double ang_end = atan2(pext[3]-param[1],pext[2]-param[0]);

    double C = M_2__PI * param[2];
    Circle c;
    c.center = cv::Point2d(param[0], param[1]);
    c.axes = cv::Vec2d(param[2], param[3]);
    if ( angle_diff(ang_start,ang_end) >= M_2__PI*SQRT2/C
         || angle_diff_signed(ang_start,ang_end) <= 0 )
    {
        if ( ang_start < 0 ) ang_start += M_2__PI;
        if ( ang_end < 0 ) ang_end += M_2__PI;
        if ( ang_end < ang_start ) ang_end += M_2__PI;
        c.start_angle = ang_start*180/M_PI;
        c.end_angle = ang_end*180/M_PI;
        c.ext1 = cv::Point2d(pext[0],pext[1]);
        c.ext2 = cv::Point2d(pext[2],pext[3]);
    }
    circles.push_back(c);
}

void ElsdWrapper:: addEllipse( double *param,
                               int *pext,
                               bool smooth,
                               std::vector<Ellipse> & ellipses)
{
    if (smooth) {
        param[0] *= 1.25; param[1] *= 1.25;
        param[2] *= 1.25; param[3] *= 1.25;
        pext[4] *= 1.25; pext[5] *= 1.25;
        pext[6] *= 1.25; pext[7] *= 1.25;
    }

    Ellipse e;
    e.center = cv::Point2d(param[0],param[1]);
    //    if ( param[4] < 0 ) param[4] += M_2__PI;
    e.angle = param[4]*180.0/M_PI;
    e.axes = cv::Vec2d(param[2],param[3]);

    double a = pow((param[2]-param[3])/(param[2]+param[3]),2);
    double C = M_PI*(param[2]+param[3])*(1+3*a/(10+sqrt(4-3*a)));
    // note: this is not the real ang_start, ang_end!
    // this is just an approximation taking a circle
    double ang_start = std::atan2(pext[5]-param[1],pext[4]-param[0]);
    double ang_end = std::atan2(pext[7]-param[1],pext[6]-param[0]);
    if ( angle_diff(ang_start,ang_end) > M_2__PI*SQRT2/C
         || angle_diff_signed(ang_start,ang_end) < 0 )
    {
        e.ext1 = cv::Point2d(pext[4],pext[5]);
        e.ext2 = cv::Point2d(pext[6],pext[7]);
        // NOTE: since the refined points achieved by rosin_point()
        // are used in the svg-creation part
        // of the original implementation (i.e. centers will vary, etc.),
        // the ellipses there are slightly different than ours (still very similar though)
        rosin_point(param, pext[4], pext[5], &e.ext1.x, &e.ext1.y);
        rosin_point(param, pext[6], pext[7], &e.ext2.x, &e.ext2.y);
    }
    ellipses.push_back(e);
}

// this is basically a copy of elsd.c's main + EllipseDetection but replaced
// each call of write_svg to addEllipse/addCircle/addLine
void ElsdWrapper:: detect(const cv::Mat1d & img,
                          std::vector<Line> & lines,
                          std::vector<Circle> & circles,
                          std::vector<Ellipse> & ellipses)
{

    double p = ang_th/180.0;
    double prec = M_PI*ang_th/180.0; // radian precision
    double rho = quant/sin(prec);

    int ell_count = 0, line_count = 0, circ_count = 0;

    image_double angles,gradx,grady,grad,imgauss;
    image_char used;
    void *mem_p;
    struct coorlist *list_p;
    struct point *reg, *regl;
    struct point3 *regc, *rege;
    struct rect rec;
    int reg_size = 0,regp_size[3];
    unsigned int xsize,ysize; /* image size */
    int min_size[3];
    double logNT[3]; /* number of tests for the 3 primitive types */
    double nfa[3]; /* NFA value using the discrete formulation for the three primitive types */
    double parame[5], paramc[5]; /* ellipse/circle parameters */
    double lin[5];
    double mlog10eps = - log10(eps);
    double reg_angle;
    int pext[8];
    unsigned int xsz0,ysz0;

    // let's convert it to the image_double-type
    // Note: a shallow copy is made, don't free/delete image_data!
    // Note: image_double is actually a pointer to a struct
    image_double image = (image_double) new struct image_double_s;
    image->data = const_cast<double*>(reinterpret_cast<const double*>(img.data));
    image->xsize = img.cols;
    image->ysize = img.rows;

    xsz0 = image->xsize;
    ysz0 = image->ysize;

    /* perform gaussian smoothing and subsampling */
    if (smooth)
    {
        imgauss = gaussian_sampler( image, 0.8, 0.6);
        //		free_image_double(image); // don't free it (shallow copy!)
        /* compute gradient magnitude and orientation  */
        angles = ll_angle(imgauss,rho,&list_p,&mem_p,&gradx,&grady,&grad,n_bins,max_grad);
    }
    else
        angles = ll_angle(image,rho,&list_p,&mem_p,&gradx,&grady,&grad,n_bins,max_grad);

//    fprintf(stderr, "grady: %d x %d\n", grady->xsize, grady->ysize);
//    std::cerr << "A grady: " << grady->xsize << " x " << grady->ysize << "\n";

    xsize = angles->xsize;
    ysize = angles->ysize;

    /* number of tests for elliptical arcs */
    logNT[2] = 4.0 *(log10((double)xsize)+log10((double)ysize)) + log10(9.0) + log10(3.0); /* N^8 */
    /* number of tests for circular arcs */
    logNT[1] = 3.0 *(log10((double)xsize)+log10((double)ysize)) + log10(9.0) + log10(3.0); /* N^6 */
    /* number of tests for line-segments */
    logNT[0] = 5.0 *(log10((double)xsize)+log10((double)ysize))/2.0 + log10(11) + log10(3.0); /* N^5 */

    /* thresholds from which an elliptical/circular/linear arc could be meaningful */
    min_size[2] =(int)((-logNT[2]+log10(eps))/log10(p));
    min_size[1] =(int)((-logNT[1]+log10(eps))/log10(p));
    min_size[0] =(int)((-logNT[0]+log10(eps))/log10(p));

    /* allocate memory for region lists */
    reg = (struct point *) calloc(xsize * ysize, sizeof(struct point));
    regl = (struct point *) calloc(xsize * ysize, sizeof(struct point));
    regc = (struct point3 *) calloc(xsize * ysize, sizeof(struct point3));
    rege = (struct point3 *) calloc(xsize * ysize, sizeof(struct point3));
    used = new_image_char_ini(xsize,ysize,NOTUSED);

    /* init temporary buffers */
    gBufferDouble = (double*)malloc(sizeof(double));
    gBufferInt    = (int*)malloc(sizeof(int));

    /* begin primitive detection */
    for( ; list_p; list_p = list_p->next)
    {
        reg_size = 0;
        if( used->data[list_p->y*used->xsize+list_p->x] == NOTUSED
                && angles->data[list_p->y*angles->xsize+list_p->x] != NOTDEF )
        {
            /* init some variables */
            for ( int i=0; i < 5; i++ )
            {
                parame[i] = 0.0; paramc[i] = 0.0;
            }
            nfa[2] = nfa[1] = nfa[0] = mlog10eps;
            reg_size = 1;regp_size[0] = regp_size[1] = regp_size[2] = 0;
            region_grow(list_p->x, list_p->y, angles, reg, &reg_size, &reg_angle,
                        used, prec);


            /*-------- FIT A LINEAR SEGMENT AND VERIFY IF VALID ------------------- */
            valid_line(reg,&reg_size,reg_angle,prec,p,&rec,lin,grad,gradx,grady,
                       used,angles,density_th,logNT[0],mlog10eps,&nfa[0]);
            regp_size[0] = reg_size;

            for ( int i=0; i < regp_size[0]; i++ ) {
                regl[i].x = reg[i].x; regl[i].y = reg[i].y;
            }

            if ( reg_size > 2 )
            {

                /*-------- FIT CONVEX SHAPES (CIRCLE/ELLIPSE) AND VERIFY IF VALID -------- */
//                std::cerr << "B grady: " << grady->xsize << " x " << grady->ysize << "\n";
                valid_curve(reg,&reg_size,prec,p,angles,used,grad,gradx,grady,paramc,parame,
                            rec,logNT,mlog10eps,density_th,min_size,nfa,pext,regc,rege,regp_size);
//                std::cerr << "C grady: " << grady->xsize << " x " << grady->ysize << "\n";

                //                fprintf(stderr, "%f %f %f %f  %f %f %f %f  %d %d %d %d  %d %d %d %d\n",
                //                        paramc[0], paramc[1], paramc[2], paramc[3],
                //                        parame[0], parame[1], parame[2], parame[3],
                //                        pext[0], pext[1], pext[2], pext[3],
                //                        pext[4], pext[5], pext[6], pext[7]);

                /* ------ DECIDE IF LINEAR SEGMENT OR CIRCLE OR ELLIPSE BY COMPARING THEIR NFAs -------*/
                // -ellipse-
                if( nfa[2] > mlog10eps
                        && nfa[2] >= nfa[0]
//                        && nfa[2] > nfa[1]
                        && regp_size[2] > min_size[2])
                {
                    ell_count++;
                    addEllipse(parame, pext, smooth, ellipses);
                    for ( int i=0; i < regp_size[0]; i++ )
                        used->data[regl[i].y*used->xsize+regl[i].x] = NOTUSED;
                    for (int i=0; i < regp_size[1]; i++)
                        used->data[regc[i].y*used->xsize+regc[i].x] = NOTUSED;
                    for (int i=0; i < regp_size[2]; i++) {
                        if (rege[i].z == USEDELL)
                            used->data[rege[i].y*used->xsize+rege[i].x] = USED;
                        else
                            used->data[rege[i].y*used->xsize+rege[i].x] = USEDELLNA;
                    }
                }
                // -circle-
//                else if( nfa[1] > mlog10eps
//                         && nfa[1] > nfa[0]
//                         && nfa[1] > nfa[2]
//                         && regp_size[1] > min_size[1])
//                {
//                    circ_count++;
//                    addCircle(paramc, pext, smooth, circles);

//                    for ( int i = 0; i < regp_size[0]; i++ )
//                        used->data[regl[i].y*used->xsize+regl[i].x] = NOTUSED;
//                    for ( int i = 0; i < regp_size[2]; i++ )
//                        used->data[rege[i].y*used->xsize+rege[i].x] = NOTUSED;
//                    for ( int i = 0; i < regp_size[1]; i++ ) {
//                        if (regc[i].z == USEDCIRC)
//                            used->data[regc[i].y*used->xsize+regc[i].x] = USED;
//                        else
//                            used->data[regc[i].y*used->xsize+regc[i].x] = USEDCIRCNA;
//                    }
//                }
                // -line-
                else if( nfa[0] > mlog10eps
                         && regp_size[0] > min_size[0]
//                         && nfa[0] > nfa[1]
                         && nfa[0] > nfa[2])
                {
                    line_count++;
                    addLine(lin, smooth, lines);

                    for ( int i = 0; i < regp_size[1]; i++ )
                        used->data[regc[i].y*used->xsize+regc[i].x] = NOTUSED;
                    for ( int i = 0; i < regp_size[2]; i++)
                        used->data[rege[i].y*used->xsize+rege[i].x] = NOTUSED;
                    for ( int i = 0; i < regp_size[0]; i++)
                        used->data[regl[i].y*used->xsize+regl[i].x] = USED;
                }
                else /* no feature */
                {
                    for ( int i = 0; i < regp_size[1]; i++)
                        used->data[regc[i].y*used->xsize+regc[i].x] = NOTUSED;
                    for ( int i = 0; i < regp_size[2]; i++)
                        used->data[rege[i].y*used->xsize+rege[i].x] = NOTUSED;
                    for ( int i = 0; i < regp_size[0]; i++)
                        used->data[regl[i].y*used->xsize+regl[i].x] = NOTUSED;
                }
            }
        }/* IF USED */
    }/* FOR LIST */

    // Note: do not free the image-data, since we are using a shallow copy
    delete image;
    free_image_double(gradx); free_image_double(grady);
    free_image_double(grad); free_image_double(angles);
    free_image_char(used);
    free(reg);free(regl); free(regc); free(rege);
    free(gBufferDouble); free(gBufferInt);    
    free(mem_p);

    if( smooth ) {
        free_image_double(imgauss);
    }
    // since there exist some stupid global variables we need to
    // reset them to the elsd-begin-state
    gSizeBufferDouble = 1;
    gSizeBufferInt = 1;
}

void ElsdWrapper:: draw( const cv::Mat & src,
                         const std::vector<Line> & lin,
                         const std::vector<Circle> & circ,
                         const std::vector<Ellipse> & ell,
                         cv::Mat & elsd_image,
                         double min_angle,
                         bool draw_centers,
                         bool draw_extremal_pts ) const
{
    if( elsd_image.empty()
            || elsd_image.size != src.size
            || elsd_image.type() != src.type())
    {
        elsd_image = src.clone();
        if (elsd_image.channels() == 1)
            cv::cvtColor(elsd_image, elsd_image, CV_GRAY2BGR);
    }

    if ( elsd_image.channels() == 1 ) {
        cv::cvtColor(elsd_image, elsd_image, CV_GRAY2BGR);
    }

    if( !lin.empty() ) {
        for( size_t i = 0; i < lin.size(); i++) {
            cv::line(elsd_image, lin[i].start(), lin[i].end(), cv::Scalar(0,255,0) );
        }
    }
    if ( !circ.empty() ) {
        for( size_t i = 0; i < circ.size(); i++) {
//            double ang_start = std::atan2(circ[i].ext1.y-circ[i].center.y,
//                                          circ[i].ext1.x-circ[i].center.x);
//            double ang_end = std::atan2(circ[i].ext2.y-circ[i].center.y,
//                                        circ[i].ext2.x-circ[i].center.x);

//            if( angle_diff(ang_start, ang_end) < min_angle )
//                continue;

            if( draw_centers ) {
                cv::circle(elsd_image, circ[i].center, 3, cv::Scalar(255,0,0));
            }
            if ( draw_extremal_pts ) {
                cv::circle(elsd_image, ell[i].ext1, 3, cv::Scalar(0,0,255));
                cv::circle(elsd_image, ell[i].ext2, 3, cv::Scalar(0,0,255));
            }
            cv::ellipse(elsd_image, circ[i].center, cv::Size(circ[i].axes[0], circ[i].axes[1]),
                        circ[i].angle, circ[i].start_angle,
                        circ[i].end_angle, cv::Scalar(255,0,0) );//,1, CV_AA );
        }
    }
    if ( !ell.empty() ) {
        for( size_t i = 0; i < ell.size(); i++) {
            double ang_start = std::atan2(ell[i].ext1.y-ell[i].center.y,
                                          ell[i].ext1.x-ell[i].center.x);
            double ang_end = std::atan2(ell[i].ext2.y-ell[i].center.y,
                                        ell[i].ext2.x-ell[i].center.x);
//            int arc_start = ang_start*180/M_PI;
//            int arc_end = ang_end*180/M_PI;
//            int i;
//            if( arc_start > arc_end )
//            {
//                i = arc_start;
//                arc_start = arc_end;
//                arc_end = i;
//            }
//            while( arc_start < 0 )
//            {
//                arc_start += 360;
//                arc_end += 360;
//            }
//            while( arc_end > 360 )
//            {
//                arc_end -= 360;
//                arc_start -= 360;
//            }
//            if( arc_end - arc_start > 360 )
//            {
//                arc_start = 0;
//                arc_end = 360;
//            }

//            std::cerr << arc_end - arc_start << " "
//                      << arc_start << " " << arc_end << "\n";
////            std::cerr << "angle_diff: "  << angle_diff(ang_start, ang_end) << std::endl;
//            if( arc_end - arc_start < min_angle )
//                continue;
            if ( angle_diff_full(ang_start, ang_end, 1) > ((360.0-min_angle)/180.0)*M_PI )
                continue;

            std::vector<cv::Point> poly;
            ell[i].ellipse2Poly(poly);
            cv::polylines(elsd_image, poly, false, cv::Scalar(0,0,255));//,1, CV_AA);
            if( draw_centers ) {
                cv::circle(elsd_image, ell[i].center, 3, cv::Scalar(0,0,255));
            }
            if ( draw_extremal_pts ) {
                cv::circle(elsd_image, ell[i].ext1, 3, cv::Scalar(0,0,255));
                cv::circle(elsd_image, ell[i].ext2, 3, cv::Scalar(0,0,255));
            }
        }
    }
}

void ElsdWrapper::clip(const cv::Mat1b &mask,
                       std::vector<ElsdWrapper::Line> &lines,
                       std::vector<ElsdWrapper::Circle> &circles,
                       std::vector<ElsdWrapper::Ellipse> &ellipses)
{
    // clip lines, circles and ellipses
    // TODO: this goes definitely nicer!
    // TODO: it currently always removes element if it hits a non-masked-area
    // --> should be changed to real clip
    std::vector<ElsdWrapper::Line>::iterator it_l;
    for( it_l = lines.begin(); it_l != lines.end(); ) {
        cv::Point l_s =  it_l->start();
        cv::Point l_e =  it_l->end();
        if ( l_s.x < 0 || l_s.x >= mask.cols
             || l_s.y < 0 || l_s.y >= mask.rows
             || l_e.x < 0 || l_e.x >= mask.cols
             || l_e.y < 0 || l_e.y >= mask.rows )
        {
            it_l = lines.erase(it_l);
            continue;
        }

        cv::LineIterator lit(mask, l_s, l_e);
        bool removed = false;
        for( int l = 0; l < lit.count; l++, ++lit ) {
            if ( **lit == 255) {
                it_l = lines.erase(it_l);
                removed = true;
                break;
            }
        }
        if ( !removed )
            ++it_l;
    }
    std::vector<ElsdWrapper::Circle>::iterator it_c;
    for( it_c = circles.begin(); it_c != circles.end();  ) {
        std::vector<cv::Point> pts;
        cv::ellipse2Poly(it_c->center, cv::Size(it_c->axes[0], it_c->axes[1]),
                         it_c->angle,
                         it_c->start_angle, it_c->end_angle,
                         1, pts);
        bool removed = false;
        for( size_t l = 0; l < pts.size(); l++ )
        {

            if ( pts[l].y < 0 || pts[l].x < 0
                 || pts[l].y >= mask.rows || pts[l].x >= mask.cols
                 || mask(pts[l].y, pts[l].x) == 255 )
            {
                it_c = circles.erase(it_c);
                removed = true;
                break;
            }
        }
        if ( !removed )
            ++it_c;
    }
    std::vector<ElsdWrapper::Ellipse>::iterator it_e;
    for( it_e = ellipses.begin(); it_e != ellipses.end();  ) {
        std::vector<cv::Point> pts;
        it_e->ellipse2Poly(pts);
        bool removed = false;
        for( size_t l = 0; l < pts.size(); l++ )
        {
            if ( pts[l].y < 0 || pts[l].x < 0
                 || pts[l].y >= mask.rows || pts[l].x >= mask.cols
                 || mask(pts[l].y, pts[l].x) == 255 )
            {
                it_e = ellipses.erase(it_e);
                removed = true;
                break;
            }
        }
        if ( !removed )
            ++it_e;
    }
}

void ElsdWrapper::save(const std::vector<ElsdWrapper::Line> &l,
                       const std::vector<ElsdWrapper::Circle> &c,
                       const std::vector<ElsdWrapper::Ellipse> &e,
                       cv::FileStorage &fs) const
{
    if ( !fs.isOpened() ) {
        std::cerr << "WARNING: fs isn't open -> return save()\n";
        return;
    }

    fs << "lines" << "[";
    for( size_t i = 0; i < l.size(); i++ ) {
        fs << "{:"
           << "width" << l[i].width
           << "line" << l[i].line
           <<  "}";
    }
    fs << "]";

    fs << "circles" << "[";
    for( size_t i = 0; i < c.size(); i++ ) {
        fs << "{:"
           << "center" << c[i].center
           << "axes" << c[i].axes
           << "angle" << c[i].angle
           << "ext1" << c[i].ext1
           << "ext2" << c[i].ext2
           << "start_angle" << c[i].start_angle
           << "end_angle" << c[i].end_angle
           <<  "}";
    }
    fs << "]";

    fs << "ellipses" << "[";
    for( size_t i = 0; i < e.size(); i++ ) {
        fs << "{:"
           << "center" << e[i].center
           << "axes" << e[i].axes
           << "angle" << e[i].angle
           << "ext1" << e[i].ext1
           << "ext2" << e[i].ext2
           <<  "}";
    }
    fs << "]";
    fs.release();
}

void ElsdWrapper::load(const cv::FileNode &fs,
                       std::vector<ElsdWrapper::Line> &l,
                       std::vector<ElsdWrapper::Circle> &c,
                       std::vector<ElsdWrapper::Ellipse> &e)
{
//    if ( !fs.isOpened() ) {
//        std::cerr << "WARNING: fs isn't open -> return load()\n";
//        return;
//    }

    cv::FileNode lines = fs["lines"];
    cv::FileNodeIterator it_l = lines.begin();
    for( ; it_l != lines.end(); ++it_l ) {
        Line ll;
        (*it_l)["width"] >> ll.width;

        cv::FileNodeIterator it = (*it_l)["line"].begin();
        it >> ll.line[0] >> ll.line[1] >> ll.line[2] >> ll.line[3];

        l.push_back(ll);
    }

    cv::FileNode circles = fs["circles"];
    cv::FileNodeIterator it_c = circles.begin();
    for( ; it_c != circles.end(); ++it_c ) {
        Circle cc;
        cv::FileNodeIterator it = (*it_c)["center"].begin();
        it >> cc.center.x >> cc.center.y;
        it = (*it_c)["axes"].begin();
        it >> cc.axes[0] >> cc.axes[1];
        (*it_c)["angle"] >> cc.angle;
        it = (*it_c)["ext1"].begin();
        it >> cc.ext1.x >> cc.ext1.y;
        it = (*it_c)["ext2"].begin();
        it >> cc.ext2.x >> cc.ext2.y;
        (*it_c)["start_angle"] >> cc.start_angle;
        (*it_c)["end_angle"] >> cc.end_angle;
        c.push_back(cc);
    }

    cv::FileNode ellipses = fs["circles"];
    cv::FileNodeIterator it_e = ellipses.begin();
    for( ; it_e != ellipses.end(); ++it_e ) {
        Ellipse ee;
        cv::FileNodeIterator it = (*it_e)["center"].begin();
        it >> ee.center.x >> ee.center.y;
        it = (*it_e)["axes"].begin();
        it >> ee.axes[0] >> ee.axes[1];
        (*it_e)["angle"] >> ee.angle;
        it = (*it_e)["ext1"].begin();
        it >> ee.ext1.x >> ee.ext1.y;
        it = (*it_e)["ext2"].begin();
        it >> ee.ext2.x >> ee.ext2.y;
        e.push_back(ee);
    }
}


void ElsdWrapper::detectAndDraw( const cv::Mat & src,
                                 cv::Mat & elsd_image,
                                 bool draw_circles,
                                 bool draw_lines,
                                 bool draw_ellipses,
                                 bool draw_centers,
                                 bool draw_extremal_pts )
{
    std::vector<Line> lin;
    std::vector<Circle> circ;
    std::vector<Ellipse> ell;

    detect(src, lin, circ, ell);

    if ( !draw_lines )
        lin.clear();
    if ( !draw_circles )
        circ.clear();
    if ( !draw_ellipses )
        ell.clear();

    draw(src, lin, circ, ell , elsd_image, draw_centers, draw_extremal_pts);
}

// -- some conversions --
void ElsdWrapper:: detect(const cv::Mat & image,
                          std::vector<Line> & _lines,
                          std::vector<Circle> & _circles,
                          std::vector<Ellipse> & _ellipses)
{
    cv::Mat1d tmp;

    if (image.channels() == 1)
        tmp = cv::Mat1d(image);
    else {
        cv::Mat1b tmp1;
        cv::cvtColor(image, tmp1, CV_BGR2GRAY);
        tmp = tmp1;
    }
    detect(tmp, _lines, _circles, _ellipses);
}

void ElsdWrapper:: detect(const cv::Mat3b & image,
                          std::vector<Line> & _lines,
                          std::vector<Circle> & _circles,
                          std::vector<Ellipse> & _ellipses)
{
    cv::Mat1b tmp;
    cv::cvtColor(image, tmp, CV_BGR2GRAY);
    detect(tmp, _lines, _circles, _ellipses);
}

void ElsdWrapper:: detect(const cv::Mat1b & image,
                          std::vector<Line> & _lines,
                          std::vector<Circle> & _circles,
                          std::vector<Ellipse> & _ellipses)
{
    detect(cv::Mat1d(image), _lines, _circles, _ellipses);
}

