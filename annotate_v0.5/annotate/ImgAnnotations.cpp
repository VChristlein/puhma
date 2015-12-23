#include "ImgAnnotations.hpp"
#include "functions.h"

// C++ STD Lib
#include <string>
#include <iostream>
#include <fstream>
#include <cassert>
#include <algorithm>
#include <vector>
#include <set>

// Boost Lib
#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/foreach.hpp>
#include <boost/io/ios_state.hpp>

#include <QFileInfo>
#include <QDir>
#include <QFile>
#include <QDomDocument>
#include <QTextStream>
#include <QDebug>

// using namespaces
using namespace IA;
using std::string;
//using __gnu_cxx::hash_map;
using std::map;
using std::list;
using std::vector;
using std::cout;
using std::cerr;
using std::endl;
using boost::lexical_cast;
using boost::bad_lexical_cast;
using boost::algorithm::starts_with;
using boost::algorithm::trim;
using boost::algorithm::split;
using boost::algorithm::to_lower;
using boost::algorithm::to_lower_copy;
using boost::algorithm::is_any_of;
using boost::algorithm::contains;

// Stupid fix
#define BOOST_NO_EXCEPTIONS

//and add this somewhere in your code:
namespace boost
{
void throw_exception( std::exception const & e ) { assert(false); } // Maybe log the error...
}

// ========== ImgAnnotations ==========

IA::ImgAnnotations::ImgAnnotations()
{
	_highestObjID = 0;
}

IA::ImgAnnotations::~ImgAnnotations()
{
	// delete all allocated Dir references .. the rest is done automatically
	typedef std::pair<string, Dir*> thePair;
	BOOST_FOREACH(thePair entry, _dirs)
		delete entry.second;
}

QStringList IA::ImgAnnotations :: loadFromXML(const QDomDocument & anno_dom)
{
    // clear the current content
    clear();

	QDomElement root = anno_dom.namedItem("annotation").toElement();
	if ( root.isNull() || !root.hasChildNodes() ) {
		qWarning() << "Warning: couldn't find <annotation> Element\n";
		return QStringList();
	}

//	QDomElement e = root.namedItem("ontology").toElement();
//	if ( e.isNull() || !e.hasChildNodes() ) {
//		qWarning() << "Warning: couldn't find <ontology> Element\n";
//		return QStringList();
//	}
//	QStringList property_list = e.namedItem("propertylist").toElement().text().split(",");

	// TODO read in possible options
//		for ( ; !e.isNull(); e = e.nextSiblingElement("property"))

	// database with connected objects
	// foreach object-element
	for ( QDomElement file_node = root.firstChildElement("file");
		  !file_node.isNull();
		  file_node = file_node.nextSiblingElement("file"))
	{
		// TODO: maybe parse id
		// add file
        std::string file_path = file_node.attribute("filename").toStdString();
        if ( existsFile(file_path) )
            continue;
        File * current_file = addFile( file_path );
        assert( current_file != NULL );
//        qDebug() << "file" << current_file->getFilePath().c_str();

		// parse file-properties
		for ( QDomElement property_node = file_node.namedItem("fileproperties").firstChildElement();
			  !property_node.isNull();
			  property_node = property_node.nextSiblingElement() )
		{
//            qDebug() << "file property" << property_node.tagName();
			current_file->set( property_node.tagName().toStdString(),
							   property_node.text().toStdString() );
		}

		// foreach object-element
		for ( QDomElement dom_obj = file_node.firstChildElement("object");
			  !dom_obj.isNull();
			  dom_obj = dom_obj.nextSiblingElement("object"))
		{
			IA::ID id = dom_obj.attribute("id").toInt();
//            qDebug() << (int)id;
			Object * obj = newObject( current_file, id );
            assert( obj != NULL );
			// foreach property of the object
			for ( QDomElement property = dom_obj.firstChildElement();
				  !property.isNull();
				  property = property.nextSiblingElement() )
			{
//                qDebug() << property.tagName() << property.text();
				obj->set( property.tagName().toStdString(), property.text().toStdString() );
			}
		} // foreach child of file
	} // foreach child
//	return property_list;
	return QStringList();
}

QStringList IA::ImgAnnotations::loadFromFile( QFile & file )
{
	// clear the current content
	clear();

	// loop over all lines
	Object* currentObj = NULL;
	File* currentFile = NULL;
	string currentFilePath;
	ID currentObjID = -1;
	QStringList properties;
	QTextStream in( &file );
	int i = 0;
	for ( QString line = in.readLine(); !line.isNull(); line = in.readLine(), i++ ) {

		if ( i == 0 && line.contains("$properties$") ){
			properties = line.section(",", 1).split(",", QString::SkipEmptyParts);
			continue;
		}
		// ignore empty lines and comment lines
		if (line.isEmpty() || line.startsWith('#') || !line.contains(':')){
			continue;
		}

		// split the line in its key and value (up to ':' and the value)
		QStringList key_value = line.split( ":", QString::SkipEmptyParts );
        QString key = key_value.at(0).trimmed();
		QString value = key_value.at(1).trimmed();

		// decide what to do based on the key value
		if ( "file" == key ) {
			// add a new empty file .. if it doesn't exist already
            if ( existsFile( value.toStdString() ) )
                continue;
            currentFile = addFile( value.toStdString() );

			// update currentFile and currentObj (to zero, since we added a new file)
			currentFilePath = value.toStdString();
			currentObjID = -1;
			currentObj = 0;			
		}
		else if ( "object" == key ) {
			// we have a new object given
			if ( !currentFile )
				continue;

			// the object is given with its object id, parse its id and check for validity
			ID objID;
//			try {
				objID = lexical_cast<ID>(value.toStdString());
//			}
//			catch(bad_lexical_cast &e) {
//				std::cerr << "cannot cast id-value in file to the objID\n";
//				error = true;
//				objID = -1;
//			}
//			if (objID == 0 || objID < -1 || existsObject(objID)) {
//				error = true;
//				objID = -1;
//			}

			// add a new empty object
			currentObj = newObject(currentFilePath, objID);
			assert(NULL != currentObj);
			assert(existsObject(currentObj->getID()));
			currentObjID = currentObj->getID();
		}
		else {
			// if we have not seen an object tag so far, these are properties
			// for the image file
			if (!currentObj && currentFile) {
				currentFile->set(key.toStdString(), value.toStdString());
			}
			// otherwise these are properties for the object
			else if (currentObj)
				currentObj->set(key.toStdString(), value.toStdString());
		}
	}	
	return properties;
}

QDomElement IA::ImgAnnotations :: addElement( QDomDocument & doc,
                                              QDomNode & node,
                                              const QString & tag,
                                              const QString & value )
{
    QDomElement el = doc.createElement( tag );
    node.appendChild( el );

    if ( !value.isNull() ) {
        QDomText txt = doc.createTextNode( value );
        el.appendChild( txt );
    }
    return el;
}

void IA::ImgAnnotations :: addOntologyXML( QDomDocument & dom_doc,
                                           QDomNode & node,
                                           const QStringList & properties )
{
    QDomElement root = addElement(dom_doc, node, "ontology");
    addElement( dom_doc, root, "propertylist", properties.join(",") );
    foreach ( QString property, properties ) {
        QStringList values = std2qt( getAllObjPropertyValues(property.toStdString()) );
        if ( !values.empty() ){
//            QDomElement prop_val = addElement( ontology_dom, root, property );
            foreach( QString value, values ) {
                addElement( dom_doc, root, property, value );
            }
        }
    }
}

void IA::ImgAnnotations :: addAnnotationXML( QDomDocument & dom_doc,
                                             const QStringList & properties )
{
    QDomElement root = addElement( dom_doc, dom_doc, "annotation");
    // the ontology
//    addOntologyXML( dom_doc, root, properties );

    // the files
    BOOST_FOREACH( File *iFile, getFiles() ) {
        QDomElement file = addElement( dom_doc, root, "file");
        // TODO: replace with "real" id and optionally put filename as fileproperty
        file.setAttribute("filename", QFileInfo(iFile->getFilePath().c_str()).fileName());

        // general file properties
        if ( !iFile->getProperties().empty() ) {
           QDomElement file_properties = addElement(dom_doc, file, "fileproperties" );
            BOOST_FOREACH( const string& iProperty, iFile->getProperties()) {
                addElement(dom_doc, file_properties, iProperty.c_str(), iFile->get(iProperty).c_str() );
            }
        }

		// loop over the objects
		BOOST_FOREACH( Object *iObj, iFile->getObjects() ) {
			// output the object tag with its id
			assert(NULL != iObj);
			StrList property_list = iObj->getProperties();
			if ( property_list.empty() )
				continue;
			QDomElement obj = addElement(dom_doc, file, "object");
			obj.setAttribute("id", iObj->getID());
			// properties
			BOOST_FOREACH( const string& iProperty, property_list ) {
				if ( !iObj->get(iProperty).empty() ) {
					addElement(dom_doc, obj, iProperty.c_str(), iObj->get(iProperty).c_str());
				}
			}
		} // end for obj
	} // end for files
}

QString IA::ImgAnnotations:: replaceSpecial(QString snippet,
                                            Object *obj,
                                            const QString & prefix,
                                            const QString & value,
                                            int width,
                                            int height) const
{
    bool relative = false;
    if( snippet.contains(prefix+"rpoints"))
        relative = true;

    QList<QPointF> coords;
    if (obj->isSet("bbox")){
        QRectF rec = str2rect( QString::fromStdString(obj->get("bbox")) );
        coords.push_back(rec.topLeft());
        coords.push_back(rec.topRight());
        coords.push_back(rec.bottomRight());
        coords.push_back(rec.bottomLeft());
    } else if ( obj->isSet("fixpoints") ) {
        coords = str2points( QString::fromStdString(obj->get("fixpoints")) );
    }

    QString points = "";
    for(int i = 0; i < coords.size(); i++){
        QPointF point = coords[i];
        points += QString::number(relative ? (point.x() / width) : point.x());
        points += ",";
        points += QString::number(relative ? (point.y() / height) : point.y());
        if ( i != coords.size()-1 ) // not last one
            points += " ";
    }

    snippet.replace(prefix + "id", QString::number(obj->getID()));
    if ( relative )
        snippet.replace(prefix + "rpoints", points);
    else
        snippet.replace(prefix + "points", points);

    snippet.replace(prefix + "val", value);
    snippet.replace(prefix+"width", QString::number(width));
    snippet.replace(prefix+"height", QString::number(height));
    snippet.replace(prefix+"url", QString::fromStdString(obj->getFilePath()));
    snippet.replace(prefix+"filename", QString::fromStdString(obj->getFilePath()));

    return snippet;
}

QString IA::ImgAnnotations:: getXmlString(const QString & img_filepath,
										  QString header,
										  QString footer,
										  QString prefix,
										  bool zone)
{
	File* img_file = getFile(img_filepath.toStdString());
	if (img_file == NULL){
		qDebug() << "WARNING: IA::ImgAnnotations::saveZoneXML(): file == NULL; img_filepath:" << img_filepath;
		return "";
	}
	if (img_file->isEmpty()) {
		return "";
	}

    QString width_str = QString::fromStdString(img_file->get("width"));
    QString height_str = QString::fromStdString(img_file->get("height"));

    QString out = "";

    // replace now the placeholders
    header.replace(prefix + "width", width_str);
    header.replace(prefix + "height", height_str);
    header.replace(prefix + "url", QFileInfo(img_filepath).fileName());
    header.replace(prefix + "filename", QFileInfo(img_filepath).fileName());
    out += header;

	for(Object *obj : img_file->getObjects()) {
		if ( obj->isEmpty())
			continue;
		std::string snip;
		if( zone )
			snip = obj->get("zone");
		else
			snip = obj->get("snippet");
		out += QString::fromStdString(snip);
	}

    footer.replace(prefix + "width", width_str);
    footer.replace(prefix + "height", height_str);
    footer.replace(prefix + "url", QFileInfo(img_filepath).fileName());
    footer.replace(prefix + "filename", QFileInfo(img_filepath).fileName());

    out += footer;

    return out;
}

//void IA::ImgAnnotations :: saveXML( QFile & file, QString content ) const
//{
//
//	QTextStream out ( &file );
//	out <<

//}

void IA::ImgAnnotations :: saveToFile( QFile & file ,
                                       const QStringList & properties )
{		 	
    QTextStream out ( &file );

    if ( QFileInfo(file).suffix() == "xml" ) {
        QDomDocument dom_doc;
        QDomProcessingInstruction instr =
                dom_doc.createProcessingInstruction(
                    "xml", "version='1.0' encoding='UTF-8'" );
        dom_doc.appendChild(instr);

        addAnnotationXML( dom_doc, properties );

		dom_doc.save( out, 4 );
		return;
	}

	if ( !properties.empty() )
		out << "$properties$," << properties.join(",") << "\n";
	// loop over all elements in our datastructur and write their data to the file	
	BOOST_FOREACH (File *iFile, getFiles()) {
		// output the fileName
		assert(NULL != iFile);
		out << "########## NEW FILE ##########\n";
		out << "file: " << iFile->getFilePath().c_str() << "\n";
		
		// output file properties
		BOOST_FOREACH (const string& iProperty, iFile->getProperties())
				out << iProperty.c_str() << ": " << iFile->get(iProperty).c_str() << "\n";
		out << "\n";

		// loop over the properties
		BOOST_FOREACH (Object *iObj, iFile->getObjects()) {
			// output the object tag with its id
			assert(NULL != iObj);
			out << "object: " << iObj->getID() << "\n";

			// output the properties
			BOOST_FOREACH (const string& iProperty, iObj->getProperties())
				out << iProperty.c_str() << ": " << iObj->get(iProperty).c_str() << "\n";

			// additional empty line at the end of an object
			out << "\n";
		}
	}	
}

StrList IA::ImgAnnotations::getAllProperties(const IA::PropList& propList) const
{
	map<string, bool> propertiesHash;
	StrList outList;

	// go through all properties of all objects
	BOOST_FOREACH (const PropertyMap* propMap, propList) {
		assert(NULL != propMap);
		BOOST_FOREACH (const string& property, propMap->getProperties())
			if (!propertiesHash[property]) {
				// the first occurrence of the property, i.e. add it to the list
				propertiesHash[property] = true;
				outList.push_back(property);
			}
	}

	return outList;
}

StrList IA::ImgAnnotations::getAllPropertyValues(const IA::PropList& propList,
                                                 const string& property) const
{    
    std::set<std::string> values;
	StrList outList;    

    // go through all objects
    BOOST_FOREACH (const PropertyMap* propMap, propList) {
        assert(NULL != propMap);
        if (propMap->isSet(property)
                && !propMap->get(property).empty()
                && (values.find(propMap->get(property)) == values.end()) )
        {
            outList.push_back(propMap->get(property));
            values.insert(propMap->get(property));
        }
    }

    return outList;
}

StrList IA::ImgAnnotations::getAllObjProperties() const
{
	ObjList objList = getObjects();
	PropList propList;
	std::copy(objList.begin(), objList.end(), std::inserter(propList, propList.end()));
	return getAllProperties(propList);
}

StrList IA::ImgAnnotations::getAllObjPropertyValues(const string& property) const
{	
    ObjList objList = getObjects();
    PropList propList;
	std::copy(objList.begin(), objList.end(), std::inserter(propList, propList.end()));
	return getAllPropertyValues(propList, property);
}

StrList IA::ImgAnnotations::getAllFileProperties() const
{	
	FileList fileList = getFiles();
    PropList propList;
	std::copy(fileList.begin(), fileList.end(), std::inserter(propList, propList.end()));
	return getAllProperties(propList);
}

StrList IA::ImgAnnotations::getAllFilePropertyValues(const string& property) const
{	 
	FileList fileList = getFiles(property);
    PropList propList;
	std::copy(fileList.begin(), fileList.end(), std::inserter(propList, propList.end()));
	return getAllPropertyValues(propList, property);
}

size_type IA::ImgAnnotations::numOfDirs() const
{
	return _dirs.size();
}

size_type IA::ImgAnnotations::numOfFiles(const string &dirPath) const
{
	size_type num = 0;

	// if the dir is not given, return number of all existing files
	if (dirPath.empty()) {
		BOOST_FOREACH (const Dir *iDir, getDirs())
			num += iDir->numOfFiles();
	}
	// otherwise return number of all files in the specified directory
	else if (existsDir(dirPath)) {
		const Dir *dir = getDir(dirPath);
		assert(NULL != dir);
		num = dir->numOfFiles();
	}

	return num;
}

size_type IA::ImgAnnotations::numOfObjects(const string &dirPath,
										   const string &fileName) const
{
	size_type num = 0;

	// if the dir is not given, return number of all existing objects
	if (dirPath.empty()) {
		BOOST_FOREACH (const Dir *iDir, getDirs())
			BOOST_FOREACH (const File *iFile, iDir->getFiles())
				num += iFile->numOfObjects();
	}
	// if dir is given, but not file, return number of all objects in the specified directory
	else if (existsDir(dirPath) && fileName.empty()) {
		const Dir *dir = getDir(dirPath);
		assert(NULL != dir);
		BOOST_FOREACH (const File *iFile, dir->getFiles())
			num += iFile->numOfObjects();
	}
	// if dir and file are given, return number of all objects assigned to the specified file
    else if (existsFile(filePath(dirPath, fileName))) {
		const File *file = getFile(filePath(dirPath, fileName));
		assert(NULL != file);
		num = file->numOfObjects();
	}

	return num;
}

bool IA::ImgAnnotations::existsDir(const std::string &dirPath) const
{
	return NULL != getDir(dirPath);
}

bool IA::ImgAnnotations::existsFile(const std::string &filePath) const
{
	return NULL != getFile(filePath);
}

bool IA::ImgAnnotations::existsObject(ID objID) const
{
	return NULL != getObject(objID);
}

StrList IA::ImgAnnotations::getDirPaths() const
{
	StrList outList;
//	typedef std::pair<string, Dir*> thePair;
//	BOOST_FOREACH(const thePair entry, _dirs)
//		outList.push_back(entry.first);
	BOOST_FOREACH(Dir *dir, getDirs())
		outList.push_back(dir->getDirPath());
	return outList;
}

StrList IA::ImgAnnotations::getFilePaths(const string &dirPath) const
{
	StrList outList;

//	// if the dir is not given, return all existing files
//	if (dirPath.empty()) {
//		BOOST_FOREACH (const Dir *iDir, getDirs())
//			BOOST_FOREACH (const File *iFile, iDir->getFiles())
//				outList.push_back(iFile->getFilePath());
//	}
//	// otherwise return all files in the specified directory
//	else if (existsDir(dirPath)) {
//		const Dir *dir = getDir(dirPath);
//		assert(NULL != dir);
//		BOOST_FOREACH (const File *iFile, dir->getFiles())
//			outList.push_back(iFile->getFilePath());
//	}
	BOOST_FOREACH(File *file, getFiles(dirPath))
		outList.push_back(file->getFilePath());

	return outList;
}

IDList IA::ImgAnnotations::getObjectIDs(const string &dirPath,
										const string &fileName) const
{
	IDList outList;

	BOOST_FOREACH(Object *obj, getObjects(dirPath, fileName))
		outList.push_back(obj->getID());

	return outList;
}

DirList IA::ImgAnnotations::getDirs() const
{
	DirList dirList;
	typedef std::pair<string, Dir*> thePair;
	BOOST_FOREACH (const thePair entry, _dirs)
		dirList.push_back(entry.second);
	return dirList;
}

FileList IA::ImgAnnotations::getFiles(const string &dirPath) const
{
	FileList fileList;

	// if the dir is not given, return all existing files
	if (dirPath.empty()) {
		BOOST_FOREACH (Dir *iDir, getDirs()) {
			FileList tmpList = iDir->getFiles();
			fileList.splice(fileList.end(), tmpList);
		}
	}
	// otherwise return all files in the specified directory
	else if (existsDir(dirPath)) {
		Dir *dir = getDir(dirPath);
		assert(NULL != dir);
		return dir->getFiles();
	}

	return fileList;
}

ObjList IA::ImgAnnotations::getObjects(const string &dirPath,
									   const string &fileName) const
{
	ObjList objList;

	// if the dir is not given, return number of all existing objects
	if (dirPath.empty()) {
		BOOST_FOREACH (Dir *iDir, getDirs())
			BOOST_FOREACH (File *iFile, iDir->getFiles()) {
				ObjList tmpList = iFile->getObjects();
				objList.splice(objList.end(), tmpList);
			}
	}
	// if dir is given, but not file, return number of all objects in the specified directory
	else if (existsDir(dirPath) && fileName.empty()) {
		Dir *dir = getDir(dirPath);
		assert(NULL != dir);
		BOOST_FOREACH (File *iFile, dir->getFiles()) {
			ObjList tmpList = iFile->getObjects();
			objList.splice(objList.end(), tmpList);
		}
	}
	// if dir and file are given, return number of all objects assigned to the specified file
	else if (existsFile(filePath(dirPath, fileName))) {
		File *file = getFile(filePath(dirPath, fileName));
		assert(NULL != file);
		return file->getObjects();
	}

	return objList;
}

IA::Object *IA::ImgAnnotations::getObject(ID objID) const
{
	Object *obj = NULL;

	// get the pointer to the corresponding file
	IDStrMap::const_iterator iFilePath = _objFilePaths.find(objID);
    if ( iFilePath != _objFilePaths.end() ) {
        File *file = getFile( iFilePath->second );
        assert( NULL != file ); // should exist since it is in our cache

		// get the pointer to the corresponding object
		obj = file->getObject(objID);
        assert( NULL != obj ); // should exist since it is in our cache
	}

	return obj;
}

IA::File* IA::ImgAnnotations::getFile(const string &filePath) const
{
	File *file = NULL;

	// get the file name and the dir path
	string dirPath = this->dirPath(filePath);
	string fileName = this->fileName(filePath);

	// get the pointer to the corresponding dir
	Dir *dir = getDir(dirPath);

	// get the pointer to the corresponding file
    if ( NULL != dir ) {
		file = dir->getFile(fileName);
    }

	return file;
}

IA::Dir *IA::ImgAnnotations::getDir(const string &dirPath) const
{
	Dir *dir = NULL;
	StrDirMap::const_iterator iDirPath = _dirs.find(dirPath);
	if (iDirPath != _dirs.end())
		dir = iDirPath->second;
	return dir;
}

IA::Object *IA::ImgAnnotations::newObject(File *file, ID objID)
{
	Object *newObj = NULL;

	// get the object ID
	if (objID < 0)
		objID = ++_highestObjID;
	else if (objID > _highestObjID)
		_highestObjID = objID;

	// ensure that the object ID does not exist in the file
    assert( NULL == file->getObject(objID) );

	// add the new object in the system
	newObj = new Object( file->getFilePath(), objID );
	file->_objects[objID] = newObj;
	_objFilePaths[objID] = file->getFilePath();
	return newObj;
}

IA::Object *IA::ImgAnnotations::newObject(const string &filePath, ID objID)
{
    assert( objID < 0 || !existsObject(objID) );

	// create a new object only if its file exists in our database
	// and only if its ID is valid (i.e. not used or automatically chosen)
    if ( !existsFile(filePath) )
		addFile(filePath);
	File *file = getFile(filePath);
    assert( NULL != file );

	return newObject( file, objID );
}

File* IA::ImgAnnotations::addFile(const string& filePath)
{
	// only add the file if the file does not already exist
    if ( filePath.empty() )
		return NULL;

	string dirPath = this->dirPath(filePath);
	string fileName = this->fileName(filePath);

	// add a new directory if it does not already exist
	// otherwise get its reference
	Dir *dir = NULL;
    if ( !existsDir(dirPath) ) {
		dir = new Dir(dirPath);
		_dirs[dirPath] = dir;
	}
	else
		dir = getDir(dirPath);

	// add the file
    File * f = new File( filePath );
	dir->_files[fileName] = f;

	return f;
}

void IA::ImgAnnotations::addFiles(const StrList& filePaths)
{
	// add all given files to the database
	BOOST_FOREACH (string filePath, filePaths)
		addFile(filePath);
}

void IA::ImgAnnotations::removeFile(const string &filePath)
{
	// only remove the file if the file does exist
	if (existsFile(filePath)) {
		string fileName = this->fileName(filePath);
		string dirPath = this->dirPath(filePath);

		// get the file and its directory
		File *file = getFile(filePath);
		assert(NULL != file);
		Dir *dir = getDir(dirPath);
		assert(NULL != dir);

		// get all object ids and remove them from our cache
		BOOST_FOREACH (ID id, file->getObjectIDs()) {
			IDStrMap::size_type nRemoves = _objFilePaths.erase(id);
			assert(nRemoves == 1);
		}

		// delete the file entry from its directory
		StrFileMap::size_type nRemoves = dir->_files.erase(fileName);
		assert(nRemoves == 1);
		delete file;

		// delete the directory in case it is now empty
		if (dir->numOfFiles() <= 0) {
			assert(NULL != getDir(dirPath));
			StrDirMap::size_type nRemoves = _dirs.erase(dirPath);
			assert(nRemoves == 1);
			delete dir;
		}
	}
}

void IA::ImgAnnotations::removeFiles(const StrList &filePaths)
{
	// remove all given files from the database
	BOOST_FOREACH (string filePath, filePaths)
		removeFile(filePath);
}

void IA::ImgAnnotations::clearFilesAndObjects()
{
	_dirs.clear();
	_highestObjID = 0;
	_objFilePaths.clear();

	// remove all given files from the database
//	BOOST_FOREACH (string filePath, filePaths)
//		removeFile(filePath);
}

void IA::ImgAnnotations::removeObject(ID objID)
{
    if ( existsObject(objID) ) {
		Object *obj = getObject(objID);
        assert( NULL != obj );
        File *file = getFile( obj->getFilePath() );

		// delete object id from our cache
		IDStrMap::size_type nRemoves = _objFilePaths.erase(objID);
		assert(nRemoves == 1);

		// delete the object entry from its file
		nRemoves = file->_objects.erase(objID);
        assert( nRemoves == 1 );
		delete obj;
	}
}

void IA::ImgAnnotations::removeProperty(const IDList & objIDList,
                                        std::string property)
{
    BOOST_FOREACH (ID objID, objIDList) {
        Object* obj = getObject(objID);        
        obj->clear(property);
    }
}

void IA::ImgAnnotations::removeObjects(const IDList &objIDList)
{
	// remove all given objects from the database
	BOOST_FOREACH (ID objID, objIDList)
		removeObject(objID);
}

void IA::ImgAnnotations::clear()
{
	_dirs.clear();
	_objFilePaths.clear();
	_highestObjID = 0;
}

string IA::ImgAnnotations::dirPath(const string &filePath)
{    
    string dirpath = QFileInfo( QString::fromStdString(filePath) ).path().toStdString();
    return dirpath;
}

string IA::ImgAnnotations::fileName(const string &filePath)
{
    return QFileInfo(QString::fromStdString(filePath)).fileName().toStdString();
}

string IA::ImgAnnotations::filePath(const string &dirPath,
                                    const string &fileName)
{
    string filePath = QFileInfo(QDir(QString::fromStdString(dirPath)),
                                QString::fromStdString(fileName)).filePath().toStdString();
    return filePath;
}



// ========== PropertyMap ==========

IA::PropertyMap::PropertyMap()
{
	// empty
}

bool IA::PropertyMap::isEmpty()
{
	return _properties.empty();
}

string IA::PropertyMap::get(const string &property) const
{
	string lowProperty = to_lower_copy(property);
	StrStrMap::const_iterator i = _properties.find(lowProperty);
	if (i == _properties.end())
		return string("");
	else
		return i->second;
}

bool IA::PropertyMap::isSet(const string &property) const
{
	string lowProperty = to_lower_copy(property);
	StrStrMap::const_iterator i = _properties.find(lowProperty);
	return i != _properties.end();
}

void IA::PropertyMap::clear(const std::string &property)
{
	string lowProperty = to_lower_copy(property);
	_properties.erase(lowProperty);
}

StrList IA::PropertyMap::getProperties() const
{
	StrList outList;
	typedef std::pair<string, string> thePair;
	BOOST_FOREACH(const thePair entry, _properties)
		outList.push_back(entry.first);
	return outList;
}

IA::PropertyMap& IA::PropertyMap::operator=(const IA::PropertyMap& p2)
{
	_properties = p2._properties;
	return *this;
}



// ========== Dir ==========

IA::Dir::Dir(const string &dirPath)
{
	_dirPath = dirPath;
}

IA::Dir::~Dir()
{
	// delete all allocated File references .. the rest is done automatically
	typedef std::pair<string, File*> thePair;
	BOOST_FOREACH(thePair entry, _files)
		delete entry.second;
    _files.clear();
}

string IA::Dir::getDirPath() const
{
	return _dirPath;
}

StrList IA::Dir::getFileNames() const
{
	StrList outList;
	typedef std::pair<string, File*> thePair;
	BOOST_FOREACH(const thePair entry, _files)
		outList.push_back(entry.first);
	return outList;
}

FileList IA::Dir::getFiles() const
{
	FileList outList;
	typedef std::pair<string, File*> thePair;
	BOOST_FOREACH(const thePair entry, _files)
		outList.push_back(entry.second);
	return outList;
}

size_type IA::Dir::numOfFiles() const
{
	return _files.size();
}

IA::File *IA::Dir::getFile(const string &fileName) const
{
	File *file = NULL;
	StrFileMap::const_iterator iFile = _files.find(fileName);
	if (iFile != _files.end())
		file = iFile->second;
	return file;
}


// ========== File ==========

IA::File::File(const string &filePath)
{
	_filePath = filePath;
}

IA::File::~File()
{
	// delete all allocated Object references .. the rest is done automatically
	typedef std::pair<int, Object*> thePair;
	BOOST_FOREACH(thePair entry, _objects)
		delete entry.second;
    _objects.clear();
}

size_type IA::File::numOfObjects() const
{
	return _objects.size();
}

string IA::File::getFilePath() const
{
	return _filePath;
}

IDList IA::File::getObjectIDs() const
{
	IDList outList;
	typedef std::pair<ID, Object*> thePair;
	BOOST_FOREACH(const thePair entry, _objects)
		outList.push_back(entry.first);
	return outList;
}

ObjList IA::File::getObjects() const
{
	ObjList outList;
	typedef std::pair<ID, Object*> thePair;
	BOOST_FOREACH(const thePair entry, _objects)
		outList.push_back(entry.second);
	return outList;
}

IA::Object *IA::File::getObject(ID objID) const
{
	Object *obj = NULL;
	IDObjMap::const_iterator iObj = _objects.find(objID);
	if (iObj != _objects.end())
		obj = iObj->second;
	return obj;
}


// ========== Object ==========

IA::Object::Object(const string &filePath, ID id)
{
	_id = id;
	_filePath = filePath;
}

ID IA::Object::getID() const
{
	return _id;
}

string IA::Object::getFilePath() const
{
	return _filePath;
}

Object& IA::Object::operator=(const Object& other)
{
	if (this != &other) {
		_filePath = other._filePath;
		_properties = other._properties;
	}
	return *this;
}
