#include "MainWindow.h"

#include <QtDebug>
#include <QFileDialog>
#include <QHeaderView>
#include <QMessageBox>
#include <QAction>
#include <QMenu>
#include <QToolBox>
#include <QToolButton>
#include <QToolBar>
#include <QDoubleSpinBox>
#include <QErrorMessage>
#include <QTime>
#include <QTimer>

#include <QDebug>
#include <QInputDialog>

#include <iostream>
#include <sstream>
#include <boost/foreach.hpp>

#include "functions.h"
#include "ScrollAreaNoWheel.h"
#include "AnnotationsPixmapWidget.h"
#include "propertydialog.h"
#include "imageChoper.h"

#include "prepareWizard.h"
#include "recogWizard.h"
#include "xmldialog.h"
#include "snippetsdialog.h"

using namespace IA;
using std::string;


MainWindow::MainWindow(QWidget *parent, Qt::WFlags flags)
	: QMainWindow(parent, flags)
{
	// set up the GUI
	setupUi(this);
	scrollArea = new ScrollAreaNoWheel(this);
	pixmapWidget = new AnnotationsPixmapWidget(&annotations, scrollArea, scrollArea);
	scrollArea->setWidgetResizable(true);
	scrollArea->setWidget(pixmapWidget);
	setCentralWidget(scrollArea);

//	fileDockWidget->setVisible(false);
//	propertiesTableWidget->horizontalHeader()->setResizeMode(QHeaderView::Stretch);

	propertyList << "<ID>";
	propertyList << "snippet";
	propertyList << "zone";
	propertyList << "snippet_raw";
	propertyList << "value";

	// theses properties cannot be used/edited/seen directly by the user in the table view
	reservedProperties << "bbox" << "fixpoints";
	copiedObj = NULL;    
    allow_zoom  = false;

	// we want to receive key events, therefore we have to set the focus policy
	setFocusPolicy(Qt::WheelFocus);

    // tool-bar
    actionSaveDatabase->setDisabled(true);
    toolBar->addAction(actionOpenDatabase);
    toolBar->addAction(actionSaveDatabase);
    toolBar->addSeparator();
    toolBar->addAction(actionBrowsingMode);
    toolBar->addAction(actionCreateRectangle);
    toolBar->addAction(actionCreatePolygon);
    toolBar->addSeparator();
    toolBar->addAction(actionZoomIn);
    toolBar->addAction(actionZoomOut);    
    // the zoom handler
    zoomSpinBox = new QDoubleSpinBox(imgDockWidgetContents);
    zoomSpinBox->setObjectName(QString::fromUtf8("zoomSpinBox"));
    zoomSpinBox->setMinimum(0.01);
    zoomSpinBox->setMaximum(19.99);
    zoomSpinBox->setSingleStep(0.1);
    zoomSpinBox->setValue(1);
    // add it to the toolbar
    toolBar->addWidget(zoomSpinBox);

    // -- CONNECTIONS

    // zoom
//    connect(this, SIGNAL(zoomFactorChanged(double)), zoomSpinBox, SLOT(setValue(double)))
    connect(zoomSpinBox, SIGNAL(valueChanged(double)), pixmapWidget, SLOT(setZoomFactor(double)) );
    connect(pixmapWidget, SIGNAL(zoomFactorChanged(double)), zoomSpinBox, SLOT(setValue(double)) );
    connect(pixmapWidget, SIGNAL(zoomFactorChanged(double)), this, SLOT(checkZoom(double)) );
    connect(scrollArea, SIGNAL(wheelTurned(QWheelEvent*)), this, SLOT(onScrollAreaWheelTurned(QWheelEvent *)) );

    // tell pixmapwidget where to draw the pasted object
    connect(this, SIGNAL(pasteObj(IA::ID)), pixmapWidget, SLOT(onPasteObj(IA::ID)) );        

    // a new object shall be created
    connect( pixmapWidget, SIGNAL(createNewObject()), this, SLOT(addObj()) );

	connect(pixmapWidget, SIGNAL(activeObjectChanged(IA::ID)), this, SLOT(onPixmapWidgetActiveObjectChanged(IA::ID)));
	connect(this, SIGNAL(activeObjectChanged(IA::ID)), pixmapWidget, SLOT(onActiveObjectChanged(IA::ID)));
    connect(this, SIGNAL(activeObjectChanged(IA::ID)), this, SLOT(onPixmapWidgetActiveObjectChanged(IA::ID)));
    connect(this, SIGNAL(selectedObjectsChanged(const IA::IDList &)), pixmapWidget, SLOT(onVisibleObjectsChanged(const IA::IDList &)));

    connect(this, SIGNAL(sthChanged()), this, SLOT(onSthChanged()));
    connect(pixmapWidget, SIGNAL(sthChanged()), this, SLOT(onSthChanged()));

    // connect visibility status
    connect(objDockWidget, SIGNAL(visibilityChanged(bool)), actionShowObjectProperties, SLOT(setChecked(bool)) );
    connect(imgDockWidget, SIGNAL(visibilityChanged(bool)), actionShowImageFileBrowser, SLOT(setChecked(bool)) );
    connect(xmlDockWidget, SIGNAL(visibilityChanged(bool)), actionShowXmlPreview, SLOT(setChecked(bool)) );
    connect(snippetDockWidget, SIGNAL(visibilityChanged(bool)), actionShowSnippetMenu, SLOT(setChecked(bool)) );
//    connect(fileDockWidget, SIGNAL(visibilityChanged(bool)), actionShowImageProperties, SLOT(setChecked(bool)) );

    // remove-menu for objDockWidget
    objTableWidget->horizontalHeader()->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(objTableWidget->horizontalHeader(), SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(showObjTableContextMenu(const QPoint)) );
    connect(objTableWidget, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(showObjTableContextMenu(const QPoint)) );

    // quick edit menu
    connect(pixmapWidget, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(showObjContextMenu(QPoint)) );
    // quick obj-propery menu
    //connect(pixmapWidget, SIGNAL(requestQuickPropertyMenu()), this, SLOT(showQuickPropertyMenu()) );

	showPropertyMenuAction = new QAction(pixmapWidget);
	showPropertyMenuAction->setShortcut( Qt::Key_Space );
	showPropertyMenuAction->setShortcutContext(Qt::WidgetShortcut);
	pixmapWidget->addAction(showPropertyMenuAction);
	connect(showPropertyMenuAction,
			SIGNAL(triggered()),
			this,
			SLOT(on_actionPropertydialog_triggered())
			);


    QString xml_header = "<?xml version='1.0' encoding='UTF-8'?>\n";
    QString zone_header = "<?xml version='1.0' encoding='UTF-8'?>\n";
    xml_options = new XmlOptions(this, xml_header, "",
                                 zone_header, "", "",
                                 "%", false);

    current_snippet = 0;
    snippets_filename = "";
    loadSerialization();
    snippet_text.resize(snippetSelectionTableWidget->columnCount()*snippetSelectionTableWidget->rowCount());
    snippet_labels.resize(snippet_text.size());

    loadDatabase(database_filename, false);

    //TODO::initial attri_config DONE
    //      load ini DONE
    //      check file existence DONE?
    //      if exist DONE?
    //          create instance. DONE?
    initAEConfig();
    loadSerialization4Reco();
    ae = new puhma::AttributeEmbed(*attri_config);
    //attri_config = nullptr;
    //ae = nullptr;
    rm = nullptr;
    //firstTimeOpenReco = true;

//    snippetTextEdit->setText(snippet_text[0]);


//    snippetSelectionTableWidget->setCurrentCell(0,0);
//    snippetSelectionTableWidget->setItem(0,0,new QTableWidgetItem(""));
//    snippetSelectionTableWidget->currentItem()->setText("New XML");
//    snippetSelectionTableWidget->adjustSize();

//	// trigger autosave every 1 min
//	autosave = new QTimer(this);
    //	connect( autosave, SIGNAL(timeout()), this, SLOT(onAutosave()) );
}

MainWindow::~MainWindow()
{}

void MainWindow :: loadSerialization()
{
    QFile file( "annotation.ini" );
    if ( !file.open(QFile::ReadOnly | QFile::Text) ) {
        return;
    }
    QDataStream in(&file);
    in >> database_filename;
    in >> snippets_filename;
    in >> snippet_labels;
    in >> snippet_text;
    refreshSnippetSelectionTable();

    QString tmp;
    in >> tmp;
    xml_options->setXml_header(tmp);
    in >> tmp;
    xml_options->setXml_footer(tmp);
    in >> tmp;
    xml_options->setZone_header(tmp);
    in >> tmp;
    xml_options->setZone_snippet(tmp);
    in >> tmp;
    xml_options->setZone_footer(tmp);
    in >> tmp;
    xml_options->setPrefix(tmp);
    bool one_file;
    in >> one_file;
    xml_options->setOne_file(one_file);

    in >> snippet_extraction_folder;
    file.close();
}


void MainWindow :: saveSerialization()
{
    QFile file( "annotation.ini" );
    if ( !file.open(QFile::WriteOnly | QFile::Text) ) {
        return;
    }
    QDataStream out(&file);
    out << database_filename;
    out << snippets_filename;
    out << snippet_labels;
    out << snippet_text;
    out << xml_options->getXmlHeader();
    out << xml_options->getXmlFooter();
    out << xml_options->getZone_header();
    out << xml_options->getZone_snippet();
    out << xml_options->getZone_footer();
    out << xml_options->getPrefix();
    out << xml_options->getOne_file();
    out << snippet_extraction_folder;
    file.close();
}

void MainWindow :: loadSerialization4Reco()
{
    if(attri_config == nullptr){
        return;
    }
    QFile file("anno_reco.ini");
    if ( !file.open(QFile::ReadOnly | QFile::Text) ) {
        return;
    }
    QDataStream in(&file);
    QString tmp;
    in >> tmp;
    attri_config->outputdir = tmp.toStdString();
    in >> tmp;
    attri_config->inputfolder = tmp.toStdString();
    in >> tmp;
    attri_config->dataset = tmp.toStdString();
    in >> tmp;
    attri_config->transcriptionfile = tmp.toStdString();
    in >> tmp;
    attri_config->divisionTrainfile = tmp.toStdString();
    in >> tmp;
    attri_config->divisionTestfile = tmp.toStdString();
    in >> tmp;
    attri_config->divisionValifile = tmp.toStdString();
    in >> attri_config->digitalInPHOC;
    in >> attri_config->heightIm;
    in >> attri_config->numWordsTranGMM;
    in >> attri_config->dimPCA;
    in >> attri_config->clusterGMM;
    in >> attri_config->numSpatialX;
    in >> attri_config->numSpatialY;
    in >> attri_config->dimCCA;
    in >> attri_config->discardThreshold;
    file.close();
}

void MainWindow :: saveSerialization4Reco(){
    if(attri_config == nullptr){
        return;
    }
    QFile file("anno_reco.ini");
    if (!file.open(QFile::WriteOnly | QFile::Text)) {
        return;
    }
    QDataStream out(&file);
    out << QString::fromStdString(attri_config->outputdir);
    out << QString::fromStdString(attri_config->inputfolder);
    out << QString::fromStdString(attri_config->dataset);
    out << QString::fromStdString(attri_config->transcriptionfile);
    out << QString::fromStdString(attri_config->divisionTrainfile);
    out << QString::fromStdString(attri_config->divisionTestfile);
    out << QString::fromStdString(attri_config->divisionValifile);
    out << attri_config->digitalInPHOC;
    out << attri_config->heightIm;
    out << attri_config->numWordsTranGMM;
    out << attri_config->dimPCA;
    out << attri_config->clusterGMM;
    out << attri_config->numSpatialX;
    out << attri_config->numSpatialY;
    out << attri_config->dimCCA;
    out << attri_config->discardThreshold;
    file.close();
}

void MainWindow :: loadSnippets(const QString & file_name,
								bool show_error )
{
    if (file_name.isEmpty()) return;

	QFile file( file_name );
	if ( !file.open(QFile::ReadOnly | QFile::Text) ) {
		if ( show_error ) {
			QErrorMessage * error = new QErrorMessage(this);
			error->showMessage("Error opening File");
		}
		return;
	}
	QDataStream in(&file);
	in >> snippet_text;
	file.close();
	refreshSnippetSelectionTable();
}

void MainWindow :: saveSnippets( const QString & snippet_path)
{
    // let's block every signal while saving
    bool status_main = blockSignals(true);
    bool status_pix = pixmapWidget->blockSignals(true);
    bool status_scr = scrollArea->blockSignals(true);
    bool status_zoom = zoomSpinBox->blockSignals(true);
    QApplication::setOverrideCursor(Qt::WaitCursor);

    QFile file(snippet_path);
    if ( !file.open(QFile::WriteOnly | QFile::Text) ) {
        QErrorMessage * error = new QErrorMessage(this);
        error->showMessage("Couldn't open file" + snippet_path);
        QApplication::restoreOverrideCursor();
        statusBar()->showMessage("");
        return;
    }

    QDataStream out(&file);
    out << snippet_text;
    file.close();

    QApplication::restoreOverrideCursor();
    zoomSpinBox->blockSignals(status_zoom);
    scrollArea->blockSignals(status_scr);
    pixmapWidget->blockSignals(status_pix);
    blockSignals(status_main);
}

QString MainWindow::currentFilePath() const
{    

    QTreeWidgetItem *current = imgTreeWidget->currentItem();
    if ( NULL == current )
        return QString();
    QString dir = item2dir.at(current);  
    QString file_name = current->text(0);

    // can also be a relative path
    return QFileInfo(dir, file_name).filePath();
}

IA::File* MainWindow::currentFile() const
{
    return annotations.getFile( currentFilePath().toStdString() );
}

ID MainWindow::currentObjectID() const
{
	// hand the active object over to the PixmapWidget
	return objectIDAtRow( objTableWidget->currentRow() );
}

Object *MainWindow::currentObject() const
{
	return annotations.getObject( currentObjectID() );
}

IA::ID MainWindow::objectIDFromItem( QTableWidgetItem *item ) const
{
	if (NULL == item)
		return -1;
	return objectIDAtRow(item->row());
}

IA::ID MainWindow::objectIDAtRow(int iRow) const
{

	// query the header item of the given row, it holds the object id
	QTableWidgetItem *item = objTableWidget->item(iRow, 0);
	if ( NULL == item )
		return -1;

	return static_cast<ID>(item->text().toInt());
	
}

void MainWindow::setCurrentTableItem(ID objID)
{
    // find the item in the TableWidget with the correct object id
	QTableWidgetItem* foundItem = NULL;
	QString idStr = QString::number(objID);
	for (int i = 0; i < objTableWidget->rowCount() && foundItem == NULL; ++i) {
		QTableWidgetItem* tmpItem = objTableWidget->item(i, 0);
		if (NULL != tmpItem && tmpItem->text() == idStr)
			foundItem = tmpItem;
	}
	if (NULL != foundItem) {
		objTableWidget->clearSelection();
		objTableWidget->setCurrentItem(foundItem);
	}
}

// TODO: remove me?
//void MainWindow::onObjectContentChanged(ID objID)
//{
//	// check wether dir/file/object have been selected
//	QString iFile = currentFile();
//	QString iDir = currentDir();
//	int iObj = currentObj();
//	if (iFile.isEmpty() || iDir.isEmpty() || iObj < 0)
//		return;
//
//	// set the new bounding box values
//	IAObj *obj = annotations.getObject(iDir, iFile, iObj);
//	if (obj) {
//		obj->box = newBox.box;
//		obj->fixPoints = newBox.fixPoints;
//	}
//}

void MainWindow::onPixmapWidgetActiveObjectChanged(ID objID)
{
	// if the id is < 0, clear the selection
	if (objID < 0) {
		objTableWidget->setCurrentItem(NULL);
		objTableWidget->clearSelection();
        actionRemoveObj->setText("Remove Object");
        actionRemoveObj->setEnabled(false);
	}
	// otherwise find the correct item in the QTableWidget
	else {
        QTableWidgetItem *item = NULL;
		for (int i = 0; i < objTableWidget->rowCount(); i++) {
            item = objTableWidget->item(i, 0);
			ID tmpID = objectIDFromItem(item);
			if (tmpID == objID) {
				// we found the right item
                objTableWidget->blockSignals(true);
				objTableWidget->clearSelection();
				objTableWidget->setCurrentCell(i, 0);
				objTableWidget->blockSignals(false);
				break;
			}
        }
        if ( item != NULL ){
            actionRemoveObj->setText("Remove Object '" + objTableWidget->item(item->row(), 0)->text());
            actionRemoveObj->setEnabled(true);
        }        
	}
}

void MainWindow::onScrollAreaWheelTurned(QWheelEvent *event)
{
	wheelEvent(event);
}

void MainWindow :: loadDatabase( const QString & file_name,
								 bool show_error )
{
    if ( file_name.isEmpty() ) return;

	QFile file( file_name );
	if ( !file.open(QFile::ReadOnly | QFile::Text) ) {
		if ( show_error ) {
			QErrorMessage * error = new QErrorMessage(this);
			error->showMessage("Error opening File");
		}
		return;
	}

	QStringList properties_candidate;
	// try to parse
	if ( QFileInfo(file).suffix() == "xml" ){
		// parse xml
		QString error_msg;
		int error_line, error_column;
		QDomDocument anno_dom;
		if ( ! anno_dom.setContent( &file, &error_msg, &error_line, &error_column ) )
		{
			if ( show_error ) {
				QErrorMessage * error = new QErrorMessage(this);
				std::stringstream ss;
				ss << "Error in parsing XML file:\n"
				   << "Explanatory string: " << error_msg.toStdString() << '\n'
				   << " at line: " << error_line << " at column " << error_column << '\n';
				error->showMessage( QString(ss.str().c_str()) );
			}
			return;
		}
		// parse xml format
		properties_candidate = annotations.loadFromXML( anno_dom );
	}
	else {
		properties_candidate = annotations.loadFromFile( file );
	}

	// create a list with all properties
	if ( !properties_candidate.empty() ){
		propertyList = properties_candidate;
	}
	// can this actually happen any more?
	else {
		propertyList = std2qt(annotations.getAllObjProperties());
	}
	propertyList.prepend("<ID>");
	foreach (QString property, reservedProperties)
		propertyList.removeAll(property);

	// create a list with file properties
	filePropertyList = std2qt(annotations.getAllFileProperties());

	// update the window title
	setWindowTitle("ImageAnnotation - " + QFileInfo(file).baseName());

	// update the statusbar
	statusBar()->showMessage("Opened database file " + file_name, 5 * 1000);

	// save the last filepath
    database_path = QFileInfo(file).absolutePath();
    database_filename = file_name;
	saveSerialization();

	refreshImgView("");

//	// trigger now every minute an autosave
//	autosave->start( 1000 * 60 );
}

void MainWindow::on_actionNewDatabase_triggered()
{
    if ( actionSaveDatabase->isEnabled() ){
        QMessageBox msgBox;
        msgBox.setText("The document has been modified.");
        msgBox.setInformativeText("Do you want to save your changes?");
        msgBox.setStandardButtons(QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);
        msgBox.setDefaultButton(QMessageBox::Save);
        int ret = msgBox.exec();
        if ( ret == QMessageBox::Save){
            on_actionSaveDatabase_triggered();
        } else if ( ret == QMessageBox::Cancel ) {
            return;
        }
    }

    annotations.clearFilesAndObjects();
    refreshObjView();
    refreshImgView("");
    xmlTextEdit->clear();
    pixmapWidget->setPixmap(QPixmap());

//	annotations.removeAllFiles()
//	annotations.removeAllObjects()
	copiedObj = NULL;

	// update the window title
	setWindowTitle("ImageAnnotation");

    statusBar()->clearMessage();
    refreshImgView("");
}

void MainWindow::on_actionOpenDatabase_triggered()
{
	// clear the status bar and set the normal mode for the pixmapWidget
	statusBar()->clearMessage();

	// ask the user to add files
	QString file_name = QFileDialog::getOpenFileName(
 			this,
            "Open an existing database",
            database_path,
            "XML File (*.xml)");

	if (file_name.isEmpty())
		return;

    loadDatabase(file_name, true);
}

void MainWindow :: onBackup()
{
    QDir backup_dir(database_path);
    if ( !backup_dir.mkpath("backup") ) {
        QErrorMessage * error = new QErrorMessage(this);
        error->showMessage("WARNING: Couldn't create backup-folder!");
        statusBar()->showMessage("");
    }

    // check for backup-filename
    QString backup_path = database_path + "/backup";
    QString backup_filename = backup_path + '/'
            + QFileInfo(database_filename).fileName() + ".bak";
    for( size_t i = 0; i < std::numeric_limits<size_t>::max(); i++ ){
        QString test = backup_filename + QString::number(i);
        if ( !QFile::exists(test) ) {
            backup_filename = test;
            break;
        }
    }

    if ( !QFile::copy(database_filename, backup_filename) ) {
        QErrorMessage * error = new QErrorMessage(this);
        error->showMessage("WARNING: Couldn't create backup-file!");
        statusBar()->showMessage("");
    }
}

void MainWindow::onSthChanged()
{
    if ( !actionSaveDatabase->isEnabled() )
        actionSaveDatabase->setEnabled(true);
}

void MainWindow::saveAnnoXml(QString file_path)
{
    QString text = xmlTextEdit->toPlainText();
    if (text.size() == 0){
        return;
    }

    QString ext = QFileInfo(file_path).completeSuffix();
    file_path.remove('.'+ext);
    QString xml_anno_path = file_path + "_anno.xml";

    QFile xml_anno_file(xml_anno_path);
    if ( !xml_anno_file.open(QFile::WriteOnly | QFile::Text) ) {
        QErrorMessage * error = new QErrorMessage(this);
        error->showMessage("WARNING: Couldn't save file" + xml_anno_path);
        QApplication::restoreOverrideCursor();
        statusBar()->showMessage("");
        return;
    }

    QTextStream out ( &xml_anno_file );

    out << text;
    if( !text.endsWith('\n') ){
        out << '\n';
    }
    xml_anno_file.close();
}

void MainWindow::loadXmlAnno(QString file_path,
                             bool show_error)
{
    QString ext = QFileInfo(file_path).completeSuffix();
    file_path.remove('.'+ext);
    QString xml_anno_path = file_path + "_anno.xml";

    QFile xml_anno_file(xml_anno_path);
    if ( !xml_anno_file.open(QFile::ReadOnly | QFile::Text) ) {
        if( show_error ){
            QErrorMessage * error = new QErrorMessage(this);
            error->showMessage("Couldn't load file" + xml_anno_path);
            QApplication::restoreOverrideCursor();
            statusBar()->showMessage("");
        } else {
            qDebug() << "couldn't load file" << xml_anno_path;
        }
        return;
    }

    QTextStream out ( &xml_anno_file );
    QString text;
    out >> text;
    xmlTextEdit->setText(text);
    xml_anno_file.close();
}

void MainWindow::saveAnnoZone(QString file_path)
{
    if(file_path == "")
        return;
    QString file_path_orig = file_path;
    QString ext = QFileInfo(file_path).completeSuffix();
    file_path.remove('.' + ext);
    QString xml_zone_path = file_path + "_zone.xml";

    QFile xml_zone_file(xml_zone_path);
    if ( !xml_zone_file.open(QFile::WriteOnly | QFile::Text) ) {
        QErrorMessage * error = new QErrorMessage(this);
        error->showMessage("Couldn't save file" + xml_zone_path);
        QApplication::restoreOverrideCursor();
        statusBar()->showMessage("");
        return;
    }

    QString zone = annotations.getXmlString(file_path_orig,
                             xml_options->getXmlHeader(),
                             xml_options->getXmlFooter(),
                             xml_options->getPrefix(),
                             true);
    QTextStream out(&xml_zone_file);
    out << zone;
    xml_zone_file.close();
}

void MainWindow :: saveDatabase( const QString & database_path )
{
    // let's block every signal while saving
    bool status_main = blockSignals(true);
    bool status_pix = pixmapWidget->blockSignals(true);
    bool status_scr = scrollArea->blockSignals(true);
    bool status_zoom = zoomSpinBox->blockSignals(true);

    QApplication::setOverrideCursor(Qt::WaitCursor);

    if (current_img_filename != ""){
        // save current file(s)
        if ( !xml_options->getOne_file() ){
            saveAnnoZone(current_img_filename);
        }
        saveAnnoXml(current_img_filename);
    }

	QFile file(database_path);
	if ( !file.open(QFile::WriteOnly | QFile::Text) ) {
        QErrorMessage * error = new QErrorMessage(this);
        error->showMessage("Couldn't open file" + database_path);
		QApplication::restoreOverrideCursor();
		statusBar()->showMessage("");
		return;
	}

    // save the data and change the cursor to a waiting cursor
    annotations.saveToFile( file, propertyList.mid(1) );
    onBackup();
    saveSerialization();
    QApplication::restoreOverrideCursor();

    zoomSpinBox->blockSignals(status_zoom);
    scrollArea->blockSignals(status_scr);
    pixmapWidget->blockSignals(status_pix);
    actionSaveDatabase->setDisabled(true);
    blockSignals(status_main);    
}

void MainWindow::on_actionSaveDatabase_triggered()
{
	// clear the status bar and set the normal mode for the pixmapWidget
//    pixmapWidget->setMouseMode(AnnotationsPixmapWidget::CreateRectangle);
    statusBar()->showMessage("Saving database to file " + database_filename);

    if ( database_filename.isEmpty() ) {
		// no filename given, launch "save as"
		on_actionSaveDatabaseAs_triggered();
		return;
	}

    saveDatabase( database_filename );

	// update the statusbar
	statusBar()->showMessage( QTime::currentTime().toString()
                              + ": Saved database to file " + database_filename, 5 * 1000) ;
}

void MainWindow::on_actionSaveDatabaseAs_triggered()
{
	// clear the status bar and set the normal mode for the pixmapWidget
//    pixmapWidget->setMouseMode(AnnotationsPixmapWidget::CreateRectangle);
	statusBar()->clearMessage();

	// ask the user to add files
	QString file = QFileDialog::getSaveFileName(
					   this,
					   "Save Database as ...",
                       database_path,
                       "XML File (*.xml)");

	if ( file.isEmpty() )
		return;

	// check wether an extension has been given
	if ( QFileInfo(file).suffix().isEmpty() )
		file += ".xml";

    // save the database path
    database_path = QFileInfo(file).absolutePath();
    database_filename = file;

	saveDatabase( file );

	// update the statusbar
	statusBar()->showMessage("Saved database to file " + file, 5 * 1000);

	setWindowTitle("ImageAnnotation" + QFileInfo(file).baseName());

//	// trigger now every minute an autosave
//	autosave->start( 1000 * 60 );
}

void MainWindow::on_actionQuit_triggered()
{
	close();
}

void MainWindow::on_actionShortcutHelp_triggered()
{
	// clear the status bar and set the normal mode for the pixmapWidget
//    pixmapWidget->setMouseMode(AnnotationsPixmapWidget::CreateRectangle);
	statusBar()->clearMessage();

	// we display an overview on shortcuts
 	QMessageBox::about(this, "Shortcut Help",
			"<table border=0 cellpadding=0 cellspacing=2>\n"
            "<tr>\n"
                "<td><b>Del</b></td>\n"
                "<td width=10></td>\n"
                "<td>delete the current object</td>\n"
            "</tr>"
            "<tr>\n"
                "<td><b>MouseWheel + Ctrl</b></td>\n"
                "<td></td>\n"
                "<td>zoom in/out</td>\n"
            "</tr>"
            "<tr>\n"
                "<td><b>Crtl+N</b></td>\n"
				"<td></td>\n"
				"<td>Create new Object (ignoring the mode)</td>\n"
			"</tr>\n"
            "<tr>\n"
                           "<td><b>Shift</b></td>\n"
                           "<td></td>\n"
                           "<td>Switch to browsing mode</td>\n"
                       "</tr>\n"
			"</table>\n");
}

void MainWindow::on_actionCopyObj_triggered()
{
	Object *obj = currentObject();
	if (NULL != obj) {
		if (NULL != copiedObj)
			delete copiedObj;
		copiedObj = new Object("", -1);
		*copiedObj = *obj;
		statusBar()->showMessage("Object copied", 2 * 1000);
	}
}

void MainWindow::on_actionPasteObj_triggered()
{
	if ( NULL == copiedObj || currentFilePath().isEmpty() )
		return;

    Object *newObj = annotations.newObject( currentFilePath().toStdString() );
	*newObj = *copiedObj;
	// tell it our pixmap widget too
	emit pasteObj( newObj->getID() );

	statusBar()->showMessage("Copied object pasted", 2 * 1000);

	// refresh the TableWidget for the objects
	refreshObjView();

	// choose the new object as the current object
	setCurrentTableItem(newObj->getID());

	// refresh
//	emit selectedObjectsChanged(fileObjectIDs());
//	pixmapWidget->repaint();
}


void MainWindow :: on_actionShowImageFileBrowser_triggered()
{
    if ( imgDockWidget->isVisible() ) {
        imgDockWidget->hide();
    }
    else {
        imgDockWidget->show();
    }
}

void MainWindow :: on_actionShowObjectProperties_triggered()
{    
    if ( objDockWidget->isVisible() ) {
        objDockWidget->hide();
    }
    else {
        objDockWidget->show();
    }
}

void MainWindow :: on_actionShowXmlPreview_triggered()
{
    if ( xmlDockWidget->isVisible() ) {
        xmlDockWidget->hide();
    }
    else {
        xmlDockWidget->show();
    }
}

void MainWindow::on_actionShowSnippetMenu_triggered()
{
    if ( snippetDockWidget->isVisible() ) {
        snippetDockWidget->hide();
    }
    else {
        snippetDockWidget->show();
    }
}

void MainWindow::on_addImgButton_clicked()
{
    // ask the user to add files
    QStringList files = QFileDialog::getOpenFileNames(
                this,
                "Add Files to the Data Base",
                QFileInfo(lastFileAddImg).absoluteFilePath(), // last added dir
                "Images (*.tif *.png *.jpg)");

    if ( files.size() <= 0 )
        return;

    // add files to the data structure
    annotations.addFiles( qt2std(files) );

    // save the first opened file
    lastFileAddImg = files[0];
    refreshImgView( QFileInfo(lastFileAddImg).fileName() );
    emit sthChanged();
}

void MainWindow::on_delImgButton_clicked()
{
    if ( ! imgTreeWidget->isActiveWindow() || imgTreeWidget->selectedItems().empty())
        return;

	// ask user for confirmation
	if (QMessageBox::Yes != QMessageBox::question(
			this,
			"Removing Images from DB",
			"Are you sure that you would like to remove"
            "the selected file(s) from the database?",
			QMessageBox::Yes, QMessageBox::No))
		return;

	// go through all directories and files and collect the selected files
	QStringList selectedFiles;
    int lowestRow = imgTreeWidget->topLevelItemCount();
    foreach ( QTreeWidgetItem *selected, imgTreeWidget->selectedItems() ) {
        int index = imgTreeWidget->indexOfTopLevelItem(selected);
        if ( index > lowestRow )
            lowestRow = index;
        // add the filepath to the list
        QString path = item2dir[selected] + "/" + selected->text(0);
        selectedFiles << path;
	}


	// remove selected files from the database
    annotations.removeFiles( qt2std(selectedFiles) );

    // search last row before selected
    int num_restitems = imgTreeWidget->topLevelItemCount() - imgTreeWidget->selectedItems().count();
    if ( num_restitems == 0 ){
        refreshImgView("");
        return;
    }
    int next_row = lowestRow;
    for (int i = 0; i < imgTreeWidget->topLevelItemCount(); i++)
    {
        next_row = (next_row + 1) % imgTreeWidget->topLevelItemCount();     
        if ( ! imgTreeWidget->topLevelItem(next_row)->isSelected() )
            break;
    }

    refreshImgView( imgTreeWidget->topLevelItem(next_row)->text(0) );
}

//void MainWindow::on_addPropertyButton_clicked()
//{
//	// get the input from the lineEdit field
//	QString newProperty = propertyLineEdit->text().trimmed().toLower().replace(QRegExp("\\s+|:"), "_");

//	// check wether the property already exists or not
//	if (propertyList.contains(newProperty) || reservedProperties.contains(newProperty) || newProperty.isEmpty())
//		return;
//	propertyList << newProperty;

//	// clear the text from the lineEdit
//	propertyLineEdit->setText("");

//	refreshObjView();
//    emit sthChanged();
//}

//void MainWindow::on_addFilePropertyButton_clicked()
//{
//	// get the input from the lineEdit field
//	QString newProperty = filePropertyLineEdit->text().trimmed().toLower().replace(QRegExp("\\s+|:"), "_");

//	// check wether the property already exists or not
//	if (filePropertyList.contains(newProperty) || newProperty.isEmpty())
//		return;
//	filePropertyList << newProperty;
////	filePropertyList.sort();

//	// clear the text from the lineEdit
//	filePropertyLineEdit->setText("");

//	refreshFilePropertiesView();
//    emit sthChanged();
//}
	
//void MainWindow::on_objTypeComboBox_currentIndexChanged(const QString &text)
//{
//	// check wether dir/file/object have been selected
//	QString iFile = currentFile();
//	QString iDir = currentDir();
//	int iObj = currentObj();
//	if (iFile.isEmpty() || iDir.isEmpty() || iObj < 0)
//		return;
//
//	// set the new object type
//	IAObj *obj = annotations.getObj(iDir, iFile, iObj);
//	if (obj)
//		obj->type = text;
//
//	// update the text of the object which type has been changed
//	QListWidgetItem *item = objTableWidget->item(iObj);
//	if (item)
//		item->setText(text);
//
//	// save the last chosen obj type
//	lastObjType = text;
//}

void MainWindow::on_imgTreeWidget_currentItemChanged(QTreeWidgetItem *current, QTreeWidgetItem *previous)
{    
    // get new file path
    QString file_path = currentFilePath();
    if (file_path.isEmpty()){
        return;
    }

    // save current file(s)
    if ( !xml_options->getOne_file() ){
        saveAnnoZone(current_img_filename);
    }
    saveAnnoXml(current_img_filename);

    // load new file
    // the currentFilePath may be a relative path so let's check that first
    QString orig_file_path = file_path;
    if ( QFileInfo(file_path).isRelative() ) {
        file_path = QFileInfo( database_path, QFileInfo(item2dir[current], current->text(0)).filePath() ).absoluteFilePath();
    }
    // allow image-filename in lowercase
    if( !QFileInfo(file_path).exists() ){
        QFileInfo fi(file_path);
        QDir dir = fi.dir();
        QString fn = fi.fileName().toLower();
        QFileInfo fi2(dir, fn);
        file_path = fi2.absoluteFilePath();
    }


    // emit the signal that the file changed
//    emit activeFileChanged( file_path );

    // load image i.e. set image to pixmap
    QPixmap tmpPixmap(file_path);
    pixmapWidget->setPixmap(tmpPixmap);

    // set some initial specifications
    QString width = QString::number(tmpPixmap.width());
    QString height = QString::number(tmpPixmap.height());

    File* file = annotations.getFile(orig_file_path.toStdString());
    if ( file == NULL){
        qDebug() << "WARNING: no file w. file_name:" << orig_file_path;
    } else {
        annotations.getFile(orig_file_path.toStdString())->set("width", width.toStdString());
        annotations.getFile(orig_file_path.toStdString())->set("height", height.toStdString());

        xmlTextEdit->setText(annotations.getXmlString(orig_file_path,
                                                  xml_options->getXmlHeader(),
                                                  xml_options->getXmlFooter(),
                                                  xml_options->getPrefix(),
                                                  false) );
    }

    // refresh the objTableWidget
    refreshObjView();
//    refreshFilePropertiesView();

    // select the first object as current obj or no object .. refresh will be done
    // implicitely through the selection
    
    if (objTableWidget->rowCount() > 0)
        objTableWidget->setCurrentCell(0, 0);
    else {
        emit activeObjectChanged(-1);
        emit selectedObjectsChanged(IDList());
    }
    current_img_filename = orig_file_path;

    on_actionBrowsingMode_triggered();
}

void MainWindow::on_objTableWidget_currentItemChanged(QTableWidgetItem *, QTableWidgetItem *)
{
	emit activeObjectChanged(currentObjectID());
}

void MainWindow::on_objTableWidget_itemSelectionChanged()
{
	IDList selectedObjs = selectedObjectIDs();
	if (selectedObjs.size() <= 1)
		emit selectedObjectsChanged(fileObjectIDs());
	else
		emit selectedObjectsChanged(selectedObjs);
}

// TODO: might it be that this method is actually never called??

void MainWindow::on_objTableWidget_itemChanged(QTableWidgetItem* item)
{    
	// get the object in the data base
    Object *obj = currentObject();
	if (NULL == obj)
		return;

    // query the properties and save them in the database
	string property = propertyList[item->column()].toStdString();
	string value = item->text().toStdString();
	if (property.empty())
		obj->clear(property);
	else
		obj->set(property, value);

	objTableWidget->resizeColumnsToContents();
//	refreshObjView();
    emit sthChanged();

}


//void MainWindow::on_filePropertiesTableWidget_itemChanged(QTableWidgetItem* item)
//{

//    // get the object in the data base
//    File *file = currentFile();
//    if (NULL == file)
//        return;

//    // query the properties and save the in the database
//    string property = filePropertiesTableWidget->item(item->row(), 0)->text().toStdString();
//    string value = item->text().toStdString();
//    if (property.empty())
//        file->clear(property);
//    else
//        file->set(property, value);

//    filePropertiesTableWidget->resizeColumnsToContents();
////    filePropertiesTableWidget->setSortingEnabled(false);
//    filePropertiesTableWidget->horizontalHeader()->setStretchLastSection(true);
////	refreshObjView();
//    emit sthChanged();

//}


//void MainWindow::on_objListWidget_itemSelectionChanged()
//{
//	// check wether dir/file/object have been selected
//	QString iFile = currentFile();
//	QString iDir = currentDir();
//	int iObj = currentObj();
//	if (iFile.isEmpty() || iDir.isEmpty() || iObj < 0)
//		return;
//
//	// collect all selected tags
//	const QList<QListWidgetItem *> selectedTags = tagListWidget->selectedItems();
//	QStringList newTags;
//	QString tmpStr;
//	for (int i = 0; i < selectedTags.count(); i++) {
//		tmpStr = selectedTags[i]->text();
//		if (!tmpStr.isEmpty())
//			newTags << tmpStr;
//	}
//
//	// update the object's tag list
//	IAObj *obj = annotations.getObj(iDir, iFile, iObj);
//	if (obj) {
//		obj->tags.clear();
//		obj->tags << newTags;
//	}
//}

//void MainWindow::refreshTagView()
//{
//	// block all signals from the tagListWidget
//	tagListWidget->blockSignals(true);
//
//	// clear all tags and the tags from our internal tag list (containing all tags)
//	tagListWidget->clear();
//	tagListWidget->addItems(tagList);
//
//	// check wether dir/file/object have been selected
//	QString iFile = currentFile();
//	QString iDir = currentDir();
//	int iObj = currentObj();
//	if (!iFile.isEmpty() || !iDir.isEmpty() || iObj >= 0) {
//		// select the tags for the current obj
//		IAObj *obj = annotations.getObj(iDir, iFile, iObj);
//		if (obj)
//			for (int i = 0; i < obj->tags.count(); i++) {
//				// find the corresponding tag and select it
//				QString currentTag = obj->tags[i];
//				QList<QListWidgetItem *> foundItems = tagListWidget->findItems(currentTag, Qt::MatchExactly);
//				if (foundItems.count() != 1)
//					continue;
//
//				tagListWidget->setItemSelected(foundItems[0], true);
//			}
//	}
//
//	// unblock signals
//	tagListWidget->blockSignals(false);
//}


void MainWindow::refreshObjView()
{
    // check whether dir/file/object have been selected
    string file_path = currentFilePath().toStdString();
    if ( file_path.empty() ) {
		objTableWidget->clear();
		return;
	}

	// block the signals of the objTableWidget
//	ID currentID = currentObjectID();
	objTableWidget->blockSignals(true);
//	objTableWidget->setSortingEnabled(false);

	// adjust the size of the table
    int nRows = annotations.numOfObjects(ImgAnnotations::dirPath(file_path), ImgAnnotations::fileName(file_path));
	int nCols = propertyList.size();
	objTableWidget->clear();
	objTableWidget->setRowCount(nRows);
	objTableWidget->setColumnCount(nCols);

	// clear our QTableWidget and add the objects of the current image to it
	int iRow = 0;
    BOOST_FOREACH(Object *obj, annotations.getObjects(ImgAnnotations::dirPath(file_path), ImgAnnotations::fileName(file_path))) {
		// we have to add each table cell manually
		int iCol = 0;

		foreach(QString qtProperty, propertyList) {
			string stdProperty = qtProperty.toStdString();
            QTableWidgetItem * newItem = new QTableWidgetItem();

			// set the property value as text
            if ( 0 == iCol ) {
				newItem->setData(Qt::DisplayRole, QVariant(obj->getID()));
				newItem->setFlags(Qt::ItemIsEnabled | Qt::ItemIsUserCheckable | Qt::ItemIsSelectable);
			}
			else {
				QString str = QString::fromStdString(obj->get(stdProperty));
				newItem->setData(Qt::DisplayRole, QVariant(str));
                newItem->setFlags(Qt::ItemIsEnabled | Qt::ItemIsUserCheckable | Qt::ItemIsSelectable | Qt::ItemIsEditable);
			}

			// add the item            
            objTableWidget->setItem(iRow, iCol, newItem);

			// increase the counter
			iCol++;
		}

		// increase the counter
		iRow++;
	}

	// set the labels for the header row/column
	objTableWidget->setHorizontalHeaderLabels(propertyList);
	objTableWidget->resizeColumnsToContents();
//	objTableWidget->resizeRowsToContents();
//	setCurrentTableItem(currentID);

	// unblock the signals
//	objTableWidget->setSortingEnabled(true);
	objTableWidget->blockSignals(false);    


}

void MainWindow::refreshImgView(QString last_file )
{
    // clear all items
    imgTreeWidget->blockSignals(true);
    imgTreeWidget->setUpdatesEnabled(false);
    imgTreeWidget->clear();

    // add all dirs and files to the QTreeWidget
    QList<QTreeWidgetItem*> newItems;
    item2dir.clear();
    BOOST_FOREACH(Dir *dir, annotations.getDirs()) {
        BOOST_FOREACH(string fileName, dir->getFileNames()) {
            // construct a new file entry
            QTreeWidgetItem *fileEntry = new QTreeWidgetItem();
            imgTreeWidget->setItemExpanded(fileEntry, true);
            fileEntry->setText(0, QString::fromStdString(fileName));
            // collect item in list
            newItems.append(fileEntry);
            // add to our itemmap
            item2dir[fileEntry] = QString(dir->getDirPath().c_str());
        }
    }

    // add all items at once to the tree widget
    imgTreeWidget->addTopLevelItems(newItems);

    // unblock signals
    imgTreeWidget->setUpdatesEnabled(true);
    imgTreeWidget->blockSignals(false);
    imgTreeWidget->setSortingEnabled(true);
    imgTreeWidget->sortByColumn(0, Qt::AscendingOrder);

    if ( last_file != "" ){
        for( int i = 0; i < imgTreeWidget->topLevelItemCount(); i++ ){
            if ( imgTreeWidget->topLevelItem(i)->text(0) == last_file ){
                imgTreeWidget->setCurrentItem( imgTreeWidget->topLevelItem(i) );
                return;
            }
        }
    }
    // otherwise select last item
    else {
        imgTreeWidget->setCurrentItem( imgTreeWidget->topLevelItem( imgTreeWidget->topLevelItemCount() - 1 ) );
    }
}


//void MainWindow::refreshFilePropertiesView()
//{
//	// check wether dir/file/object have been selected
//	string filePath = currentFilePath().toStdString();
//	if (filePath.empty()) {
//		filePropertiesTableWidget->clear();
//		return;
//	}

//	// block the signals of the objTableWidget
//	filePropertiesTableWidget->blockSignals(true);
////	filePropertiesTableWidget->setSortingEnabled(false);

//	// adjust the size of the table
//	filePropertiesTableWidget->clear();
//	filePropertiesTableWidget->setRowCount(filePropertyList.size());
//	filePropertiesTableWidget->setColumnCount(2);

//	// clear our QTableWidget and add the objects of the current image to it
//	QTableWidgetItem *valueItem, *keyItem;
//	int iRow = 0;
//	IA::File* file = currentFile();
//	foreach(QString property, filePropertyList) {
//		QString value = QString::fromStdString(file->get(property.toStdString()));
//		keyItem = new QTableWidgetItem;
//		keyItem->setData(Qt::DisplayRole, property);
//		keyItem->setFlags(Qt::ItemIsEnabled | Qt::ItemIsUserCheckable | Qt::ItemIsSelectable);

//		valueItem = new QTableWidgetItem;
//		valueItem->setData(Qt::DisplayRole, value);
//		valueItem->setFlags(Qt::ItemIsEnabled | Qt::ItemIsUserCheckable | Qt::ItemIsSelectable | Qt::ItemIsEditable);

//		// add the key-value pair as new row
//		filePropertiesTableWidget->setItem(iRow, 0, keyItem);
//		filePropertiesTableWidget->setItem(iRow, 1, valueItem);

//		// increase the counter
//		iRow++;
//	}

//	// set the labels for the header row/column
//	QStringList headerList;
//	headerList << "property" << "value";
//	filePropertiesTableWidget->setHorizontalHeaderLabels(headerList);
//	filePropertiesTableWidget->resizeColumnsToContents();
////	filePropertiesTableWidget->setSortingEnabled(false);
//	filePropertiesTableWidget->horizontalHeader()->setStretchLastSection(true);
	
//	// unblock the signals
////	filePropertiesTableWidget->setSortingEnabled(true);
//	filePropertiesTableWidget->blockSignals(false);
	
//}


IDList MainWindow::selectedObjectIDs() const
{
	
	// collect the selected objects
	IDList idList;
	for (int i = 0; i < objTableWidget->rowCount(); i++) {
		QTableWidgetItem *item = objTableWidget->item(i, 0);
		if (item->isSelected())
			// add the object index to the list, denoting that this index
			// is to be removed from the list
			idList.push_back(static_cast<ID>(objectIDFromItem(item)));
	}
	return idList;
	
}

//IDList MainWindow::selectedObjectIDs() const
//{
//	IDList idList;
//
//	string filePath = currentFilePath().toStdString();
//	// get all objects associated with the current image
//	if (!filePath.empty())
//		idList = annotations.getObjectIDs(ImgAnnotations::dirPath(filePath), ImgAnnotations::fileName(filePath));
//
//	return idList;
//}

IDList MainWindow::fileObjectIDs() const
{
	IDList idList;

	string filePath = currentFilePath().toStdString();
	// get all objects associated with the current image
	if (!filePath.empty())
		idList = annotations.getObjectIDs(ImgAnnotations::dirPath(filePath),
										  ImgAnnotations::fileName(filePath));

	return idList;
}

void MainWindow::nextPreviousFile(MainWindow::Direction direction)
{
    // choose the current items from the imgTreeWidget
    QTreeWidgetItem *current = imgTreeWidget->currentItem();
    if ( !current )
        return;

    int num_elements = imgTreeWidget->topLevelItemCount();
    if ( num_elements <= 1)
        return;

    int row = imgTreeWidget->indexOfTopLevelItem(current);
    int next_element = (row + 1) % num_elements;
    if ( direction == Up ) {
        next_element = (row - 1 + num_elements) % num_elements;      
    }
    imgTreeWidget->setCurrentItem( imgTreeWidget->topLevelItem(next_element) );

//	QTreeWidgetItem *currentParent = current->parent();

//	if (!currentParent) {
//		// we have a directory selected .. take the first file as current item
//		current = current->child(0);
//		currentParent = current->parent();
//	}

//	if (!current || !currentParent)
//		return;

//	// get the indeces
//	int iParent = imgTreeWidget->indexOfTopLevelItem(currentParent);
//	int iCurrent = currentParent->indexOfChild(current);

//	// select the next file index
//	if (direction == Up)
//		iCurrent--;
//	else
//		iCurrent++;

//	// the index may be negative .. in that case we switch the parent as well
//	if (iCurrent < 0) {
//		if (iParent > 0) {
//			// get the directory before
//			iParent--;
//			currentParent = imgTreeWidget->topLevelItem(iParent);

//			if (!currentParent)
//				return;

//			// get the last item from the directory before
//			iCurrent = currentParent->childCount() - 1;
//		}
//		else
//			// we are at the beginning ..
//			iCurrent = 0;
//	}
//	// the index might be too large .. in that case we switch the parent as well
//	else if (iCurrent >= currentParent->childCount()) {
//		if (iParent < imgTreeWidget->topLevelItemCount() - 1) {
//			// get the next directory
//			iParent++;
//			currentParent = imgTreeWidget->topLevelItem(iParent);

//			if (!currentParent)
//				return;

//			// get the first item from the next directory
//			iCurrent = 0;
//		}
//		else
//			// we are at the end ..
//			iCurrent = currentParent->childCount() - 1;
//	}

//	if (!currentParent)
//		return;

//	// we handled all special cases thus we may try to set the next current item
//	current = currentParent->child(iCurrent);
//    if (current)
    //        imgTreeWidget->setCurrentItem(current);
}

void MainWindow :: showQuickPropertyMenu()
{
    PropertyDialog * property_dialog = new PropertyDialog(this, annotations, propertyList);
    if ( property_dialog->exec() == QDialog::Accepted ) {
        QMap<QString, QString> value_of_property = property_dialog->getNewValues();
        Object *obj = currentObject();
        // should never happen:
        if ( obj == NULL ) {
            delete property_dialog;
            return;
        }
        bool was_new_object = true;
        foreach( QString key, value_of_property.keys() ) {
            // check if the property was already set
            if ( ! obj->get(key.toStdString()).empty() )
                was_new_object = false;
            // is property empty?
            if ( value_of_property[key].isEmpty() )
                continue;
            obj->set(key.toStdString(), value_of_property[key].toStdString());
        }
        // add a new object when we are done, but not if we edited an old one
        if ( was_new_object )
            addObj();
    }

    delete property_dialog;
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    if ( actionSaveDatabase->isEnabled() ){
        QMessageBox msgBox;
        msgBox.setText("The document has been modified.");
        msgBox.setInformativeText("Do you want to save your changes?");
        msgBox.setStandardButtons(QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);
        msgBox.setDefaultButton(QMessageBox::Save);
        int ret = msgBox.exec();
        if ( ret == QMessageBox::Save){
            on_actionSaveDatabase_triggered();
        } else if ( ret == QMessageBox::Discard )
            event->accept();
        else
            event->ignore();
    } else {
        event->accept();
    }
}

void MainWindow::keyPressEvent(QKeyEvent * event)
{
	if ( event->key() == Qt::Key_Shift
		 && pixmapWidget->hasFocus() )
	{
		event->accept();
		saved_mouse_mode = pixmapWidget->getMouseMode();
		on_actionBrowsingMode_triggered();
//		pixmapWidget->setMouseMode(AnnotationsPixmapWidget::Browsing);
		statusBar()->showMessage("Browsing mode: select a bounding box visually");
	}
	else if ( event->key() == Qt::Key_Control )
	{
		allow_zoom = true;
	}
//    else if (event->key() == Qt::Key_Control) {
//        event->accept();
//        saved_mouse_mode = pixmapWidget->getMouseMode();
////        pixmapWidget->setMouseMode(AnnotationsPixmapWidget::DeleteFixPoint);
//        statusBar()->showMessage("Delete fix points, zoom in the image with the mouse wheel");
//    }
    // probably need this if we use Key_Control
//    else if (event->key() == Qt::Key_N){
//        saved_mouse_mode = AnnotationsPixmapWidget::CreateRectangle;
//    }
    else
        event->ignore();
}

void MainWindow::keyReleaseEvent(QKeyEvent * event)
{    
    if (event->key() == Qt::Key_Shift
            && pixmapWidget->hasFocus()
            )
//       || (event->key() == Qt::Key_Control
//        && event->modifiers() != Qt::ControlModifier))
    {
        event->accept();
        if ( saved_mouse_mode == AnnotationsPixmapWidget::CreateFixPoint )
            on_actionCreatePolygon_triggered();
        else if ( saved_mouse_mode == AnnotationsPixmapWidget::CreateRectangle )
            on_actionCreateRectangle_triggered();
		statusBar()->clearMessage();        
	}
	else if ( event->key() == Qt::Key_Control )
	{
		allow_zoom = false;
	}
    // FIXME
//	else if ( event->key() == Qt::Key_Space ) {
////		if ( pixmapWidget->mouseOverObj() )
//			showQuickPropertyMenu();
//	}
	else
		event->ignore();
}

bool MainWindow::zoomAllowed(){
    return allow_zoom;
}

void MainWindow::wheelEvent(QWheelEvent *event)
{
//    std::cerr << "wheel event in main\n";
    if ( !event->isAccepted() ) {
//        std::cerr << "not accept yet\n";
        // FIXME instead of going files up and down go image-selection up and down
        // therefore probably ScrollAreaNoWheel needs to be changed to accept wheel ...

		// see what to do with the event
//        if ( pixmapWidget->getMouseMode() == AnnotationsPixmapWidget::Normal )
//        {
//            // select a different file
//            if (event->delta() < 0)
//                nextPreviousFile(Down);
//            else if (event->delta() > 0)
//                nextPreviousFile(Up);
//        }
//        else {
            // forward the wheelEvent to the zoomSpinBox
//        if (  allow_zoom ){

			if (event->delta() < 0)
				zoomSpinBox->stepDown();
			else if (event->delta() > 0)
				zoomSpinBox->stepUp();

//        }
		event->accept();
//        }
	}
}


void MainWindow :: on_actionRemoveProperty_triggered()
{

    // ask user for confirmation
    if (QMessageBox::Yes != QMessageBox::question(
            this,
            "Removing Property from DB",
            "Are you sure that you would like to remove"
            "the selected property from the Propertylist?",
            QMessageBox::Yes, QMessageBox::No))
        return;

    // the property string
    QString property = propertyList.at(objTableColumn);

    // remove from all objects therefore build up list of objids
    if ( objTableWidget->rowCount() > 0 ){
        IA::IDList id_list;
        for (int i = 0; i < objTableWidget->rowCount(); i++) {
            QTableWidgetItem *item = objTableWidget->item(i, 0);
            // add the object index to the list
            id_list.push_back(objectIDFromItem(item));
        }        
        annotations.removeProperty(id_list, property.toStdString());
    }

    // remove from the propertylist
    propertyList.removeAt(objTableColumn);

    refreshObjView();
    statusBar()->showMessage("Removed property " + property);
    emit sthChanged();
}

void MainWindow :: showObjTableContextMenu(const QPoint & pos)
{
    // get current column, test header and cell
    objTableColumn = objTableWidget->horizontalHeader()->logicalIndexAt( pos );
    if (objTableColumn < 0 && objTableWidget->itemAt ( pos ) != NULL)
        objTableColumn = objTableWidget->itemAt ( pos )->column();

    QMenu *menu = new QMenu(this);
    // ignore the first = 0 - column as it shouldn't be allowed to remove 'id'
    if ( objTableColumn > 0 ) {
        actionRemoveProperty->setText("Remove property '" + objTableWidget->horizontalHeaderItem(objTableColumn)->text() );
        menu->addAction(actionRemoveProperty);
    }
    menu->addAction(actionRemoveObj);
    menu->exec(QCursor::pos());
    delete menu;
}


// quick edit context menu
void MainWindow :: showObjContextMenu(const QPoint & pos)
{
    if ( pixmapWidget->mouseOverObj() )
    {
        actionPropertydialog->setEnabled(true);
        actionCopyObj->setEnabled(true);
        actionRemoveObj->setEnabled(true);
        actionRecognition->setEnabled(true);
    }
    else {
        actionPropertydialog->setDisabled(true);
        actionCopyObj->setDisabled(true);
        actionRemoveObj->setDisabled(true);
        actionRecognition->setDisabled(true);
    }

    QMenu* menu = new QMenu();
    menu->addAction(actionPropertydialog);
    menu->addAction(actionCopyObj);
    menu->addAction(actionPasteObj);
    menu->addAction(actionRemoveObj);
    menu->addAction(actionRecognition);
    menu->exec( QCursor::pos() );
    delete menu;
}

//void MainWindow::on_imgTreeWidget_clicked(const QModelIndex &index)
//{
//    on_addImgButton_clicked();
//}

void MainWindow :: addObj()
{
    // check whether dir/file/object have been selected
    QString filePath = currentFilePath();
    if (filePath.isEmpty())
        return;

    // add a new object to the database
    Object *obj = annotations.newObject( filePath.toStdString() );

    // refresh the TableWidget for the objects
    refreshObjView();

    // set the new object as current object
    setCurrentTableItem(obj->getID());

    // Don't know if that is good or not atm
    if( pixmapWidget->getMouseMode() != AnnotationsPixmapWidget::CreateRectangle
            &&  pixmapWidget->getMouseMode() != AnnotationsPixmapWidget::CreateFixPoint)
    {
        on_actionCreateRectangle_triggered();
    }
}

void MainWindow :: on_actionBrowsingMode_triggered()
{
    actionBrowsingMode->setChecked(true);
    actionCreateRectangle->setChecked(false);
    actionCreatePolygon->setChecked(false);
    pixmapWidget->setMouseMode( AnnotationsPixmapWidget::Browsing );
}

void MainWindow :: on_actionCreateRectangle_triggered()
{       
    if ( currentObjectID() == -1
        || ! currentObject()->get("fixpoints").empty() )
        addObj();
    actionBrowsingMode->setChecked(false);
    actionCreateRectangle->setChecked(true);
    actionCreatePolygon->setChecked(false);
    pixmapWidget->setMouseMode( AnnotationsPixmapWidget::CreateRectangle );
}

void MainWindow::on_actionCreatePolygon_triggered()
{
    if ( currentObjectID() == -1
        || ! currentObject()->get("bbox").empty() )
        addObj();
    actionBrowsingMode->setChecked(false);
    actionCreateRectangle->setChecked(false);
    actionCreatePolygon->setChecked(true);
    pixmapWidget->setMouseMode( AnnotationsPixmapWidget::CreateFixPoint );
}


void MainWindow::on_actionRemoveObj_triggered()
{
    
    // check wether dir/file/object have been selected
    ID id = currentObjectID();
    IDList idList = selectedObjectIDs();
    int iRow = objTableWidget->currentRow();
    if (id < 0 || idList.size() <= 0)
        return;

    // remove selected objects from the file
    annotations.removeObjects(idList);

    // set the object before the last current object as current object
    if (iRow >= objTableWidget->rowCount())
        iRow = objTableWidget->rowCount() - 1;
    if (iRow >= 0)
        objTableWidget->setCurrentCell(iRow, -1);

    refreshObjView();
    emit sthChanged();
    
}


void MainWindow :: on_actionZoomIn_triggered()
{       
   pixmapWidget->setZoomFactor( pixmapWidget->getZoomFactor() + 0.1 );
}

void MainWindow :: checkZoom(double zoom_factor)
{
    if ( zoom_factor <= 0.1){
        actionZoomOut->setDisabled(true);
    }
    else {
        actionZoomOut->setEnabled(true);
    }
}

void MainWindow :: on_actionZoomOut_triggered()
{    
//    if (! actionZoomOut->isEnabled() )
//        return;
    pixmapWidget->setZoomFactor( pixmapWidget->getZoomFactor() - 0.1 );
}

void MainWindow :: setSnippetAndZone(const QString & value)
{
    Object *obj = currentObject();
    if ( obj == NULL ){
        qDebug() << "WARNING: MainWindow::setSnippetAndZone: obj = NULL";
        return;
    }

    QString width = QString::fromStdString(annotations.getFile( obj->getFilePath())->get("width"));
    QString height = QString::fromStdString(annotations.getFile( obj->getFilePath())->get("height"));

    QString zone_snip = annotations.replaceSpecial(xml_options->getZone_snippet(),
                                                   obj, xml_options->getPrefix(),
                                                   "",
                                                   width.toInt(), height.toInt());

    // paste current snippet to the edit window
    QString snippet = snippetTextEdit->toPlainText();
    obj->set("snippet_raw", snippet.toStdString());
    snippet = annotations.replaceSpecial(snippet, obj,
                                         xml_options->getPrefix(),
                                         value,
                                         width.toInt(), height.toInt());
    snippet.replace(xml_options->getPrefix() + "zone_snippet", zone_snip);

    // save to internal database the snippet and the zone
    obj->set("snippet", snippet.toStdString());
    obj->set("zone", zone_snip.toStdString());
    obj->set("value", value.toStdString());

    refreshObjView();

    // lets update the xml preview
    xmlTextEdit->setText(annotations.getXmlString(current_img_filename,
                                                  xml_options->getXmlHeader(),
                                                  xml_options->getXmlFooter(),
                                                  xml_options->getPrefix(),
                                                  false) );

}

void MainWindow :: on_actionPropertydialog_triggered()
{
//    showQuickPropertyMenu();

    // word recognition
    if ( wordRecognitionCheckBox->isChecked() ) {
        on_actionRecognition_triggered();
        return;
    }
    // else standard value-dialog
    bool ok;
    QString value = QInputDialog::getText(this, tr("Set Value"),
                                          tr("Value:"), QLineEdit::Normal,
                                          valLineEdit->text(), &ok);
    if (ok && !value.isEmpty()){
        setSnippetAndZone(value);
    }
}

void MainWindow :: on_actionAddObject_triggered()
{
    addObj();
}

//void MainWindow :: buildDomDoc()
//{
//    QDomProcessingInstruction instr =
//            ontology_dom.createProcessingInstruction(
//                "xml", "version='1.0' encoding='UTF-8'" );
//    ontology_dom.appendChild(instr);
//    // add the ontology to the dom-document
//    annotations.addOntologyXML( ontology_dom, ontology_dom, propertyList.mid(1) );
////    qDebug() << ontology_dom.toString();
//}


//void MainWindow :: on_actionSaveOntologyAs_triggered()
//{
//    // get filename and check it
//    QString file_name = QFileDialog::getSaveFileName(
//                            this, tr("Save Ontology as ..."),
//                            QDir::currentPath(),
//                            "XML files (*.xml)" );
//    if ( file_name.isEmpty() )
//        return;

//    QFile file( file_name );
//    if ( !file.open(QFile::WriteOnly | QFile::Text) ) {
//        QMessageBox::warning( this, tr("ontology"),
//                              tr("Cannot write file %1:\n%2.")
//                              .arg(file_name)
//                              .arg(file.errorString()) );
//        return;
//    }

//    // build up the dom-document
//    if ( ontology_dom.namedItem("Ontology").isNull() )
//        buildDomDoc();

//    // save dom-document
//    QTextStream out( &file );
//    ontology_dom.save( out, 4 );
//    file.close();
//}


//void MainWindow :: ontologyToAnnotation()
//{
//    QDomElement e = ontology_dom.namedItem("ontology").toElement();
//    if ( e.isNull() || ! e.hasChildNodes() ) {
//        QErrorMessage * error = new QErrorMessage(this);
//        QString s = "";
//        if ( e.isNull() )
//            s += "No <ontology> element found at the top-level of the XML file!";
//        else
//            s += "No child nodes of <ontology> in the XML file";
//        error->showMessage(s);
//        return;
//    }

//    // put into annotation
//    propertyList.clear();
//    propertyList = e.namedItem("propertylist").toElement().text().split(",");
//    propertyList.prepend("<ID>");

//    // remove old properties
//    std::list<std::string> properties = annotations.getAllObjProperties();
//    std::list<IA::ID> objects = annotations.getObjectIDs();
//    BOOST_FOREACH( std::string property, properties ) {
//        if ( reservedProperties.contains( QString(property.c_str()) ) )
//            continue;
//        annotations.removeProperty( objects, property );
//    }
//}

//void MainWindow::on_actionLoadOntology_triggered()
//{
//    QString file_name = QFileDialog::getOpenFileName(
//            this,
//            "Load an existing ontology",
//            QDir::currentPath(),
//            "XML files (*.xml)" );

//	if ( file_name.isEmpty() )
//		return;

//	QFile file( file_name );
//	if ( !file.open(QFile::ReadOnly | QFile::Text) ) {
//		QErrorMessage * error = new QErrorMessage(this);
//		error->showMessage("Error opening File");
//		return;
//	}
//	QString error_msg;
//	int error_line, error_column;
//	if ( ! ontology_dom.setContent( &file, &error_msg, &error_line, &error_column ) )
//	{
//		QErrorMessage * error = new QErrorMessage(this);
//		std::stringstream ss;
//		ss << "Error in parsing XML file:\n"
//		   << "Explanatory string: " << error_msg.toStdString() << '\n'
//		   << " at line: " << error_line << " at column " << error_column << '\n';
//		error->showMessage( QString(ss.str().c_str()) );
//		return;
//	}

//	ontologyToAnnotation();
//	refreshImgView("");
//}

using namespace boost::program_options;

void MainWindow::initAEConfig()
{
    string placeholder;
    attri_config = new puhma::AttriConfig();
    variables_map vm;
    positional_options_description p;

    options_description opt;
    opt.add(attri_config->options);
    opt.add_options()
        ("command", value(&placeholder)->default_value(""), "command to execute");
    p.add("command", 1);

    store(command_line_parser(0, NULL).options(opt).positional(p).run(), vm);
    notify(vm);
}

void MainWindow::on_actionPrepare_triggered()
{
    //attri_config = AttriConfig("--inpu;
    /*if(attri_config == nullptr){
        string placeholder;
        attri_config = new puhma::AttriConfig();
        variables_map vm;
        positional_options_description p;
        
        options_description opt;
        opt.add(attri_config->options);
        opt.add_options()
            ("command", value(&placeholder)->default_value(""), "command to execute");
        p.add("command", 1);

        store(command_line_parser(0, NULL).options(opt).positional(p).run(), vm);
		notify(vm);
    }*/
    /*if(firstTimeOpenReco){
        loadSerialization4Reco();
        firstTimeOpenReco = false;
    }*/
    PrepareWizard * prepare_wizard = new PrepareWizard(attri_config, &ae, this);
    /*prepare_wizard->show();
    while(prepare_wizard->result()!=QDialog::Accepted||
          prepare_wizard->result()!=QDialog::Rejected){
        qDebug()<<"called";
        sleep(1);
    }*/
    int status = prepare_wizard->exec();
    if( status == QDialog::Accepted||
            status == QDialog::Rejected ) {
        saveSerialization4Reco();
    }
    delete prepare_wizard;
}

void MainWindow::on_actionRecognition_triggered()
{
    //if(ae == nullptr || attri_config == nullptr){
    if(!(ae->isRecoReady())){
        QErrorMessage *error = new QErrorMessage(this);
        QString msg ="Word Recogntiion: Error in get instance of ATTRIBUTEEMBED object:\n";
        msg += "Run Prepare first.";
        error->showMessage(msg);
        return;
    }

    cv::Mat chopedImg = ImageChoper::chopOneFromImage(&annotations, currentObjectID(),
            QFileInfo(database_path,QString::fromStdString(annotations.getFile(currentFilePath().toStdString())->getFilePath())).filePath().toStdString());

    RecogResult rr;

    RecogWizard *rw = new RecogWizard(&rm, &rr, chopedImg, ae, this);
    //connect(rw, SIGNAL(accept()),this, SLOT(recogresultchoosed()));
    rw->show();
    if( rw->exec() == QDialog::Accepted
            && !rr.result.isEmpty())
    {
        setSnippetAndZone(rr.result);
    }

    ///get the image
    /*ae->recog4Annotation(chopedImg);*/
}

void MainWindow:: changeSnippet(int selected)
{
    snippet_text[current_snippet] = snippetTextEdit->toPlainText();
    current_snippet = selected;
    snippetTextEdit->setText(snippet_text[selected]);
}

void MainWindow::on_snippetSelectionTableWidget_itemSelectionChanged()
{
	QList<QTableWidgetItem*> selected = snippetSelectionTableWidget->selectedItems();
    if( selected.isEmpty() ){
        return;
    }
	changeSnippet(selected[0]->row()*snippetSelectionTableWidget->columnCount() + selected[0]->column());
}

void MainWindow::on_snippetSelectionTableWidget_cellChanged(int row, int column)
{
	snippet_labels[row] = snippetSelectionTableWidget->item(row,column)->text();
}

void MainWindow::refreshSnippetSelectionTable()
{
	snippetSelectionTableWidget->blockSignals(true);
	snippetSelectionTableWidget->clear();
	int row = 0;
	for( QString snip_label : snippet_labels){
		QTableWidgetItem * new_item = new QTableWidgetItem();
		new_item->setData(Qt::DisplayRole, QVariant(snip_label));
		new_item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsEditable | Qt::ItemIsSelectable);
		snippetSelectionTableWidget->setItem(row,0,new_item);
		row++;
	}
	snippetSelectionTableWidget->blockSignals(false);
	// set first snippet
	snippetTextEdit->setText(snippet_text[0]);
	snippetSelectionTableWidget->setCurrentCell(0,0);
//	snippetSelectionTableWidget->setCurrentIndex(snippetSelectionTableWidget->model()->index(0, 0));
}

void MainWindow::on_actionLoadSnippets_triggered()
{
	// clear the status bar and set the normal mode for the pixmapWidget
	statusBar()->clearMessage();
	// ask the user to add files
	QString file_name = QFileDialog::getOpenFileName(
			this,
			"Load existing snippets",
			database_path,
			"Snippet File (*.ini)");

	if (file_name.isEmpty())
		return;

	loadSnippets(file_name, true);
}

void MainWindow::on_actionSaveSnippetsAs_triggered()
{
	statusBar()->clearMessage();

	// ask the user to add files
	QString file = QFileDialog::getSaveFileName(
					   this,
					   "Save snippets as ...",
					   database_path,
					   "Snippets file (*.ini");

	if ( file.isEmpty() )
		return;

	// check wether an extension has been given
	if ( QFileInfo(file).suffix().isEmpty() )
		file += ".ini";

	saveSnippets( file );

	// update the statusbar
	statusBar()->showMessage("Saved snippets to file " + file, 5 * 1000);

    // save the last database path
    database_path = QFileInfo(file).absolutePath();
    snippets_filename = file;
//    saveSerialization();
}

void MainWindow::on_actionSetAnnoOptions_triggered()
{
    xml_options->exec();
}

void MainWindow::on_snippetTextEdit_textChanged()
{
    emit sthChanged();
    snippet_text[current_snippet] = snippetTextEdit->toPlainText();
}


void MainWindow::on_actionExtractSnippets_triggered()
{
    SnippetsDialog *sd = new SnippetsDialog(this,
                                            database_path,
                                            annotations,
                                            snippet_labels,
                                            snippet_text,
                                            snippet_extraction_folder);
    if( sd->exec() == QDialog::Accepted)
    {
        snippet_extraction_folder = sd->getOutput_folder();
    }
}
