#ifndef ImgAnnotation_H
#define ImgAnnotation_H

#include <list>
#include <string>
#include <iterator>
#include <map>
//#include "ImgAnnotations.hpp"
#include <boost/functional/hash/hash.hpp>
#include <QString>
#include <QDomElement>
#include <QFile>

namespace IA
{
// forward declarations
class ImgAnnotations;
class PropertyMap;
class Object;
class File;
class Dir;

// typedefs
typedef int ID;
typedef int size_type;

typedef std::map<std::string, File*> 		StrFileMap;
typedef std::map<std::string, Dir*> 			StrDirMap;
typedef std::map<std::string, Object*> 		StrObjMap;
typedef std::map<std::string, std::string> 	StrStrMap;
typedef std::map<ID, Object*> 				IDObjMap;
typedef std::map<ID, std::string> 			IDStrMap;

typedef std::pair<std::string, std::string>		StrStrPair;

typedef std::list<std::string> 	StrList;
typedef std::list<ID> 			IDList;
typedef std::list<File*> 		FileList;
typedef std::list<Dir*> 		DirList;
typedef std::list<Object*> 		ObjList;
typedef std::list<PropertyMap*> PropList;

class ImgAnnotations
{
public:
	ImgAnnotations();
	~ImgAnnotations();

	/// adds ontology to the given qdomdocument
	void addOntologyXML( QDomDocument & dom_doc, QDomNode & node, const QStringList & properties );
	/// adds complete annotation database to the given qdomdoc
	void addAnnotationXML( QDomDocument & dom_doc, const QStringList & properties );
	/// load annotation-database from xml-file
	QStringList loadFromXML(const QDomDocument &anno_dom);
	QStringList loadFromFile(QFile &file);
	void saveToFile( QFile & fileName, const QStringList & properties );

	StrList getAllObjProperties() const;
	StrList getAllObjPropertyValues(const std::string& property) const;
	StrList getAllFileProperties() const;
	StrList getAllFilePropertyValues(const std::string& property) const;

	size_type numOfDirs() const;
	size_type numOfFiles(const std::string &dirPath = std::string()) const;
	size_type numOfObjects(const std::string &dirPath = std::string(), const std::string &fileName = std::string()) const;

	bool existsDir(const std::string &dirPath) const;
	bool existsFile(const std::string &filePath) const;
	bool existsObject(ID objID) const;

	StrList getDirPaths() const;
	StrList getFilePaths(const std::string &dirPath = std::string()) const;
	IDList getObjectIDs(const std::string &dirPath = std::string(), const std::string &fileName = std::string()) const;

	DirList getDirs() const;
	FileList getFiles(const std::string &dirPath = std::string()) const;
	ObjList getObjects(const std::string &dirPath = std::string(), const std::string &fileName = std::string()) const;

	IA::Object *getObject(ID objID) const;
	IA::File *getFile(const std::string &filePath) const;
	IA::Dir *getDir(const std::string &dirPath) const;

	IA::Object *newObject(File *file, ID objID);
	IA::Object *newObject(const std::string &filePath,
						  ID objID = -1);
	File *addFile(const std::string &filePath);
	void addFiles(const StrList &filePaths);
	void removeFiles(const StrList &);
	void removeFile(const std::string &);
	void removeObjects(const IDList &objIDList);
	void removeObject(ID objID);
	void removeProperty(const IDList & objIDList,
						std::string property);
	void clear();

	// split the path to a file into its dirPath and fileName
	static std::string dirPath(const std::string &filePath);
	static std::string fileName(const std::string &filePath);
	static std::string filePath(const std::string &dirPath,
								const std::string &fileName);

	void clearFilesAndObjects();

	/**
	 * \brief save zone annotations to filepath
	 *	first the header is added then all values of
	 *	bbox / fixpoints then the footer
	 */
	void saveXML(const QString & img_filepath,
					 QFile & zone_file,
					 QString header,
					 QString zone_snippet,
                     QString footer,
                     QString prefix) const;
    QString replaceSpecial(QString snippet,
                           Object *obj,
                           const QString &prefix,
                           const QString & value,
                           int width, int height) const;
    QString getXmlString(const QString &img_filepath,
                         QString header,
                         QString footer,
                         QString prefix,
                         bool zone);
private:
	// hash that stores the directories
	StrDirMap _dirs;
	// cache the highest object id
	ID _highestObjID;
	// cache the dir and file name for each object ID
	IDStrMap _objFilePaths;

	StrList getAllProperties(const PropList& propList) const;
	StrList getAllPropertyValues(const PropList& propList, const std::string& property) const;
	/// add element to a qdomdocument
	QDomElement addElement(QDomDocument & doc,
						   QDomNode & node,
						   const QString & tag,
						   const QString & value = QString::null);
};

class PropertyMap {
	friend class ImgAnnotations;

protected:
	StrStrMap _properties;

public:
	PropertyMap();
	bool isEmpty();

	// get/set methods for the object properties
	std::string get(const std::string &property) const;
	std::string getAs(const std::string& property) const;
	//		void set(const std::string &property, const T &value);
	template<typename T>
	T getAs(const std::string& property) const;
	template<typename T>
	void set(const std::string &property, const T& value);
	bool isSet(const std::string &property) const;
	void clear(const std::string &property);
	StrList getProperties() const;
	PropertyMap& operator=(const PropertyMap& p2);
};



class Dir {
	friend class ImgAnnotations;

private:
	StrFileMap _files;
	std::string _dirPath;

public:
	Dir(const std::string &dirPath);
	~Dir();
	std::string getDirPath() const;
	StrList getFileNames() const;
	FileList getFiles() const;
	size_type numOfFiles() const;
	IA::File *getFile(const std::string &fileName) const;
};



class File : public PropertyMap {
	friend class ImgAnnotations;

private:
	IDObjMap _objects;
	std::string _filePath;

public:
	File(const std::string &filePath);
	~File();
	std::string getFilePath() const;
	IDList getObjectIDs() const;
	ObjList getObjects() const;
	size_type numOfObjects() const;
	IA::Object *getObject(ID objID) const;
	int size() const { return _objects.size(); }
};



class Object : public PropertyMap {
	friend class ImgAnnotations;

private:
	ID _id;
	std::string _filePath;

public:
	Object(const std::string &filePath, ID id);
	ID getID() const;
	std::string getFilePath() const;
	Object& operator=(const Object& other);
};

} // namespace

#endif
