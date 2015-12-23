# puhma
Tools for working with papal charters of the middle ages (Papsturkunden des hohen Mittelalters - puhma).

This contains the following tools:
- annotation tool for annotating data and export to XML
- snippet-extraction tool from XML data
- layout-stats tool from XML data
- plot tool to plot statistics gathered by the layout tool
- score tool to rate images (good, bad, ugly)

## requirements
Hardware: each modern PC should work fine, the snippet-extraction-tool and the layout-stats-tool might need (depending on the amount of data and size of images) 
more than 8gb RAM. 
Software: 
* Python 2.x (>= 2.7)

For the Python-tools the following python-Packages are required:
* numpy / scipy
* OpenCV 2.4.x
* sklearn
* progressbar

For the score tool:
* Qt 4.x

For the annotation tool:
* Qt 4.x
* OpenCV 2.4.x
* Boost > v1.49

## Annotation Tool
For compatibility reasons there are currently two different versions, 0.3 contains the annotation tool as it was used in the project (plus an
automatic word recognition).The XML files which were created from this tool are used in the python-tools. 
Version 0.5 is currently not creating the same output XML files (we are working on this), however it has support for extracting
annotated snippets right away. Furthermore, the automatic word recognition handling is much simplified. It is also much more flexible than before.

### Installation
The annotation tool uses some functionality of the vole-framework which also serves as the base module. So call cmake with vole
and add as additional modules 'common', 'annotated' and 'attributes'.  
