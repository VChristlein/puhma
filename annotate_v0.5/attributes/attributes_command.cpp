#include "attributes_command.h"
#include "attributes_core.h"
#include "puhma_common.h"

#include <opencv2/highgui/highgui.hpp>
#include <QString>
#include <QFileInfo>
#include <sstream>


namespace puhma{

AttributesCommand :: AttributesCommand(void): 
    Command(
		"attri", //attribute embed
		config,
		"Chengbiao Li",
        "chengbiao.li@fau.de")
{
}

int AttributesCommand :: execute(void) 
{
	if (config.verbosity >= 1){
		printConfig();		
	}
	
	// load images
	/*cv::Mat img;
	    if ( config.outputdir == "same" ) {
        config.outputdir = QFileInfo(QString::fromStdString(config.inputfile)).path().toStdString();
    }*/

	//the actual attribute embeding process is handled by the core-class
    AttributeEmbed ae(config);
    //cv::Mat out;
    //prepare the data
    ae.prepare();

    if ( config.generatePHOC ){
        ae.labelEmbed();
    }

    if ( config.generateFV ){
        ae.fvRep();
    }

    if ( config.trainAtt ){
        ae.trainAS();
    }

    if (config.calibrateMethod != ""){
        ae.calibrate();
        //ae.learn();
    }

    if (config.doRecog){
        ae.recognition();
    }

    if (config.doOnline){
        ae.evaOnline();
    }

	if ( config.outputdir.empty() )
		return 0;

    return 0;
}

void AttributesCommand :: printShortHelp(void) const 
{
	std::cout << "attributes embedding of the word image" << std::endl;
}

void AttributesCommand :: printHelp(void)  const 
{
	printShortHelp();	
}

void AttributesCommand :: printConfig(void) 
{
    config.printConfig();
}

}
