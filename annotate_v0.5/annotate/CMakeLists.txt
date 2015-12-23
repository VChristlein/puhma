add_definitions(-DBOOST_TT_HAS_OPERATOR_HPP_INCLUDED)

vole_module_name("puhma_annotate_gui")
vole_module_description("GUI for annotations of text images")
vole_module_variable("Puhma_Annotate_Gui")

vole_add_required_dependencies("BOOST" "BOOST_PROGRAM_OPTIONS" "OPENCV" "QT" "QT_OPENGL" "OPENGL" "QT_XML")
vole_add_required_modules("puhma_attributes")
vole_add_optional_modules()

vole_compile_library(
	MainWindow
	AnnotationsPixmapWidget
	PixmapWidget
	ScrollAreaNoWheel
	functions 
	propertydialog
	ImgAnnotations
    imageChoper
    recogModel
    prepareWizard
    preparetypepage
    preparecheckpage
    preparefinalpage
    preparedatasetpage
    prepareprocesspage
    preparefvpage
    prepareaspage
    preparecspage
    preparephocpage
    processthread
    recogWizard
    recogThread
    xmldialog
    snippetsdialog
)

vole_moc_library(
	MainWindow.h
	AnnotationsPixmapWidget.h  
	PixmapWidget.h  
	ScrollAreaNoWheel.h	 
	propertydialog.h
    recogWizard.h
    prepareWizard.h
    preparetypepage.h
    preparecheckpage.h
    preparefinalpage.h
    preparedatasetpage.h
    preparefvpage.h
    prepareprocesspage.h
    preparephocpage.h
    prepareaspage.h
    preparecspage.h
    processthread.h
    recogThread.h
    xmldialog.h
    snippetsdialog.h
)

vole_ui_library(
	MainWindow.ui
	propertydialog.ui
    preparetypepage.ui
    preparecheckpage.ui
    preparefinalpage.ui
    preparedatasetpage.ui
    preparefvpage.ui
    prepareprocesspage.ui
    preparephocpage.ui
    prepareaspage.ui
    preparecspage.ui
    recogResultPage.ui
    xmldialog.ui
    snippetsdialog.ui
)

vole_add_resources(
	icons.qrc
)

vole_add_executable("puhma_annotate" "main")

vole_add_module()

