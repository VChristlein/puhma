#ifndef MainWindow_H
#define MainWindow_H

#include <QMainWindow>
#include <QRectF>
#include <QCloseEvent>
#include <QDomDocument>

#include <string>
#include <map>

#include <ImgAnnotations.h>
#include "ui_MainWindow.h"
#include "ScrollAreaNoWheel.h"
#include "AnnotationsPixmapWidget.h"
#include "recogModel.h"

// TODO: make forward declarations
#ifndef Q_MOC_RUN
#include "attributes_config.h"
#include "attributes_core.h"
#endif


// forward declarations
class AnnotationsPixmapWidget;
class QTableWidgetItem;
class QDoubleSpinBox;
class QWheelEvent;
class XmlOptions;

class MainWindow : public QMainWindow, private Ui::MainWindow
{
	Q_OBJECT

public:
	enum Direction { Up, Down };

    MainWindow(QWidget *parent = 0, Qt::WFlags flags = 0);
    ~MainWindow();
    QString currentFilePath() const;
    QString currentDirPath() const;
    IA::File *currentFile() const;
    IA::ID currentObjectID() const;
    IA::Object *currentObject() const;
    IA::IDList fileObjectIDs() const;

    bool zoomAllowed();
protected:
	void closeEvent(QCloseEvent *event);
	void keyPressEvent(QKeyEvent * event);
	void keyReleaseEvent(QKeyEvent * event);
	void wheelEvent(QWheelEvent *event);

private:
	AnnotationsPixmapWidget *pixmapWidget;
	ScrollAreaNoWheel *scrollArea;

	QAction * browseModeAction;
	QAction * showPropertyMenuAction;
	QAction * allowZoomAction;

    // define it here as it has to be added by hand to the toolBar
    QDoubleSpinBox * zoomSpinBox;    
    QString lastFileAddImg,
            database_path,        
            database_filename,
            snippets_filename,
            current_img_filename,
            snippet_extraction_folder;

    XmlOptions * xml_options;

    /// gives dir for a specific row
    std::map<QTreeWidgetItem*, QString> item2dir;

	IA::ImgAnnotations annotations;
    QStringList propertyList,
                // e.g. id
                reservedProperties,
                filePropertyList;

	IA::Object *copiedObj;
    int objTableColumn;
    AnnotationsPixmapWidget::MouseMode saved_mouse_mode;
//    QDomDocument ontology_dom;
//    QTimer * autosave;
    bool allow_zoom;

	IA::ID objectIDAtRow(int iRow) const;
	IA::ID objectIDFromItem(QTableWidgetItem *item) const;
	IA::IDList selectedObjectIDs() const;
    void setCurrentTableItem(IA::ID objID); 
    /// builds up our qdomdocument
//    void buildDomDoc();
//    void ontologyToAnnotation();
    void saveDatabase( const QString & database_path );

    void loadSerialization();
    void saveSerialization();
    void loadDatabase(const QString &file_name, bool show_error);


    // recognition stuff
    void initAEConfig();
    //bool firstTimeOpenReco;
    void loadSerialization4Reco();
    void saveSerialization4Reco();
    ///CSR recognition
    puhma::AttriConfig* attri_config;
    puhma::AttributeEmbed* ae;
    RecogModel* rm;

    // snippet stuff
    QVector<QString> snippet_labels;
    QVector<QString> snippet_text;
    int current_snippet;

    void loadXmlAnno(QString file_path, bool show_error);
    void saveAnnoXml(QString file_path);

    void saveAnnoZone(QString file_path);

    void changeSnippet(int selected);
    void loadSnippets(const QString &file_name, bool show_error);
    void saveSnippets(const QString &snippet_path);

    void setSnippetAndZone(const QString &value);
    void refreshSnippetSelectionTable();
public slots:
//	void on_pixmapWidget_objectContentChanged(IA::ID);
//	void onWheelTurnedInScrollArea(QWheelEvent *);
	void on_actionOpenDatabase_triggered();
	void on_actionSaveDatabase_triggered();
	void on_actionSaveDatabaseAs_triggered();
	void on_actionQuit_triggered();
	void on_actionShortcutHelp_triggered();
	void on_actionCopyObj_triggered();
    void on_actionPasteObj_triggered();
    void on_actionShowImageFileBrowser_triggered();
    void on_actionShowObjectProperties_triggered();
    void on_actionShowXmlPreview_triggered();
    void on_addImgButton_clicked();
    void on_delImgButton_clicked();

//	void on_addPropertyButton_clicked();
//	void on_addFilePropertyButton_clicked();

//	void on_propertiesTableWidget_itemChanged();
//	void on_objTypeComboBox_currentIndexChanged(const QString &);
	void on_imgTreeWidget_currentItemChanged(QTreeWidgetItem *, QTreeWidgetItem *);
	void on_objTableWidget_itemSelectionChanged();
	void on_objTableWidget_currentItemChanged(QTableWidgetItem *, QTableWidgetItem *);
	void on_objTableWidget_itemChanged(QTableWidgetItem*);
//	void on_filePropertiesTableWidget_itemChanged(QTableWidgetItem*);
	void onPixmapWidgetActiveObjectChanged(IA::ID);
	void onScrollAreaWheelTurned(QWheelEvent*);
    void refreshImgView(QString last_file);
	void refreshObjView();
//	void refreshFilePropertiesView();
//	void refreshPropertiesView();
	void nextPreviousFile(Direction);
	void showQuickPropertyMenu();
    void addObj();
    void on_actionPropertydialog_triggered();


signals:
	void activeObjectChanged(IA::ID objID);
	void activeFileChanged(const QString& filePath);
	void selectedObjectsChanged(const IA::IDList& objIDs);
    void pasteObj(IA::ID obj);
    void sthChanged();
//private slots:
    //    void on_objTableWidget_customContextMenuRequested(const QPoint &pos);

protected slots:
    void onBackup();
    void onSthChanged();
//private slots:
    void showObjTableContextMenu(const QPoint & pos);
    void on_actionRemoveProperty_triggered();
    void showObjContextMenu(const QPoint & pos);
//    void on_imgTreeWidget_clicked(const QModelIndex &index);
    void on_actionCreateRectangle_triggered();
    void on_actionCreatePolygon_triggered();
    void on_actionRemoveObj_triggered();
    void on_actionZoomIn_triggered();
    void on_actionZoomOut_triggered();
    // check if zoom factor is high enough to activate zoomOutAction
    void checkZoom(double zoom_factor);

    void on_actionBrowsingMode_triggered();
    void on_actionAddObject_triggered();
//    void on_actionSaveOntologyAs_triggered();
//    void on_actionLoadOntology_triggered();
    void on_actionNewDatabase_triggered();

    ///CSR
    void on_actionPrepare_triggered();
    //void on_actionLabelEmbed_triggered();
    //void on_actionFisherVector_triggered();
    //void on_actionAttributeSpace_triggered();
    //void on_actionCommonSub_triggered();
    void on_actionRecognition_triggered();
    //void recogresultchoosed();
private slots:
    void on_actionLoadSnippets_triggered();
    void on_actionSaveSnippetsAs_triggered();
    void on_actionSetAnnoOptions_triggered();
    void on_snippetTextEdit_textChanged();
    void on_snippetSelectionTableWidget_cellChanged(int row, int column);
    void on_snippetSelectionTableWidget_itemSelectionChanged();
    void on_actionShowSnippetMenu_triggered();
    void on_actionExtractSnippets_triggered();
};

#endif
