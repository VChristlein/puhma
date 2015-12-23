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
    configdialog
    configfvdialog
    configasdialog
    configcsdialog
    processdialog
	ImgAnnotations
    imageChoper
    recogModel
    recogWizard
    recogThread
)

vole_moc_library(
	MainWindow.h
	AnnotationsPixmapWidget.h  
	ImgAnnotations.h  
	PixmapWidget.h  
	ScrollAreaNoWheel.h
	functions.h  
	propertydialog.h
    configdialog.h
    configfvdialog.h
    configasdialog.h
    configcsdialog.h
    processdialog.h
    imageChoper.h
    recogModel.h
    recogWizard.h
    recogThread.h
)

vole_ui_library(
	MainWindow.ui
	propertydialog.ui
    processdialog.ui
    configdialog.ui
    configfvdialog.ui
    configcsdialog.ui
    configasdialog.ui
    recogResultPage.ui
)

vole_add_resources(
	icons.qrc
)

vole_add_executable("puhma_annotate" "main")

vole_add_module()

