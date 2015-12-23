#include "puhma_common.h"
#include <fstream>
#include <iostream>
//#include <chrono>
//#include <random>
#include <algorithm>
#include <cstdlib>
#include <ctime>

#include <opencv2/imgproc/imgproc.hpp>
#if CV_MAJOR_VERSION >= 3
    #include <opencv2/imgcodecs.hpp>
#else
    #include <opencv2/highgui/highgui.hpp>
#endif

//#include "opencvmat.ph.h"
// TODO: convert that to msghandler

// make things easier for other classes
//#define vout(x) (puhma::verbosity_out(x))

//namespace puhma {
//	static int verbosity;
//	/// ease the output
//	static std::ostream & verbosity_out(int x){
//		class null_out_stream : public std::ostream
//		{
//		public:
//			null_out_stream() : std::ios(0), std::ostream(0){}
//		};

//		static null_out_stream cnul;
//		return (verbosity >= x) ? std::cout : cnul;
//	}
//}

namespace puhma {

using namespace boost::program_options;
ENUM_MAGIC(OUTPUT_TYPE)

CommonConfig::CommonConfig( const std::string & prefix )
    : Config(prefix)
{
#ifdef WITH_BOOST
    initBoostOptions();
#endif // WITH_BOOST
}

CommonConfig::~CommonConfig() {
}

std::string CommonConfig :: getString() const
{
    std::stringstream s;
    if (prefix_enabled){
        s << "[" << prefix << "]" << std::endl;
    }
    s << "input = " << inputfile << " # Image to process\n"
      << "inputfolder = " << inputfolder << " # process multiple images\n"
      << "outputdir = " << outputdir << " # Working directory\n"
      << "output_type = " << output_type << " # output type (BINARY, YML, CSV)\n"
      << "labelfile = " << labelfile << " # label file\n"
      << "maskfolder = " << maskfolder << " # the mask folder\n"
      << "mask_suffix = " << mask_suffix << " # the mask suffix for imgs in maskfolder\n"
      << "graphical = " << (graphical ? "true" : "false") << " # graphical stuff\n"
      << "suffix = " <<  suffix << " # suffix for the inputfile if read from labelfile\n"
      << "max_descriptors = " << max_descriptors << " # load max descriptors (0 = all)\n"
      << "seed = " << seed << " # seed for random number generator (-1 = current time)\n"
      << "\n"
    ;
    return s.str();
}

#ifdef WITH_BOOST
void CommonConfig  :: initBoostOptions()
{
    options.add_options()
            (key("graphical"), bool_switch(&graphical)->default_value(false),
             "Show any graphical output during runtime")
            (key("input,I"), value(&inputfile)->default_value(""),
             "Image to process")
            (key("inputfolder"), value(&inputfolder),
             "Image folder with images to process")
            (key("labelfile"), value(&labelfile),
             "path to the labelfile")
            (key("suffix"), value(&suffix),
             "suffix for the inputfile if read from labelfile")
            (key("maskfolder"), value(&maskfolder),
             "folder of masks")
            (key("mask_suffix"), value(&mask_suffix),
             "suffix for images in maskfolder")
            (key("outputdir,O"), value(&outputdir)->default_value("/tmp/"),
             "Working directory")
            (key("output_type"), value(&output_type)->default_value(OUT_BINARY),
             "output type (binary, csv, yml)")
            (key("max_descriptors"), value(&max_descriptors)->default_value(0),
             "load only max descriptors")
            (key("seed"), value(&seed)->default_value(-1),
             "seed for random number generator (-1 = time)")
            ;
}
#endif
void CommonConfig :: printConfig()
{
    std::cout << getString();
}

QString morphFile(QString s){
    if (s.startsWith ("~/"))
        s.replace (0, 1, QDir::homePath());
    return s;
}

std::string morphFile (std::string s) {
    if (s == "") return "";
    QString q = QString::fromStdString(s);
    if (q.startsWith ("~/"))
        q.replace (0, 1, QDir::homePath());
    return q.toStdString();
}

QStringList imagesFromDir( const QString & dir_name )
{
    QDir dir( dir_name,
              "",
              QDir::Name | QDir::IgnoreCase,
              QDir::NoDotAndDotDot | QDir::Files | QDir::Readable);
    if ( !dir.exists())
        qDebug() << "WARNING: dir " << dir_name << " doesn't exist";
    QStringList filters;
    filters << "*.jpg" << "*.bmp" << "*.jpeg" << "*.tif" << "*.png";
    dir.setNameFilters(filters);

    return dir.entryList();
}

void readImages( const QString & dir,
                 QStringList & image_names,
                 QList<cv::Mat> & images,
                 float resize_factor)
{
    QStringList entries = imagesFromDir(dir);
    foreach( QString entry, entries) {
        cv::Mat img = cv::imread(QFileInfo(dir, entry).filePath().toStdString(), -1);
        if( img.empty() ) {
            qWarning() << "WARNING: cannot open: " << entry;
            continue;
        }
        if ( resize_factor != 1.0) {
            cv::Mat _img;
            cv::resize(img, _img, cv::Size(), resize_factor,
                       resize_factor,
#if CV_MAJOR_VERSION >= 3
                       cv::INTER_LINEAR
#else
                       CV_INTER_LINEAR
#endif
                           );
            img = _img;
        }
        images.append(img);
        image_names.append(entry);
    }
}

// note: we could also use push_back but should be faster that way
void concatenate(const std::vector<cv::Mat> & matrices,
                  cv::Mat & merged,
                  bool vertical)
{
    if( matrices.empty() )
        return;
    if ( matrices.size() == 1 ) {
        merged = matrices[0];
        return;
    }
    if ( vertical ) { // numpy: axis=0
        int row_count = 0;
        for( size_t i = 0; i < matrices.size(); i++ )
            row_count += matrices[i].rows;

        merged.create( row_count, matrices[0].cols, matrices[0].type() );
        int start = 0;
        for( size_t i = 0; i < matrices.size(); i++ ) {
            cv::Mat submut = merged.rowRange(start, start + matrices[i].rows);
            matrices[i].copyTo(submut);
            start += matrices[i].rows;
        }
    }
    else {
        int col_count = 0;
        for( size_t i = 0; i < matrices.size(); i++ )
            col_count += matrices[i].cols;

        merged.create( matrices[0].rows, col_count, matrices[0].type() );
        int start = 0;
        for( size_t i = 0; i < matrices.size(); i++ ) {
            cv::Mat submut = merged.colRange(start, start + matrices[i].cols);
            matrices[i].copyTo(submut);
            start += matrices[i].cols;
        }
    }
}


cv::Mat createOne(std::vector<cv::Mat> & images, int cols, int min_gap_size)
{
    int max_width = 0;
    int max_height = 0;
    for ( int i = 0; i < images.size(); i++) {
        if ( i > 0 && images[i].type() != images[i-1].type() ) {
            qWarning() << "WARNING:createOne failed, different types of images";
            return cv::Mat();
        }
        max_height = std::max(max_height, images[i].rows);
        max_width = std::max(max_width, images[i].cols);
    }
    // number of images in y direction
    int rows = std::ceil(images.size() / cols);

    cv::Mat result = cv::Mat::zeros(rows*max_height + (rows-1)*min_gap_size,
                                    cols*max_width + (cols-1)*min_gap_size, images[0].type());
    size_t i = 0;
    int current_height = 0;
    int current_width = 0;
    for ( int y = 0; y < rows; y++ ) {
        for ( int x = 0; x < cols; x++ ) {
            if ( i >= images.size() )
                return result;
            cv::Mat to(result,
                       cv::Range(current_height, current_height + images[i].rows),
                       cv::Range(current_width, current_width + images[i].cols));
            images[i++].copyTo(to);
            current_width += max_width + min_gap_size;
        }
        current_width = 0;
        current_height += max_height + min_gap_size;
    }
    return result;
}



std::string getFilename(const std::string & outputdir,
                        const std::string & img_filename,
                        const std::string & identifier,
                        const std::string & ext,
                        bool get_last)
{
    QString output_path;
    QString last_path = "";
    // try different names for max iterations
    int max_names = 50;
    for( int i = 0; i < max_names; i++) {
		// completeBaseName instead of basename to allow '.' in the filename
        QString filename = QFileInfo(QString::fromStdString(img_filename)).completeBaseName()
                           + (identifier == "" ? "" : "_" + QString::fromStdString(identifier))
                           + ( i == 0 ? "" : "_" + QString::number(i) )
                           + "." + QString::fromStdString(ext);
        QDir d(morphFile(QString::fromStdString(outputdir)));
        QString dir = d.canonicalPath();
        if (dir == "") {
            std::cerr << "WARNING: " << outputdir << " does not exist?!\n";
        }
        filename = dir + "/" + filename;
        output_path = QFileInfo(filename).filePath();
        if ( ! QFileInfo(output_path).exists() )
            break;
        last_path = output_path;
    }
    if (  QFileInfo(output_path).exists() ) {
        qDebug() << "couldn't build " << output_path << "as it already exists" << max_names <<"x";
        return "";
    }
    if ( last_path == "" )
        last_path = output_path;
    return get_last ? last_path.toStdString() : output_path.toStdString();

}

void dumpMatrices(const std::string & outputdir,
                  const std::string & img_filename,
                  const std::string & identifier,
                  const std::vector<cv::Mat> & dump_matrices,
                  int verbosity,
                  OUTPUT_TYPE out_type)
{
    CV_Assert(!dump_matrices.empty());

    if ( out_type == OUT_CSV ) {
        std::string output_path = getFilename(outputdir, img_filename, identifier, "csv");
        if ( output_path == "" ){
            std::cerr << "WARNING: output_path empty\n";
            return;
        }

        QFile file(QString::fromStdString(output_path));
        if ( !file.open(QIODevice::WriteOnly | QIODevice::Text) ) {
            qDebug() << "cannot open file" << QString::fromStdString(output_path);
            return;
        }
        QTextStream file_stream(&file);
        foreach( const cv::Mat & dump_matrix, dump_matrices ){
            CV_Assert(dump_matrix.channels() == 1);
            if ( dump_matrix.type() == CV_32FC1){
                for( int y = 0; y < dump_matrix.rows; y++ ) {
                    for( int x = 0; x < dump_matrix.cols; x++ ) {
                        file_stream << (float) dump_matrix.at<float>(y,x);
                        if ( x != dump_matrix.cols -1 )
                            file_stream << ",";
                    }
                    file_stream << "\n";
                }
            } else if ( dump_matrix.type() == CV_64FC1){
                for( int y = 0; y < dump_matrix.rows; y++ ) {
                    for( int x = 0; x < dump_matrix.cols; x++ ) {
                        file_stream << (double) dump_matrix.at<double>(y,x);
                        if ( x != dump_matrix.cols -1 )
                            file_stream << ",";
                    }
                    file_stream << "\n";
                }
            } else if ( dump_matrix.type() == CV_32SC1 ){
                for( int y = 0; y < dump_matrix.rows; y++ ) {
                    for( int x = 0; x < dump_matrix.cols; x++ ) {
                        file_stream << (int) dump_matrix.at<int>(y,x);
                        if ( x != dump_matrix.cols -1 )
                            file_stream << ",";
                    }
                    file_stream << "\n";
                }
            } else if ( dump_matrix.type() == CV_8UC1 ){
                for( int y = 0; y < dump_matrix.rows; y++ ) {
                    for( int x = 0; x < dump_matrix.cols; x++ ) {
                        file_stream << (uchar) dump_matrix.at<uchar>(y,x);
                        if ( x != dump_matrix.cols -1 )
                            file_stream << ",";
                    }
                    file_stream << "\n";
                }
            } else
                qDebug() << "dumpMatrix: format currently not supported:" << dump_matrix.type();
        }
        file.close();
        if (verbosity >= 1){
            std::cout << "wrote " << output_path.c_str() << std::endl;
        }
    }
    else
    {
        cv::Mat merg;
        concatenate(dump_matrices,  merg);
        dumpMatrix(outputdir, img_filename, identifier, merg, verbosity, out_type);
    }
}

void dumpMatrices(const std::string & outputdir,
                  const std::string & img_filename,
                  const std::string & identifier,
                  const std::vector<cv::Mat> & dump_matrices,
                  int verbosity,
                  bool as_csv)
{
    dumpMatrices(outputdir, img_filename, identifier, dump_matrices,
            verbosity, as_csv ? OUT_CSV : OUT_PNG);

}

std::string dumpMatrix( const std::string & outputdir,
                 const std::string & img_filename,
                 const std::string & identifier,
                 const cv::Mat & dump_matrix,
                 int verbosity,
                 OUTPUT_TYPE out_type)
{
    std::string output_path;
    bool wrote = true;

    if ( out_type == OUT_BINARY ) {
//        CV_Assert(dump_matrix.type() == CV_32FC1);
        CV_Assert(dump_matrix.channels() == 1);
        CV_Assert(dump_matrix.isContinuous());

        output_path = getFilename(outputdir, img_filename, identifier, "ocvmb");
        if ( output_path == "" ){
            qDebug() << "WARNING dumpMatrix() output_path empty, cannot dump";
            wrote = false;
            return std::string();
        }

        QFile file(QString::fromStdString(output_path));
        if ( !file.open(QIODevice::WriteOnly | QIODevice::QIODevice::Unbuffered) ) {
            qDebug() << "cannot open file" << QString::fromStdString(output_path);
            return std::string();
        }

        int chans = dump_matrix.channels();

        int dataSize;

        switch(dump_matrix.depth()) {
        case CV_8U:
            file.putChar('u');
            dataSize = sizeof(char);
            break;
        case CV_8S:
            file.putChar('c');
            dataSize = sizeof(char);
            break;
        case CV_16U:
            file.putChar('w');
            dataSize = sizeof(short);
            break;
        case CV_16S:
            file.putChar('s');
            dataSize = sizeof(short);
            break;
        case CV_32S:
            file.putChar('i');
            dataSize = sizeof(int);
            break;
        case CV_32F:
            file.putChar('f');
            dataSize = sizeof(float);
            break;
        case CV_64F:
            file.putChar('d');
            dataSize = sizeof(double);
            break;
        }

        file.write((char *)&dump_matrix.rows, sizeof(int));
        file.write((char *)&dump_matrix.cols, sizeof(int));
        file.write((char *)&chans, sizeof(int));
        file.write((char*)dump_matrix.data,
                   dataSize*dump_matrix.rows*dump_matrix.cols);
        file.close();
    }

    else if ( out_type == OUT_CSV ) {
        output_path = getFilename(outputdir, img_filename, identifier, "csv");
        if ( output_path == "" ){
            qDebug() << "WARNING dumpMatrix() output_path empty, cannot dump";
            wrote = false;
            return std::string();
        }
        QFile file(QString::fromStdString(output_path));
        if ( !file.open(QIODevice::WriteOnly | QIODevice::Text) ) {
            qDebug() << "cannot open file" << QString::fromStdString(output_path);
            return std::string();
        }

        QTextStream file_stream(&file);
        for( int y = 0; y < dump_matrix.rows; y++ ) {
            for( int x = 0; x < dump_matrix.cols; x++ ) {
                if ( dump_matrix.type() == CV_32FC1)
                    file_stream << (float) dump_matrix.at<float>(y,x);
                else if ( dump_matrix.type() == CV_64FC1 )
                    file_stream << (double) dump_matrix.at<double>(y,x);
                else if ( dump_matrix.type() == CV_32SC1 )
                    file_stream << (int) dump_matrix.at<int>(y,x);
                else if ( dump_matrix.type() == CV_8UC1 )
                    file_stream << (uchar) dump_matrix.at<uchar>(y,x);
                else
                    qDebug() << "dumpMatrix: format currently not supported:" << dump_matrix.type();
                if ( x != dump_matrix.cols -1 )
                    file_stream << ",";
            }
            file_stream << "\n";
        }
        file.close();
    }
    else if ( out_type == OUT_PNG ) {
        output_path = getFilename(outputdir, img_filename, identifier, "png");
        if ( output_path == "" ){
            qDebug() << "WARNING dumpMatrix() output_path empty, cannot dump";
            wrote = false;
            return std::string();
        }
        // png compression level
        std::vector<int> parameters;
#if CV_MAJOR_VERSION >= 3
        parameters.push_back(cv::IMWRITE_PNG_COMPRESSION);
#else
        parameters.push_back(CV_IMWRITE_PNG_COMPRESSION);
#endif
        parameters.push_back(9); // highest compression

        if ( ! cv::imwrite(output_path, dump_matrix, parameters) ){
            qDebug() << "couldn't write " << output_path.c_str();
            wrote = false;
        }
    }
    else if ( out_type == OUT_YML ) {
        output_path = getFilename(outputdir, img_filename, identifier, "yml");
        if ( output_path == "" ){
            qDebug() << "WARNING dumpMatrix() output_path empty, cannot dump";
            wrote = false;
            return std::string();
        }
        cv::FileStorage fs(output_path, cv::FileStorage::WRITE);
        fs << "descriptors" << dump_matrix;
        fs.release();
    }
    /*
    else {
        GOOGLE_PROTOBUF_VERIFY_VERSION;

        std::string output_path = getFilename(outputdir, img_filename, identifier, "csv");
        if ( output_path == "" ){
            std::cerr << "WARNING: output_path empty\n";
            return;
        }
        fstream outfile(output_path, ios::out | ios::binary);
        int n_rows = 0;
        OpenCVMat global;
        foreach( const cv::Mat & dump_matrix, dump_matrices ){
            OpenCVMat ocvmat;
            n_rows += dump_matrix.rows;
            ocvmat.set_rows(n_rows);
            ocvmat.set_cols(dump_matrix.cols);
            ocvmat.set_channels(dump_matrix.channels());
            ocvmat.set_type(dump_matrix.type());
            ocvmat.set_data(dump_matrix.data());

            global.merge(ocvmat)
        }
        if (!ocvmat.SerializeToOstream(&outfile)) {
            std::cerr << "Failed to write opencv-mat." << std::endl;
            exit(1);
        }
        google::protobuf::ShutdownProtobufLibrary();
    }
    */

    if (wrote && verbosity >= 1){
        std::cout << "wrote " << output_path << std::endl;
        return output_path;
    } else {
        return std::string();
    }
}

void dumpMatrix( const std::string & outputdir,
                 const std::string & img_filename,
                 const std::string & identifier,
                 const cv::Mat & dump_matrix,
                 int verbosity,
                 bool as_csv)
{
    dumpMatrix(outputdir, img_filename, identifier, dump_matrix, verbosity,
            as_csv ? OUT_CSV : OUT_PNG);
}

void convertCSVtoYAML( const std::string & outputdir,
        const std::string & img_filename )
{
    // read file
    QFile file(QString::fromStdString(img_filename));
    if ( !file.open(QIODevice::ReadOnly | QIODevice::Text) ) {
        qDebug() << "cannot open file" << QString::fromStdString(img_filename);
        return;
    }
    QTextStream stream(&file);
    std::vector<float> data;
    int cols = 0;
    int rows = 0;
    while(true){
        QString line = stream.readLine();
        if( line.isNull())
            break;
        QStringList sl = line.split(",");
        int t_c = sl.size();
        if (cols != 0)
            CV_Assert( cols == t_c);
        else
            cols = t_c;
        rows++;

        foreach(QString s, sl){
            data.push_back(s.toFloat());
        }
    }
    // convert to mat
    cv::Mat m = cv::Mat(data).reshape(1,rows);

    // output as yaml
    std::string output_path = getFilename(outputdir, img_filename, "", "yml");
    if ( output_path == "" ){
        std::cerr << "WARNING: output_path empty\n";
        return;
    }
    cv::FileStorage fs(output_path, cv::FileStorage::WRITE);
    fs << "descriptors" << m;
    fs.release();
}

void loadSingleDescriptor(const std::string & file_name, cv::Mat & descr)
{
    QString fname = QString::fromStdString(file_name);
    if ( !QFileInfo(fname).exists() ){
        qDebug() << "WARNING does not exist --> skip" << fname;
        return;
    }

    if ( fname.endsWith(".yml") ) {
        cv::FileStorage fs(file_name, cv::FileStorage::READ);
        fs["descriptors"] >> descr;
        fs.release();
    }
    else if ( fname.endsWith(".ocvmb") ){
        QFile file(fname);
        if ( !file.open(QIODevice::ReadOnly | QIODevice::QIODevice::Unbuffered) ) {
            qDebug() << "WARNING: cannot open file" << fname;
            return;
        }

        char dtype;
        file.getChar(&dtype);
//        CV_Assert( dtype == 'f');
        int rows, cols, chans;
        file.read((char *)&rows, sizeof(int));
        file.read((char *)&cols, sizeof(int));
        file.read((char *)&chans, sizeof(int));
        CV_Assert(chans == 1);

        int dataSize;

        switch(dtype) {
        case 'u':
            descr.create(rows, cols, CV_8UC1);
            dataSize = sizeof(char);
            break;
        case 'c':
            descr.create(rows, cols, CV_8SC1);
            dataSize = sizeof(char);
            break;
        case 'w':
            descr.create(rows, cols, CV_16UC1);
            dataSize = sizeof(short);
            break;
        case 's':
            descr.create(rows, cols, CV_16SC1);
            dataSize = sizeof(short);
            break;
        case 'i':
            descr.create(rows, cols, CV_32SC1);
            dataSize = sizeof(int);
            break;
        case 'f':
            descr.create(rows, cols, CV_32FC1);
            dataSize = sizeof(float);
            break;
        case 'd':
            descr.create(rows, cols, CV_64FC1);
            dataSize = sizeof(double);
            break;
        }
        file.read((char*)descr.data, dataSize*descr.rows*descr.cols);
        file.close();

    } else if ( fname.endsWith(".csv")){
        QFile file(fname);
        if ( !file.open(QIODevice::ReadOnly | QIODevice::Text) ) {
            qDebug() << "WARNING: cannot open file" << fname;
            return;
        }
        QTextStream file_stream(&file);

        int rows = 0;
        int cols = 0;
        std::vector<float> vals;
        QString line;
        while ( (line = file_stream.readLine()) != QString::null )
        {
            QStringList val_str = line.split(',', QString::SkipEmptyParts);

            int col = 0;
            foreach(QString v, val_str){
                bool ok;
                vals.push_back(v.toFloat(&ok));
                assert(ok);
                col++;
            }
            if ( (vals.size() % col) != 0) {
                qDebug() << "ERROR, wrong number of columns: " << col
                         << "vs vals.size % col != 0:" << vals.size() % col;
            }
            if ( cols != 0 && cols != col )
                qDebug() << "ERROR: cols != previous cols";
            cols = col;
            rows++;
        }
        descr = cv::Mat1f(vals);
        descr = descr.reshape(0,rows);
        file.close();
    }

    if ( descr.empty() ){
        qDebug() << "WARNING descriptor is empty fileformat / saving wrong?!" << fname;
    }
}

cv::Mat1f loadDescriptors(const std::vector<std::string> & files,
                          int max_descriptors,
                          bool random,
                          int seed)
{
    int max_descr_file = 0;
    if ( max_descriptors > 0 ){
        max_descr_file = max_descriptors / files.size();
        if ( random ){
            if (seed == -1)
                std::srand ( unsigned ( std::time(0) ) );
            else
                std::srand ( seed );
        }
    }

    std::vector<cv::Mat> all_descriptors;
    int prev_cols=-1;
    for(std::string file : files ){

        cv::Mat1f descr;
        loadSingleDescriptor(file, descr);

        // check integrity
        CV_Assert(!descr.empty());
        if ( prev_cols != -1 && prev_cols != descr.cols){
            throw std::runtime_error("ERROR: different number of columns");
        }
        prev_cols = descr.cols;

        // select descriptors from the file
        if ( max_descr_file > 0 ){
            int num_descr = std::min(max_descr_file, descr.rows);
            if (random){
                std::vector<int> indices;
                for( int i = 0; i < num_descr; i++)
                    indices.push_back(i);
                std::random_shuffle ( indices.begin(), indices.end() );
                cv::Mat1f d(num_descr, descr.cols);
                int cnt = 0;
                for( int ind : indices){
                    descr.row(ind).copyTo(d.row(cnt++));
                }
                descr = d;
            } else {
                descr = descr.rowRange(cv::Range(0,num_descr));
            }
        }
        all_descriptors.push_back(descr);
    }
    cv::Mat descriptors;
    concatenate(all_descriptors, descriptors);
    return descriptors;
}

std::vector<std::string> getFiles(const std::string &inputfolder,
                     const std::string &suffix,
                     const std::string &labelfile,
                     std::vector<int> * labels)
{
    QDir dir( QString::fromStdString(inputfolder),
              "",
              QDir::Name | QDir::IgnoreCase,
              QDir::NoDotAndDotDot | QDir::Files | QDir::Readable);
    if ( !dir.exists())
        qDebug() << "WARNING: dir " << QString::fromStdString(inputfolder) << " doesn't exist";

    if ( suffix != "" ){
        QStringList filters;
        filters << QString::fromStdString(suffix);
        dir.setNameFilters(filters);
    }
    if ( labelfile != "" ){
        std::ifstream ifs;
        ifs.open (labelfile, std::ifstream::in);

        QDir d(morphFile(QString::fromStdString(inputfolder)));
        QString dir = d.canonicalPath();
        if (dir == "") {
            throw std::runtime_error("getFiles(): outputdir does not exist");
        }

        std::vector<std::string> all_filenames;
        while (ifs.good()) {
            std::string filename;
            // TODO: allow string labels
            int label;
            ifs >> filename >> label;

            QFileInfo finfo(QString::fromStdString(filename));
            QString fname;
            if ( suffix != "" )
                fname = dir + "/"  + finfo.baseName()
                        + QString::fromStdString(suffix);
            else
                //BUG check.exists gives wrong result for existing files
                fname = dir + "/" + QString::fromStdString(filename);
            QFileInfo check(fname);
            if( !check.exists() ){
                qDebug() << "WARNING" << fname
                         << "does not exist! --> skip";
                continue;
            }
            all_filenames.push_back(fname.toStdString());
            if ( labels ){
                labels->push_back(label);
            }
        }
        ifs.close();
        return all_filenames;
    }

    std::vector<std::string> names;
    std::string path = dir.absolutePath().toStdString();
    foreach(QString str, dir.entryList())
        names.push_back(path + "/" + str.toStdString());
    return names;
}


// TODO: put this somewhere else
/*
cv::Mat mineFile(const cv::Mat1f & v_descr, const cv::Mat1f & v_kpts,
                 const cv::Mat1f & weights, std::vector<int> n_splits,
                 int n_per_split)
{
    CV_Assert(v_kpts.cols == 2);
    CV_Assert(v_kpts.rows == v_descr.rows);

    // partial kpt-selection
    double min_x, min_y, max_x, max_y;
    minMaxLoc(v_kpts.col(0), &min_x, &max_x;
    minMaxLoc(v_kpts.col(1), &min_y, &max_y);
    int w = max_x - min_x;
    int h = max_y - min_y
    cv::Mat1i kpt_map = cv::Mat1i::zeros( h+1, w+1 )
    for( int k = 0; k < v_kpts.rows; k++){
        kpt_map( v_kpts(k,1)-min_y, v_kpts(k,0)-min_x ) = k + 1;
    }

    int ww = static_cast<int>(w / n_splits.at(0))
    int hh = static_cast<int>(h / n_splits.at(1))
    std::vector<float> scores_per_file;

    for( int y = 0; y < n_splits[1]; y++){
        for( int x = 0; x < n_splits[0]; x++){
            // get now kpt-indices of that area
            int end_x, end_y
            if ( y == (n_splits[1] - 1) ) {
                end_y = h+1;
            } else {
                end_y = (y+1) * hh;
            }
            if ( x == (n_splits[0] - 1) ) {
                end_x = w+1;
            } else {
                end_x = (x+1) * ww;
            }

            cv::Mat1i ind_m = kpt_map( cv::Range(y*hh, end_y),
                                     cv::Range(x*ww, end_x) );
            std::vector<int> ind;
            for( int yy = 0; yy < ind_m.rows; yy++){
                for( int xx = 0; xx < ind_m.cols; xx++){
                    if( ind_m(y,x) == 0 )
                        continue
                    ind.push_back( ind_m(y,x) - 1 )
                }
            }


            // TODO: the dot product may be computed for all weights at
            // once...
            for( int e = 0; e < weights.rows; e++ ) {
                // evaluate classifier-weight
                // note: * here is a matrix-matrix-multiplication = dot product
                cv::Mat1f scores = v_descr * weight
                    // TODO
            }

            for e, weight in enumerate(weights):
                # evaluate classifier-weight
                scores = v_descr[ind].dot(weight)
                # sort the scores and take n_best ones
                max_score_ids = scores.argsort()[::-1][:n_per_split]
                # add them
                scores_per_file.extend(scores[max_score_ids])

                # brainfuck - let's hope that this is correct...
                ind_start = i*n_parts*n_per_split \
                            + y*n_splits[0]*n_per_split
                scores_per_weight[e, ind_start + x*n_per_split: ind_start +\
                             (x+1)*n_per_split] = scores[max_score_ids]
    return scores_per_file;
}
*/


} // namespace puhma
