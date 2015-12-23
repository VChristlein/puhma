#pragma once
#ifndef PUHMA_COMMON_H
#define PUHMA_COMMON_H

#include "vole_config.h"

#include <opencv2/core/core.hpp>
#include <QFileInfo>
#include <QDir>
#include <QDebug>

// TODO: convert that to msghandler

// make things easier for other classes
//#define vout(x) (puhma::verbosity_out(x))

namespace puhma {
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

/**
 * @brief The OUTPUT_TYPE enum
 */
enum OUTPUT_TYPE { OUT_PNG, OUT_CSV, OUT_YML, OUT_BINARY}; //, PROTO };
#define OUTPUT_TYPEString {"OUT_PNG", "OUT_CSV", "OUT_YML", "OUT_BINARY"}

class CommonConfig : public vole::Config
{
public:
    explicit CommonConfig( const std::string& prefix = std::string() );
    ~CommonConfig();

    bool graphical;
    /// the input file
    std::string inputfile;
    /// the input folder, if not given, the path to the file is used
    std::string inputfolder;
    /// the labelfile
    std::string labelfile;
    /// the possible suffix labelfile -> inputfile
    std::string suffix;
    /// mask-folder, basename of img-file in inputfolder mast be similar
    std::string maskfolder;
    /// suffix for images in maskfolder
    std::string mask_suffix;
    /// the output directory
    std::string outputdir;
    /// type of the output (CSV / YML / OCVMB)
    OUTPUT_TYPE output_type;

    /// load only max descriptors
    int max_descriptors;
    /// common seed for all actions
    int seed;

public:
    virtual std::string getString() const;

#ifdef WITH_BOOST
    void printConfig();
#endif // WITH_BOOST
protected:
#ifdef WITH_BOOST
    virtual void initBoostOptions();
#endif // WITH_BOOST
};

/**
 * @brief getFiles: Creates a vector of filenames, either from a directory or from a given lable file.
 * @param inputfolder: Base folder to read from.
 * @param suffix: File suffix for filtering.
 * @param labelfile: Optional lablefile to read files and their lable from.
 * @param labels: If files are read from a lable file this vector will later on contain the according file lables.
 * @return std::vector<std::string> containing filenames.
 */
std::vector<std::string> getFiles(const std::string &inputfolder,
                                  const std::string &suffix,
                                  const std::string &labelfile,
                                  std::vector<int> *labels = NULL);

/**
 * @brief loadSingleDescriptor: Loads a single descriptor from disk. Possible formats are YAML, CSV or OCVMB. File formats are determined by file suffix.
 * @param file_name: Absolute filename of the descriptor.
 * @param descr: cv::Mat to store the read descriptor in.
 */
void loadSingleDescriptor(const std::string & file_name,
                    cv::Mat &descr);

/**
 * @brief loadDescriptors: Batch load vectors given by std::vector<std::string>
 * @param files: std::vector<std::string> containing absolute filenames, might be generated using \link getFiles \endlink
 * @param max_descriptors: Maximum amount of descriptors to load.
 * @param random: Randomly select descriptors.
 * @param seed: Random seed. If set to -1 current time is used.
 * @return: cv::Mat1f containing the concatenated descriptors, one descriptor per row.
 */
cv::Mat1f loadDescriptors(const std::vector<std::string> & files,
                          int max_descriptors = 150000,
                          bool random = true,
                          int seed = -1);

QStringList imagesFromDir( const QString & dir_name );

void readImages( const QString & input_folder,
                 QStringList & image_names,
                 QList<cv::Mat> & images,
                 float resize_factor = 1.0);

// note: we could also use push_back but should be faster that way
/**
 * @brief concatenate: Merges a vector of cv::Mat into one single cv::Mat object.
 * @param matrices: Vector of matrices.
 * @param merged: Output matrix.
 * @param vertical: If true matrices are concatenated in row direction, otherwise in column direction.
 */
void concatenate( const std::vector<cv::Mat> & matrices,
                  cv::Mat & merged,
                  bool vertical = true);

cv::Mat createOne(std::vector<cv::Mat> & images,
                  int cols,
                  int min_gap_size);

std::string getFilename(const std::string & outputdir,
                        const std::string & img_filename,
                        const std::string & identifier,
                        const std::string & ext = "png",
                        bool get_last = false);


/**
 * @brief dumpMatrices: Batch dump a vector of cv::Mat objects to file.
 * @param outputdir: Base directory to store the files in.
 * @param img_filename: Output filename.
 * @param identifier: Optional identifier which is added to the filename, separated by an underscore.
 * @param dump_matrices: std::vector<cv::Mat> containing the matrices to dump.
 * @param verbosity: How verbose the output should be.
 * @param out_type: \link OUTPUT_TYPE \endlink specifying the filetype of the output file.
 */
void dumpMatrices(const std::string & outputdir,
                  const std::string & img_filename,
                  const std::string & identifier,
                  const std::vector<cv::Mat> & dump_matrices,
                  int verbosity,
                  OUTPUT_TYPE out_type);

/**
 * @brief dumpMatrix: Dump a cv::Mat object to disk.
 * @param outputdir: Base directory to store the file in.
 * @param img_filename: Filename
 * @param identifier: Optional identifier which is added to the filename, separated by an underscore.
 * @param dump_matrix: cv::Mat object which is to be dumped.
 * @param verbosity: How verbose the output should be.
 * @param out_type: \link OUTPUT_TYPE \endlink specifying the filetype of the output file.
 */
std::string dumpMatrix( const std::string & outputdir,
                        const std::string & img_filename,
                        const std::string & identifier,
                        const cv::Mat & dump_matrix,
                        int verbosity,
                        OUTPUT_TYPE out_type);

// backwards compatibility
void dumpMatrices(const std::string & outputdir,
                  const std::string & img_filename,
                  const std::string & identifier,
                  const std::vector<cv::Mat> & dump_matrices,
                  int verbosity,
                  bool as_csv = false);

void dumpMatrix( const std::string & outputdir,
                 const std::string & img_filename,
                 const std::string & identifier,
                 const cv::Mat & dump_matrix,
                 int verbosity,
                 bool as_csv = false);

void convertCSVtoYAML( const std::string & outputdir,
                       const std::string & img_filename );

//Either I'm too blind to see, or there's really no such thing like range() in Python
template <typename T>
class UniqueNumber
{
private:
    T number;
    T stepSize;
    bool reverse;

public:
    UniqueNumber(T start = 0, T step = 1.0, bool rev = false)
    {
        reverse = rev;
        stepSize = abs(step);
        rev ? number = start+stepSize : number = start-stepSize;
    }
    T operator()() { return reverse ? (number-=stepSize) : (number+=stepSize); }
};

template <typename T>
class RandomNumber
{
public:
    RandomNumber()
    {
    }
    //Chosen by fair dice roll
    T operator()() { return 4; }
};

/**
 * Range template class.
 *
 * Provides static methods to generate a std::vector filled
 * with elements, either in ascending/descending order from
 * start to end, or with random values.
 */
template <typename T>
class Range
{
    public:
        static std::vector<T> unique(T from, T to, T step = 1.0)
        {
            if(!std::is_integral<T>::value) { return std::vector<T>(); }

            T delta = abs(to-from);
            T stepSize = abs(step);

            std::vector<T> ret;
            if(step == 0) {
                return ret;
            } else if(step != 1) {
                if(delta % stepSize == 0) {
                    ret.resize(delta/stepSize);
                } else {
                    ret.resize((delta/stepSize)+1);
                }
            } else {
                ret.resize(abs(to-from));
            }
            if(from == to) {
                return std::vector<T>();
            } else if(from > to) {
                std::generate(ret.begin(), ret.end(), UniqueNumber<T>(from, stepSize, true));
            } else {
                std::generate(ret.begin(), ret.end(), UniqueNumber<T>(from, stepSize));
            }
            return ret;
        }
};

/**
 * this is similar to the python nonzero function, it
 * returns indices of all matrix elements which are non-zero
 */
static void nonzero(const cv::Mat1b & mask, std::vector<cv::Point> & indices )
{
    for(int y=0; y < mask.rows; y++){
        for(int x = 0; x < mask.cols; x++){
            if ( mask(y,x) != 0 )
                indices.push_back(cv::Point(x,y));
        }
    }
}

} // namespace puhma

#endif // PUHMA_COMMON_H
