#!/usr/bin/python2

"""
author: Vincent Christlein
lincense: GPL v.2.0, see LICENSE

description:
    this tool extracts snippets from XML-files
    which were created by the annotation tool
"""


import glob
import numpy as np
import xml.dom.minidom as xml
#import xml.etree.ElementTree as xml
#from xml.etree.ElementTree import ElementTree
import sys
import cv2
import argparse
import os
import re
import errno
import gzip
import cPickle

def parserArguments(parser):    
    parser.add_argument('xml', help='the xml file with annotations to parse. The folder containing xml-files or the folder \
                    containing folders with xml works, too.')
    parser.add_argument('output_folder', help='output folder for the snippets')
    
    group1 = parser.add_argument_group('output options', 'Specify the output-format of the snippets')
    group1.add_argument('--skip', action='store_true',
                        help='skip img file if there exist any snippet in the'
                        ' output_folder')
    group1.add_argument('-p', '--prefix', help='prefix for the output-snippets, standard is the basename of the xml-file. \
                    i.e.: output_folder/xml-filename_id.png')
    group1.add_argument('-e', '--encode_filters', action='store_true', help='encode the filters in the filename, i.e. \
                    tag(-f) (of the corresponding tag you have filtered with) \
                    and value in the filepath and img (-i) and (-y)\
                    i.e.: {output_folder}/{prefix}_{img}_{year}_{id}_{tag}_{value}.png')

    group1.add_argument('-m', '--mask', action='store_true', help='set pixels out of the \
                    polygon (defined by fixpoints) to white')
    group1.add_argument('-d', '--debug_img', action='store_true', help='writes debug image(s) in which all selected objects \
                    are marked in the current folder')
    group1.add_argument('-b', '--binarize', action='store_true', help='binarize each snippet independently with Otsu-Algorithm \
                    after grayscale-conversion')
    group1.add_argument('-s', '--size', help='specify output-size of each snippet, too small ones will be padded with white pixels, too large ones \
                    will be resized. Note: the given size is assumed to be for both dimensions; otherwise give two integers seperated by \'x\' \
                    , i.e. WIDTHxHEIGHT, e.g. 40x60')
    group1.add_argument('--all_in_one', action='store_true', help='write the cut snippets all in one big image file')
    group1.add_argument('--all_in_one_width', default=800, type=int, help='width of resulting image')

    group2 = parser.add_argument_group('filtering options', 'Options for filtering the object-nodes containing the snippet \
                    bounding box / polygon-points')
    group2.add_argument('-i', '--img', nargs='*', help='select certain image(s) by name')
    group2.add_argument('-y', '--year', '--date', nargs=2, type=int, help='the filenames contain \
                    typically a yeari or date, you need to set here a range of years/dates (either in the format YYYY or YYYYMMDD. \
                    Note: this may conflict with the `img`-option.')
    group2.add_argument('-f', '--filter', nargs='*', help='select specific objects which \
                    have children w. this tag (you can also select tags with a certain value: tag@value), e.g. graph@a.\
                    If you append another \'@\' at the end of the tag-value it will be searched exactly for that pattern,\
                    otherwise all patterns having the specified value will be matched.')
    group2.add_argument('--area', nargs='*', help='select another tag@value pair(s) - the objects which are lying outside these areas will be removed')
    
    
    group3 = parser.add_argument_group('training options', 'Specific options for the classification task to\
            generate more snippets and save in a different format')
    group3.add_argument('-t', '--translation', nargs=2, type=int, help = "Generate additional snippets by applying translations. \
                    The first argument is the maximum range and the second one the step size. \
                    The original snippet will be translated in all possible combinations.")
    group3.add_argument('-r', '--rotation', nargs=2, type=int, help = "Generate even more snippets (rotated ones). Syntax similar to translation.")
    group3.add_argument('-n', '--negative_samples', action='store_true', help='generate negative samples, i.e. from all positions in the image\
                    except the ones which you have selected with -f, all other parameters are the same (+step). Note: --area has no effect here.')
    group3.add_argument('--step', type=int, help='step between each negative sample')
    group3.add_argument('--iffilter', action='store_true', help='extract snippets only if the filter also applies \
            (useful if not all the data is labelled, then only from those images where the filter (-f) has hits will be used')
    group3.add_argument('--pickle', action='store_true', help = "write output not as images but as gzipped and pickled row-wise saved snippets per xml-file, \
                    this can be used for easier loading in the \
                    training stage. Note gives one file per xml.")

    args = parser.parse_args()

    return args

# note 1: maybe we need the additional fields later    
# note 2: this whole thing fails if the filename-format is wrong
def splitFileName(file_name):
    file_name = os.path.splitext(os.path.basename(file_name))[0]
    parts = re.split('[(_ )]+', file_name)
    ret = {}
    if len(parts) == 1:
        return ret
    # the jaffe nr
    ret['jaffe'] = parts[0]
    # the date encoded YYYYMMDD
    try:
        ret['date'] = int(parts[1])
    except ValueError:
        ret['date'] = int(parts[1].split()[0][:4] + '0000')
        print "WARNING: filename is in an old format (date not as YYYYDDMM), can only compare the year " + str(ret['date'])

    # now we need to go from the back since the stuff is not 
    # encoded in the best way. Actually now the first three 
    # words of the document come in which
    # we're not interested at all
    last = len(parts)-1
    try:
        int(parts[last])
        ret['imgnr'] = parts[last]
        last -= 1
    except ValueError:
        pass
    # the archive from which the document stems
    ret['source'] = parts[last]
    last -= 1
    # file type, e.g. PH = photo
    ret['type'] = parts[last]

    return ret

# todo: instead of reading in complete the xml-file,
#    apply that filter immediatly
def filterByName(file_nodes, img_names):
    for i in reversed(xrange(len(file_nodes))):
        filename = file_nodes[i].attributes["filename"].value    
        for name in img_names:
            if filename.find(name) == -1:
                file_nodes.remove(file_nodes[i])
    return file_nodes

def filterByYear(file_nodes, year_range):    
    for i in reversed(xrange(len(file_nodes))):
        filename = file_nodes[i].attributes["filename"].value    
        parts = splitFileName(filename)
        if len(year_range[0]) !=4 or len(year_range[0]) != 8\
            or len(year_range[1]) !=4 or len(year_range[1]) != 8:
            print 'one of the years is not in the correct format (YYYYMMDD or YYYY)'
        year0 = year_range[0] + '0000' if len(year_range[0]) == 4 else year_range[0]
        year1 = year_range[1] + '0000' if len(year_range[1]) == 4 else year_range[1]
        if parts['date'] < year0 or parts['date'] > year1:
            file_nodes.remove(file_nodes[i])
    return file_nodes            

def getObjNodeDict(obj):
    obj_node_dict = {}

# TODO: remove that     
    bbox_nodes = obj.getElementsByTagName('bbox')
    poly_nodes = obj.getElementsByTagName('fixpoints')
    if (bbox_nodes and poly_nodes) or len(bbox_nodes) > 1 or len(poly_nodes) > 1:
        raise ValueError('WARNING: The Object w. id: ' + str(obj.attributes['id'].value) \
            + 'has more than one <bbox> or <fixpoints> tag! Fix your xml!')
                
    if bbox_nodes:
        obj_node_dict['bbox'] = bbox_nodes[0].childNodes[0].data
        obj_node_dict['poly'] = ''
    else:
        obj_node_dict['bbox'] = ''
        if not poly_nodes:
            raise ValueError("WARNING: neither bbox nor fixpoints-tag found for obj w. id:"\
                + str(obj.attributes['id'].value) + '! Fix your xml!')
        obj_node_dict['poly'] = poly_nodes[0].childNodes[0].data

    obj_node_dict['id'] = obj.attributes['id'].value

# TODO: write this w. element-tree functionality    
#    obj_node_dict['tag'] = []
#    obj_node_dict['value'] = []
#    for child in obj.childNodes:    
#        print child.nodeName, nodeValue
#        if not child.firstChild:
#            raise ValueError("WARNING: no value for "\
#                + str(obj.attributes['id'].value) + '! Fix your xml!')
#        value = child.firstChild.value
#        if not value:
#            raise ValueError("WARNING: no value for "\
#                + str(obj.attributes['id'].value) + '! Fix your xml!')
#
#        if child.data == 'bbox':
#            obj_node_dict['bbox'] = value
#        if child.data == 'fixpoints':
#            obj_node_dict['poly'] = value
#
#        object_node_dict['tag'].append(child.data)
#        object_node_dict['value'].append(value)
#
#    if (bbox_nodes and poly_nodes) or len(bbox_nodes) > 1 or len(poly_nodes) > 1:
#        raise ValueError('WARNING: The Object w. id: ' + str(obj.attributes['id'].value) \
#            + 'has more than one <bbox> or <fixpoints> child! Fix your xml!')
#    if not bbox_nodes and not poly_nodes:
#        raise ValueError("WARNING: neither bbox nor fixpoints-child found for obj w. id:"\
#            + str(obj.attributes['id'].value) + '! Fix your xml!')
#    if not object_node_dict['tag'] or not object_node_dict['value']:
#        raise ValueError("WARNING: no child or value for "\
#            + str(obj.attributes['id'].value) + '! Fix your xml!')

    return obj_node_dict

# todo this got somewhat ugly, maybe refactor this
def filterObjectNodes(obj_nodes, filters):
    all_filters_applied = True
    obj_list = []
    if filters:
        for filter in filters:            
            obj_for_filter = False
            for obj in obj_nodes:
                subfilter = filter.split('&')
                
                for subfilt in subfilter:
                    tag_value = subfilt.split('@',1)
                    tag_nodes = obj.getElementsByTagName(tag_value[0])
                    if not tag_nodes:
                        continue
                    for t in tag_nodes:
                        # tag or check value if we want one
                        if len(tag_value) > 1:
                            exact = tag_value[1].endswith('@')
                            tag_value[1] = tag_value[1].replace('@', '')
                        if len(tag_value) == 1 or (len(tag_value) == 2 \
                                and (exact == True and t.firstChild.nodeValue == tag_value[1])\
                                or (exact == False and t.firstChild.nodeValue.find(tag_value[1]) != -1)):
                            try:
                                obj_node_dict = getObjNodeDict(obj)
                            except ValueError as e:
                                print e
                                continue
                            obj_for_filter = True
    # TODO: remove that and rewrite                          
                            if obj_node_dict in obj_list:
                                index = obj_list.index(obj_node_dict)
                                obj_list[index]['tag'].append(tag_value[0])
                                obj_list[index]['value'].append(t.firstChild.nodeValue)
                            else:
                                obj_node_dict['tag'] = [ tag_value[0] ]
                                obj_node_dict['value'] = [ t.firstChild.nodeValue ]
                                obj_list.append(obj_node_dict)
                        
                
            if not obj_for_filter:
                all_filters_applied = False
    else:                    
        for obj in obj_nodes:
            try:
                obj_node_dict = getObjNodeDict(obj)
            except ValueError as e:
                print e
                continue

            obj_list.append(obj_node_dict)
    return obj_list, all_filters_applied

def getObjectsOfXML(xml_file, img_names, year_range, filter):

# TODO: exchange that w. below    
    dom = xml.parse(xml_file)    
    file_nodes = dom.getElementsByTagName('file')

# TODO:    
#    tree = ElementTree() 
#    file_nodes = tree.findall('filename')

    # apply filters
    if img_names:
        file_nodes = filterByName(file_nodes, img_names)

    if year_range:
        file_nodes = filterByYear(file_nodes, year_range)

    
    # process all file-nodes
    file_to_object = {}
    for node in file_nodes:
        filename = node.attributes["filename"].value
        obj_nodes = node.getElementsByTagName('object')
        # filter all obj-nodes
        obj_nodes_list, all_filters_applied = filterObjectNodes(obj_nodes, filter)
        if len(obj_nodes_list) == 0:
            print '\tNote: have no objects for: ',filename.encode('utf8')
        file_to_object[filename] = (obj_nodes_list, all_filters_applied)
    
    return file_to_object


def mkdir_p(path):
    try:
        os.makedirs(path)
    except OSError as exc:
        if exc.errno == errno.EEXIST:
            pass
        else: raise

def realSize(size, snippet):
    w_h = size.split('x', 1)
    w = float(w_h[0])
    h = float(w_h[1]) if len(w_h) == 2 else w
    # first resize so that largest dimension fits bounding box
    # if we give a scale-factor instead of absolute values
    if w < 5 and h < 5:
        h = int(h * snippet.shape[0])
        w = int(w * snippet.shape[1])
    else:
        w = int(w)
        h = int(h)
    ry = float(snippet.shape[0])/h
    rx = float(snippet.shape[1])/w
    return w, h, rx, ry

# resize snippet, pad if necessary, and binarize 
def modifySnippet(snippet, size, binarize):
    if snippet is None:
        print "A WARNING: snippet is None"
        return None
    if snippet.ndim == 3:
        bak = snippet
        snippet = cv2.cvtColor(snippet, cv2.COLOR_BGR2GRAY)
        if snippet is None:        
            print bak.dtype
            print "B WARNING: snippet is None"
            cv2.imshow("snipp", bak)
            cv2.waitKey()
            return None
    if binarize:
        ret, snippet = cv2.threshold(snippet, 125, 255, cv2.THRESH_OTSU | cv2.THRESH_BINARY)
        if snippet is None:
            print "C WARNING: snippet is None"
            return None
    if size:
        w, h, rx, ry = realSize(size, snippet)
        if rx > ry:                
            snippet = cv2.resize(snippet, (w, int(snippet.shape[0]/rx)))
        else:
            snippet = cv2.resize(snippet, (int(snippet.shape[1]/ry), h))
        crop = snippet
        if crop.shape[0] != h or crop.shape[1] != w:
            diffW = w - crop.shape[1]
            diffH = h - crop.shape[0]
            top = int(diffH/2.0 + 0.5)
            bottom = diffH/2
            left = int(diffW/2.0 + 0.5)
            right = diffW/2
            if top+bottom+crop.shape[0] != h or left+right+crop.shape[1] != w or diffW < 0 or diffH < 0:
                print "WARNING: sth went wrong with the padding step, print various variables:"
                print top, bottom, crop.shape[0], left, right, crop.shape[1]
            paddedCrop = cv2.copyMakeBorder(crop, top, bottom, left, right, cv2.BORDER_CONSTANT, value=255)
            snippet=paddedCrop
    
    return snippet

def generateVariation(snippet, translation, rotation):
    """ translation: tuple with (max, (step)) translations
    rotation: tuple with (max, (step)) rotation angles"""
    if translation:
        if len(translation) > 1:
            translations = [i for i in range(-translation[0], translation[0]+1, translation[1]) if not i==0]
        else:
            translations = [i for i in range(-translation[0], translation[0]+1) if not i==0]
    else:
        translations = []
    if rotation:
        if len(rotation) > 1:
            rotations = [i for i in range(-rotation[0], rotation[0]+1, rotation[1]) if not i==0]
        else:
            rotations = [i for i in range(-rotation[0], rotation[0]+1) if not i==0]
    else:
        rotations = []
#    print rotations
    
    rotated_snippets = {}
    for rot in rotations:
        rotationMatrix = cv2.getRotationMatrix2D( (snippet.shape[1]/2., snippet.shape[0]/2.), rot, 1. )
        rotated_snippets[rot] = cv2.warpAffine(snippet, rotationMatrix, snippet.shape[:2], borderMode = cv2.BORDER_CONSTANT, borderValue = 255 )
    rotated_snippets[0] = snippet
#    print len(rotated_snippets)
    translated_snippets = {}
    for trans in translations:
        for rot, r_snippet in rotated_snippets.iteritems():
        #first translation in x
            b = np.ones(snippet.shape, snippet.dtype)*255
            if trans<0:
                b[:,:trans] = r_snippet[:, -trans:]
            else:
                b[:, trans:] = r_snippet[:, :-trans]
            translated_snippets[(trans, 0, rot)] = b
        #second translation in y
            b = np.ones(snippet.shape, snippet.dtype)*255
            if trans<0:
                b[:trans,:] = r_snippet[-trans:,:]
            else:
                b[trans:,:] = r_snippet[:-trans,:]
            translated_snippets[(0, trans, rot)] = b
        #third translation in x & y
            for trans2 in translations:
                if (trans, trans2) not in translated_snippets.keys():
                    b = np.ones(snippet.shape, snippet.dtype)*255
                    if trans<0:
                        if trans2<0:
                            b[:trans,:trans2] = r_snippet[-trans:,-trans2:]
                        else:
                            b[:trans,trans2:] = r_snippet[-trans:,:-trans2]
                    else:
                        if trans2<0:
                            b[trans:,:trans2] = r_snippet[:-trans,-trans2:]
                        else:
                            b[trans:,trans2:] = r_snippet[:-trans,:-trans2]
                    translated_snippets[(trans, trans2, rot)] = b
    return translated_snippets


def getRealPath(filepath, xml_path):    
    filepath = filepath.encode('utf-8')
    orig_path = filepath
    # typically the filename is encoded without an ending in the xml file
    # and as relative path, however it may also be an absolut path and/or
    # have a file-ending
    # 1. fix path
    if not os.path.isabs(filepath):
        # note: probably os.path.join would be sufficient
        #filepath = os.path.normcase(os.path.normpath(os.path.join(xml_path, filepath)))
        filepath = os.path.join(xml_path, filepath)

    # 2. check file-ending
    ext = os.path.splitext(filepath)[1]
    if ext == '' or not os.path.exists(filepath): # no extension given or file not found
        file_found = False
        # try several extensions (prefer high quality)
        for e in ['.tif', '.png', '.jpg']:
            filepath = os.path.splitext(filepath)[0] + e
            if os.path.exists(filepath):
                file_found = True
                break
        if not file_found:
          return None
    return filepath

def cutSnippet(obj, img, use_mask, debug_img, mark_matrix):
    snippet = None
    bbox_str = obj['bbox']
    #preocess  bbox               
    if bbox_str != '':
        # split string and round to nearest int                
        bbox = np.array(np.rint( np.array(map(float, bbox_str.split(','))) ),
                               np.int32)
        bbox[ bbox < 0 ] = 0
        if len(bbox) != 4:
            print "WARNING: wrong number of points for a rectangle"
            return None
        snippet = img[bbox[1]:bbox[1]+bbox[3], bbox[0]:bbox[0]+bbox[2]]
        # debug
        if debug_img is not None:
            cv2.rectangle(debug_img, (bbox[0],bbox[1]), (bbox[0]+bbox[2],bbox[1]+bbox[3]), (255,0,0), 3)
        if mark_matrix is not None:
            cv2.rectangle(mark_matrix, (bbox[0],bbox[1]), (bbox[0]+bbox[2],bbox[1]+bbox[3]), (1,1,1), cv2.cv.CV_FILLED)
    # process polygon
    else:
        fp_str = obj['poly']
        if fp_str == '':
            print "WARNING: sth went terribly wrong: obj['poly'] == ''  - check your code"
            return None
        # split string and round to nearest int
        fixpts = np.array(np.rint( np.array(map(float, fp_str.split(','))) ),
                                 np.int32)
        fixpts[ fixpts < 0] = 0
        if len(fixpts) == 0:
            print "WARNING: fixpts-length == 0"
            return None
        if len(fixpts) % 2 != 0:
            print "WARNING: odd number of fixpoints, can't build points"
            return None
        # bring points in the correct format
        fixpts = np.reshape(fixpts, (-1,2))

        # note: cv2.boundingRect wants this format: [[[1,2]],[[3,5]],...] and as numpy-array
        # note: maybe the format [[[1,2],[3,5],..]] also does work
        # what a fuckup?! -> probably comes from internally handling this as matrix
        fixpts_cv = np.array([[x] for x in fixpts.tolist()], dtype=np.int32)
        x,y,width,height = cv2.boundingRect(fixpts_cv)
        bbox = [x,y,width,height]

        if use_mask == True:
            if mark_matrix is not None:
                cv2.fillPoly(mark_matrix, np.array([[[fp] for fp in fixpts.tolist()]]), (1,1,1))
            snippet = img[y:y+height, x:x+width].copy()
            # relate fixpoints to origin
            fixpts -= np.array([x,y], np.int32)
            # note: here we need another [] around the points
            fixpts_cv = np.array([[[fp] for fp in fixpts.tolist()]])
            maski = np.zeros(snippet.shape, np.uint8)
            cv2.fillPoly(maski, fixpts_cv, (1,1,1))
            snippet[maski == 0] = 255                    
        else: 
            snippet = img[y:y+height, x:x+width]
            if mark_matrix is not None:
                cv2.rectangle(mark_matrix, (bbox[0],bbox[1]), (bbox[0]+bbox[2],bbox[1]+bbox[3]), (1,1,1), cv2.cv.CV_FILLED)
        # debug
        if debug_img is not None:
            cv2.rectangle(debug_img, (x,y), (x+width,y+height), (255,0,0), 3)
    
    return snippet, bbox

def computeSelectionMask(filepath, object_nodes, all_filters_applied, xml_path, iffilter):
    file_selection_mask = {}

    if iffilter and not all_filters_applied:
        print ' - skip', filepath.encode('utf8'), ' due to iffilter ( computeSelectionMask() )'
        return
    real_filepath = getRealPath(filepath, xml_path)
    if not real_filepath:
        real_filepath = getRealPath(filepath.lower(), xml_path)
    if not real_filepath:
        print 'WARNING: computeSelectionMask: can not get real path for ' + filepath.encode('utf-8') + ' --> skip'
        return
    
    filepath = real_filepath

    img = cv2.imread(filepath)
    if img is None:
        print "WARNING: computeSelectionMask: cannot open " + filepath + " --> skip "
        return

    if filepath in file_selection_mask:
        mark_matrix = file_selection_mask[filepath]
    else:
        mark_matrix = np.zeros( (img.shape[0], img.shape[1]), np.int8 )

    for obj in object_nodes:
        # write in mark_matrix
        snip, bbox = cutSnippet(obj, img, True, None, mark_matrix)

    file_selection_mask[filepath] = mark_matrix

    cv2.imwrite('file_selection_mask.png', mark_matrix * 255)
            
    return file_selection_mask            


def getSnippets(filepath, object_nodes, all_filters_applied, use_mask, size,
                binarize, xml_path, write_debug_img=None, translation=None, rotation=None,
                selection_matrix=None):
    snippet_of_file = {}
    try:
        date = splitFileName(filepath)['date']
    except:
        date = 0
    real_filepath = getRealPath(filepath, xml_path)
    if not real_filepath:
        real_filepath = getRealPath(filepath.lower(), xml_path)
    if not real_filepath:
        print 'WARNING: getSnippets: can not get real path for ' + filepath.encode('utf-8') + ' --> skip'
        return

    filepath = real_filepath

    img = cv2.imread(filepath)
    if img is None:
        print "WARNING: cannot open " + filepath + " --> skip "
        return

    print "+ process " + filepath    
    snippet_of_file[filepath] = []

    # for debug
    if write_debug_img:
        debug_img = img.copy()
        debug_img_mask = np.zeros((img.shape[0],img.shape[1]), dtype=np.uint8)
    else:
        debug_img = None
        debug_img_mask = None

    for obj in object_nodes:
        obj['filepath'] = filepath
        obj['date'] = date
        snippet, bbox = cutSnippet(obj, img, use_mask, debug_img, None)
        if selection_matrix:
            area = selection_matrix[filepath]
            box = np.zeros(area.shape, dtype=np.int8)
            cv2.rectangle(box, (bbox[0],bbox[1]), (bbox[0]+bbox[2],bbox[1]+bbox[3]), (1,1,1), cv2.cv.CV_FILLED)
            
            intersect = np.bitwise_and(box, area)        
            area_intersect = np.sum(intersect)
            if float(area_intersect) / float(bbox[2] * bbox[3]) < 0.7:
                continue            
        if write_debug_img:
            cv2.rectangle(debug_img_mask, (bbox[0],bbox[1]),
                          (bbox[0]+bbox[2],bbox[1]+bbox[3]), (255,255,255), cv2.cv.CV_FILLED)
        
        if snippet is None or snippet.size == 0:
            print "WARNING: snippet w. id:" + obj['id'] + "  is None or empty->skip - check params of bbox or fixpoints"
            continue
        snippet = modifySnippet(snippet, size, binarize)
        obj['snippet'] = snippet
        obj['bounding'] = bbox
        # additional snippets
        add_snippets = generateVariation(snippet, translation, rotation)
        obj['add_snippets'] = add_snippets

        snippet_of_file[filepath].append(obj)
    print '\tgot ' + str(len(snippet_of_file[filepath])) + ' objects'

    # debug    
    if write_debug_img:
        dbg_name = os.path.splitext(os.path.basename(filepath))[0] + '_marked_snippets.png'
        if cv2.imwrite(dbg_name, debug_img):
            print 'DEBUG: wrote debug image (all selected objects\
                    are marked in the image w. blue(polygons) or red(boxes) squares: ' + dbg_name
        dbg_name2 = os.path.splitext(os.path.basename(filepath))[0] + '_masked_snippets.png'
        if cv2.imwrite(dbg_name2, debug_img_mask):
            print 'DEBUG: wrote 2nd debug image: binary mask\
                    of selected snippetst ' + dbg_name2
    return snippet_of_file

def negativeSamples(file_to_objects, step, size, binarize, xml_path, translation, \
            rotation, selection_matrix, filter, iffilter):
    
    snippet_of_file = {}
    assert(file_to_objects)
    count = 0
    for filepath, (all_objects, all_filter_applied) in file_to_objects.iteritems():
        if iffilter and not all_filter_applied:
            print ' - skip', filepath.encode('utf8'), ' due to iffilter ( negativeSamples() )'
            continue
        try:
            date = splitFileName(filepath)['date']
        except ValueError:
            date = 0
        real_filepath = getRealPath(filepath, xml_path)
        if not real_filepath:
            real_filepath = getRealPath(filepath.lower(), xml_path)
        if not real_filepath:
            print 'WARNING: getSnippets: can not get real path for ' + filepath.encode('utf-8') + ' --> skip'
            continue

        filepath = real_filepath

        snippet_of_file[filepath] = []

        img = cv2.imread(filepath)
        if img is None:
            print "WARNING: cannot open " + filepath + " --> skip "
            continue        
        w_h = size.split('x', 1)
        w = int(w_h[0])
        h = int(w_h[1]) if len(w_h) == 2 else w
        if w < 5 and h < 5:
            raise ValueError('cannot generate negative samples w. variable size')

        area = selection_matrix[filepath]                
        shape = img.shape
        for y in xrange(0, shape[0]-h, step):
            for x in xrange(0, shape[1]-w, step):
                obj = {}
                # check if area is part of the selection matrix
                # if yes --> skip
                # TODO: integrate the option 'iffilter'
                check_area = area[y:y+h, x:x+w]
                skip = False
                for i in np.nditer(check_area):
                    if i == 1:
                        skip = True
                if skip:
                    continue
                obj['id'] = str(count)
                count += 1
                obj['filepath'] = filepath
                obj['date'] = date
                obj['tag'] = 'no' # NOTE: why does it only print 'n_' instead of 'no_' ? 
                obj['value'] = '_'.join(map(filter)).replace('@', '_')
                snippet = img[y:y+h, x:x+w]
                snippet = modifySnippet(snippet, size, binarize)
                obj['snippet'] = snippet
                # additional snippets
                add_snippets = generateVariation(snippet, translation, rotation)
                obj['add_snippets'] = add_snippets
                snippet_of_file[filepath].append(obj)

        print '\tgot ' + str(len(snippet_of_file[filepath])) + ' negative samples for ' + filepath
    return snippet_of_file

def writeAllInOne(file_snippet_dict, output_folder, prefix, one_img_width):
    if not os.path.exists(output_folder):
        mkdir_p(output_folder)
    print '\ttry to create all-in-one image w. width: ', one_img_width

    rows = []
    gap_size = 3
    row_max_height = 0
    row_width = 0
    snippet_for_row = []
    for filename, snippets in file_snippet_dict.iteritems():
        for s in snippets:        
            snippet = s['snippet']
            if snippet.shape[1] > one_img_width:
                print '\tWARNING: cannot add snippet (id:', s['id'], 'of file:', os.path.basename(filename), ') to one single image, since its width (',snippet.shape[1],\
                        ') is larger than all_in_one_width (', one_img_width,')'
                continue
            # if row is full:
            if row_width + snippet.shape[1] + gap_size >= one_img_width:
                # put all snippets now in one row w. small gap between
                if snippet.ndim > 2: # note: a mix of both doesn't work, either 3 chans or 2!
                    row = np.zeros((row_max_height+gap_size, one_img_width, 3), snippet.dtype)
                else:
                    row = np.zeros((row_max_height+gap_size, one_img_width), snippet.dtype)
                col = 0
                for snip in snippet_for_row:
                    row[ 0:snip.shape[0] , col:col+snip.shape[1] ] = snip
                    col += snip.shape[1] + gap_size
                # append and reset rest
                rows.append(row)    
                snippet_for_row = []
                row_max_height = 0
                row_width = 0
            row_max_height = max(snippet.shape[0], row_max_height)
            row_width += snippet.shape[1] + gap_size
            snippet_for_row.append(snippet)
    # append last one
    if snippet_for_row: 
        if snippet.ndim > 2:
            row = np.zeros((row_max_height+gap_size, one_img_width, 3), snippet.dtype)
        else:
            row = np.zeros((row_max_height+gap_size, one_img_width), snippet.dtype)
        col = 0
        for snip in snippet_for_row:
            row[ 0:snip.shape[0] , col:col+snip.shape[1] ] = snip
            col += snip.shape[1] + gap_size
        rows.append(row)    

    if not rows:
        print 'no snippets to put in one image'
        return
    # now concatenate all rows:
    all_in_one = np.concatenate(rows)
    print '\tyour all-in-one image has the dimensions: (widthxheight):', all_in_one.shape[1], all_in_one.shape[0]

    path = os.path.join(output_folder, prefix + '_all_in_one.png')

    try:
        if not cv2.imwrite(path, all_in_one):
            print 'couldnt write ' + path
    except UnicodeEncodeError:
        print 'ERROR', path, type(path)
    print '\twrote all-in-one-img:', path


def writeSnippets(file_snippet_dict, output_folder, prefix, encode_filters, img, year, size):
    if not os.path.exists(output_folder):
        mkdir_p(output_folder)

    for filename, snippets in file_snippet_dict.iteritems():
        for snippet in snippets:
            in_prefix = prefix.decode('utf-8')
            in_prefix += '__' + os.path.splitext(os.path.basename(snippet['filepath']))[0]
            in_prefix += '__' + snippet['id']
            if encode_filters:
                if size:
                    w_h = size.split('x', 1)
                    w = int(w_h[0])
                    h = int(w_h[1]) if len(w_h) == 2 else w
                    in_prefix += '_' + str(w) + '_' + str(h)
                if year:
                    in_prefix += '_' + snippet['date']
            try:
                in_prefix += '__' + '_'.join([x+'_'+y for x,y in zip(snippet['tag'],snippet['value'])])
            except KeyError:
                pass
            in_prefix = re.sub('[^a-zA-Z0-9_.-\\\\\\\/]+', '_',\
                                   in_prefix)
            path = os.path.join(output_folder, in_prefix)
            #path.replace(' ', '_').encode('utf-8')
#            print 'write ' + path
            try:
                if not cv2.imwrite(path + '.png', snippet['snippet']):
                    print 'couldnt write ' + path +'.png'
            except UnicodeEncodeError:
                print 'ERROR', path, type(path)
            # save additional (translated and rotated snippets)
            for transRot, additional_snippets in snippet['add_snippets'].iteritems():
                path += '_' + '_'.join(map(str,transRot)) + '.png'
                try:
                    if not cv2.imwrite(path, additional_snippets):
                        print 'couldnt write ' + path
                except UnicodeEncodeError:
                    print 'ERROR', path, type(path)

def writePickle(file_snippet_dict, output_folder, prefix, encode_filters, year, dim):
    if not os.path.exists(output_folder):
        mkdir_p(output_folder)

    count = 0
    for snippets in file_snippet_dict.itervalues():
        for snippet in snippets:
            count += 1
            count += len(snippet['add_snippets'].values())

    w_h = size.split('x', 1)
    w = int(w_h[0])
    h = int(w_h[1]) if len(w_h) == 2 else w
    if h < 5 and w < 5:
        raise ValueError("h and w bot < 5 --> it is seen as a scaling factor, can't pickle variable size")

    # contains all snippets (row-wise)
    all_snippets = np.empty((count, w*h), dtype=np.uint8)    
    count = 0
    labels = []
    for filename, snippets in file_snippet_dict.iteritems():
        for snippet in snippets:
            all_snippets[count:count+1,:] = snippet['snippet'].flatten()

            label = '_'.join([x+'_'+y for x,y in zip(snippet['tag'],snippet['value'])])
            if year:
                in_prefix += '__' + '_'.join(map(str, year))
            labels.append(label)    

            count += 1
            # save additional (translated and rotated snippets)
            for add_snipp in snippet['add_snippets'].itervalues():
                all_snippets[count:count+1,:] = add_snipp.flatten()
                labels.append(label)
                count += 1

        path = os.path.join(output_folder, prefix + '.pkl.gz')            
    
        try:
            path = re.sub('[^a-zA-Z0-9_.-\\\\\\\/]+', '_', path)
        except UnicodeEncodeError:
            print 'ERROR', path, type(path)
            raise

        # write snippets as gzipped and pickled file
        f = gzip.open(path, 'wb')
        cPickle.dump((all_snippets,labels), f, -1)
        f.close()

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description="Parse an xml file\
            and extract the selected snippets")
    args = parserArguments(parser)
    
    if os.path.isdir(args.xml):
        all_xml_files = glob.glob(os.path.join(args.xml, '*', '*.xml')) \
                        if glob.glob(os.path.join(args.xml, '*', '*.xml')) else glob.glob(os.path.join(args.xml,'*.xml'))
    else:
        all_xml_files = [args.xml]
    if not all_xml_files:
        raise IOError('no xml-file (or (sub-)folder containing xml-file(s)) given')

    if args.skip:
        collect = glob.glob( os.path.join(args.output_folder, '*.png') )        
        collect = [ os.path.splitext(os.path.basename(p))[0].split('__')[1] for p in collect ]
    

    for xml_file in all_xml_files:        
        print '# get objects of the xml', xml_file
        file_object_dict = getObjectsOfXML(xml_file, args.img, args.year, args.filter)

        if args.area:
            file_object_dict_area = getObjectsOfXML(xml_file, args.img, args.year, args.area)        


        file_snippet_dict = {}
        for filepath, (object_nodes, all_filters_applied) in file_object_dict.iteritems():
            if args.skip and\
                os.path.splitext(os.path.basename(filepath))[0].lower()\
                in collect:
                print ('skip file'
                    ' {}'.format(os.path.basename(filepath)))
                continue
            
            if args.negative_samples:
                negative_samples_mask = computeSelectionMask(filepath,
                                                             object_nodes,
                                                             all_filters_applied, os.path.dirname(xml_file), args.iffilter)
                file_snippet_dict = negativeSamples(file_object_dict, args.step, \
                        args.size, args.binarize, os.path.dirname(xml_file), args.translation, args.rotation,\
                        negative_samples_mask, args.filter, args.iffilter)
            else:
                # get objects of the filter selected in 'args.area'
                selection_mask = None
                if args.area:
                    if not file_object_dict_area or len(file_object_dict_area) == 0:
                        print "- no area objects " + '_'.join(args.area) + " found --> skip"
                        continue
                    area_obj_nodes, area_all_filters_applied = file_object_dict_area[filepath]
                    selection_mask = computeSelectionMask(filepath,
                                                          area_obj_nodes,
                                                          area_all_filters_applied, os.path.dirname(xml_file), args.iffilter)
                    
                if not file_object_dict:
                    print "- no objects found or all filtered out -> skip"
                    continue
        
                print '# get the snippets of the objects'
                file_snippet_dict_per_file = getSnippets(filepath, object_nodes,
                                                all_filters_applied , args.mask, args.size, args.binarize,\
                                            os.path.dirname(xml_file), args.debug_img, args.translation, args.rotation, selection_mask)                   
            # should be put out of the loop? 
            if not file_snippet_dict_per_file:
                print "- no snippets found -> skip"
                continue
            if not args.all_in_one and not args.pickle:
                print '# write snippets'
                writeSnippets(file_snippet_dict_per_file, args.output_folder,\
                    os.path.splitext(os.path.basename(xml_file))[0] if not args.prefix else args.prefix,\
                    args.encode_filters, args.img, args.year, args.size)
            else:
                file_snippet_dict.update(file_snippet_dict_per_file)

        if args.all_in_one:
            print '# write all in one image'
            writeAllInOne(file_snippet_dict, args.output_folder,\
                    os.path.splitext(os.path.basename(xml_file))[0] if not args.prefix else args.prefix,\
                    args.all_in_one_width)
        if args.pickle:
            print  '# pickle'
            writePickle(file_snippet_dict, args.output_folder,\
                    os.path.splitext(os.path.basename(xml_file))[0] if not args.prefix else args.prefix,\
                    args.filter, args.year, args.size)
