#!/usr/bin/python
# -*- coding: utf-8 -*-

"""
author: Vincent Christlein
lincense: GPL v.2.0, see LICENSE

description:
    this tool extracts statistics of XML-files
    which were created by the annotation tool
"""

import snippets_from_xml_seriel as xml_snippets
import numpy as np
import argparse
import cv2
import os
import glob
import sys
import math

def lineHeights(origimg, filename, show=False, write=False, return_img=False):
    if origimg.ndim == 3:
        origimg = cv2.cvtColor(origimg, cv2.COLOR_BGR2GRAY)            
    img = origimg.astype(float)
    if np.max(img) > 1:
        img /= 255.0

    mean_height = 0.0
    std_height = 1.0 
    num_lines = 0
    col_range = None
    
    #col_range = (0,int(img.shape[1] / 4))
    #col_range = (int(img.shape[1]/8.0),int(img.shape[1] / 3.0))
    #profile = np.sum(img[:,col_range[0]:col_range[1]], axis=1)
    col_div = 3 
    for i in range(col_div):
        c_col_range = ( int( (float(i)   / col_div)*img.shape[1] * 0.45 ),
                        int( (float(i+1) / col_div)*img.shape[1] * 0.45 ))
        profile = np.sum(img[:,c_col_range[0]:c_col_range[1]], axis=1)
        # find best sigma value
        for s in range(21, 302, 10):
            c_g_profile = cv2.GaussianBlur(profile, (s,s), 0)
            # central difference
            minima = np.where( (c_g_profile[1:-1] < c_g_profile[:-2] ) \
                          & (c_g_profile[1:-1] < c_g_profile[2:]) )[0]
            # get local minima (except first and last line):
            heights = minima[1:] - minima[:-1]
            heights = heights[1:-1]
            if len(heights) == 0:
                break
            c_mean_height = np.mean(heights)
            c_std_height = np.std(heights)

            if len(minima) > 8 and len(minima) < 50 \
               and c_mean_height / c_std_height > mean_height / std_height:
                num_lines = len(minima) + 1
    #           and c_std_height < std_height:
                mean_height = c_mean_height
                std_height = c_std_height
                col_range = c_col_range
                g_profile = c_g_profile

        # if no proper lines found so far it's probably only 
        # an excerpt of a charter
        if not col_range:
            for s in range(7, 308, 10):
                c_g_profile = cv2.GaussianBlur(profile, (s,s), 0)
                # central difference
                minima = np.where( (c_g_profile[1:-1] < c_g_profile[:-2] ) \
                              & (c_g_profile[1:-1] < c_g_profile[2:]) )[0]
                # get local minima (except first and last line):
                heights = minima[1:] - minima[:-1]
                
                if len(heights) == 0:
                    break
                c_mean_height = np.median(heights)
                c_std_height = np.std(heights)

                if c_mean_height / c_std_height > mean_height / std_height:
                    num_lines = len(minima) + 1
        #           and c_std_height < std_height:
                    mean_height = c_mean_height
                    std_height = c_std_height
                    col_range = c_col_range
                    g_profile = c_g_profile

    if (show or write or return_img) and col_range != None:
        line_img = origimg.copy()
        #line_img = np.bitwise_not(line_img)
        line_img = cv2.cvtColor(line_img, cv2.COLOR_GRAY2BGR)
        line_img[:,col_range[0]:col_range[1]] /= 2
        # draw line boundings                      
        maxima = np.where( (g_profile[1:-1] > g_profile[:-2] ) \
                      & (g_profile[1:-1] > g_profile[2:]) )[0]
        minima = np.where( (g_profile[1:-1] < g_profile[:-2] ) \
                      & (g_profile[1:-1] < g_profile[2:]) )[0]
        for maxi in maxima:
            cv2.line(line_img, (0, maxi), (img.shape[1], maxi), (0,255,255), 3)
        # draw line masses
        minima = np.where( (g_profile[1:-1] < g_profile[:-2] ) \
                      & (g_profile[1:-1] < g_profile[2:]) )[0]
        for mini in minima:
            cv2.line(line_img, (0, mini), (img.shape[1], mini), (100,0,255), 3)

        show_img = cv2.resize(line_img, 
                              (int((800.0 / line_img.shape[0]) *\
                                   line_img.shape[1]), 800 ))
#                              show_img, 0.2, 0.2)
        if show:
            cv2.imshow('lines', show_img)
            cv2.waitKey()
        if write:
            cv2.imwrite(filename, show_img)

    if return_img:
        return num_lines, mean_height, std_height, line_img

    return num_lines, mean_height, std_height, None

def writeLineRotaImage(line_img, rota_snip, mean_height, fname):
    # get text size
    text_size, _ = cv2.getTextSize(
    '{:.2f}'.format(rota_snip.shape[0]/mean_height),
                cv2.FONT_HERSHEY_DUPLEX, 2, 2)
    # check shape
    if rota_snip.ndim < 3:
        rota_snip = cv2.cvtColor(rota_snip,
                                 cv2.COLOR_GRAY2BGR)
    # add ratio-text below the rota-snippet
    ext_rota_snip = np.full( (rota_snip.shape[0] + text_size[1], 
                              max(rota_snip.shape[1],
                                  text_size[0]), 3), 255,
                            dtype=np.uint8)
    ext_rota_snip[:rota_snip.shape[0],:rota_snip.shape[1]]\
        = rota_snip
    cv2.putText(ext_rota_snip,
                '{:.2f}'.format(rota_snip.shape[0]/mean_height),
                (0,rota_snip.shape[0]+text_size[1]),
                cv2.FONT_HERSHEY_DUPLEX, 2, (255,128,0),
               2)
    # add rota right of the line image
#    print line_img.shape, ext_rota_snip.shape
    line_rota_img = np.full( (max(line_img.shape[0],
                               ext_rota_snip.shape[0]),
                               line_img.shape[1]+ext_rota_snip.shape[1],
                              3), 255, dtype=np.uint8)
    line_rota_img[:line_img.shape[0],:line_img.shape[1]] = line_img
    line_rota_img[:ext_rota_snip.shape[0],
                  line_img.shape[1]:line_img.shape[1]+ext_rota_snip.shape[1]]\
                = ext_rota_snip

    # resize line img
    line_rota_img = cv2.resize(line_rota_img, 
          (int((800.0 / line_img.shape[0]) *\
           line_img.shape[1]), 800 ))

    cv2.imwrite(fname, line_rota_img)

def polygonFromString(fp_str):
    if fp_str == '':
        return None
    
    # split string and round to nearest int
    fixpts = np.array(np.rint( np.array(map(float, fp_str.split(','))) ), np.int32)
    if len(fixpts) == 0:
        print "WARNING: fixpts-length == 0"
        return None
    if len(fixpts) % 2 != 0:
        print "WARNING: odd number of fixpoints, can't build points"
        return None
        
    # bring points in the correct format
    fixpts = np.reshape(fixpts, (-1,2))

    return fixpts

def pointPolygonDistance(point, polygon):   
    if polygon is None:
        return None

    minDist = np.finfo(float).max
    for i in range(len(polygon)-1):
        v = polygon[i]
        w = polygon[i+1]
        l2 = float( np.sum((v-w)**2) )
        if l2 == 0: # case v == w -- shouldnt happen
            dist = float( np.sum( (v-point)**2) )
        else:
            t = np.dot(point-v, w-v) / l2
            if t < 0.0:
                dist = float( np.sum((v-point)**2) )
            elif t > 1.0:
                dist = float( np.sum((w-point)**2) )
            else:
                proj = v + t * (w-v)
                dist = float( np.sum((proj-point)**2) )
        
        if dist < minDist:
            minDist = dist
    return np.sqrt(minDist)

def pointSnippetDistance(point, snippet):
    if snippet['poly'] != '':
        polygon = polygonFromString(snippet['poly'])
        return pointPolygonDistance(point, polygon)
    else:
         dx = max(snippet['bounding'][0] - point[0], 0, 
                    point[0] - (snippet['bounding'][0]+snippet['bounding'][2]));
         dy = max(snippet['bounding'][1] - point[1], 0, 
                    point[1] - (snippet['bounding'][1]+snippet['bounding'][3]));
         return np.sqrt(dx*dx + dy*dy);
 
def maskFromPolygon(polygon, bbox):
    """
    return mask from polygon-points
    """
    if polygon is None:
        return None

    # note: cv2.boundingRect wants this format: [[[1,2]],[[3,5]],...] and as numpy-array
    # note: maybe the format [[[1,2],[3,5],..]] also does work
    # what a fuckup?! -> probably comes from internally handling this as matrix
#    fixpts_cv = np.array([[x] for x in polygon.tolist()], dtype=np.int32)

    # relate fixpoints to origin
    fixpts = polygon - np.array([bbox[0],bbox[1]], np.int32)
    # note: here we need another [] around the points
    # TODO: do we really??
    fixpts_cv = np.array([[[fp] for fp in fixpts.tolist()]])
    mask = np.zeros( (bbox[3], bbox[2]), np.uint8)
    cv2.fillPoly(mask, fixpts_cv, (1,1,1))

    return mask

def computeStats(img, mask, what=['bw_ratio', 'wh_ratio', 'area', 'extent',
                                  'solidity', 'orientation', 'eccentricity', 
                                  'equivalent', 'skew'] ):
    """
    computing different statistics from the image
    """
    if img is None: 
        return None
    snip = img.copy()
    if snip.ndim == 3:
        #TODO: replace w. mean
        snip = cv2.cvtColor(snip, cv2.COLOR_BGR2GRAY)    
#    snip = cv2.fastNlMeansDenoising(snip)
    ret, snip = cv2.threshold(snip, 125, 1, cv2.THRESH_OTSU |
                              cv2.THRESH_BINARY_INV)
    if snip is None:
        return None
    if mask is not None:
        snip[ mask==0 ] = 0
    stats = {} 
    if 'bw_ratio' in what:
        # Note: black and white are exchanged since we inverted 
        # the image when thresholding
        if mask is None:
            white = np.count_nonzero( (snip == 0) & (mask != 0) )
            black = np.count_nonzero( (snip == 1) & (mask != 0) )
        else:
            white = np.count_nonzero( snip == 0 )
            black = img.shape[0]*img.shape[1] - white
        stats['bw_ratio'] = 1.0 if white == 0 else black / float(white)

    if 'wh_ratio' in what:
        if hasattr(snip, 'shape'):
            stats['wh_ratio'] = snip.shape[1] / float(snip.shape[0])
        else:
            stats['wh_ratio'] = 0.0

    if 'area' in what\
       or 'extent' in what\
       or 'solidity' in what \
       or 'orientation' in what\
       or 'eccentricity' in what\
       or 'equivalent' in what:
        contours, hierarchy = cv2.findContours(snip.copy(),
                                           cv2.RETR_EXTERNAL,
                                           cv2.CHAIN_APPROX_NONE)
        largest_cnt = 0
        area = 0
        for i,c in enumerate(contours):
            ar = cv2.contourArea(c)        
            if ar > area:
                area = ar
                largest_cnt = i                
        if len(contours) > 0:
            cnt = contours[largest_cnt] # most-outer contour       
        else:
            cnt = None

    if 'area' in what:
        stats['area'] = area

    if 'extent' in what:
        if cnt is None:
            stats['extent'] = 0.0
        else:
            x,y,w,h = cv2.boundingRect(cnt)
            rect_area = w*h
            stats['extent'] = float(area)/rect_area

    if 'solidity' in what:
        if cnt is None:
            stats['solidity'] = 0.0
        else:
            hull = cv2.convexHull(cnt)
            hull_area = cv2.contourArea(hull)
            stats['solidity'] = float(area)/hull_area if hull_area > 0.0 else 0.0

    if 'orientation' in what\
       or 'eccentricity' in what:
        if cnt is not None and len(cnt) < 5:
            cnt = None
        if cnt is not None:
            (x,y),(MA,ma),angle = cv2.fitEllipse(cnt)
        if 'orientation' in what:
            if cnt is None:
                stats['orientation'] = 0.0
            else:
                stats['orientation'] = angle        
        if 'eccentricity' in what:
            if cnt is None:
                stats['eccentricity'] = 0.0
            else:
                ecc = MA / ma if ma > 0.0 else 0.0
                stats['eccentricity'] = math.sqrt(1 - ecc*ecc)

    if 'equivalent' in what:
        # equivalent diameter
        stats['equivalent'] = np.sqrt(4*area/np.pi)

    if 'skew' in what:
        m = cv2.moments(snip, True)
        stats['skew'] = m['mu11'] / m['mu02'] if m['mu02'] > 0.0 else 0.0

    return stats


def adjustBbox(img, bbox, mask=None):
    snip = img.copy()
    if snip.ndim == 3:
        snip = cv2.cvtColor(snip, cv2.COLOR_BGR2GRAY)
#    snip = cv2.fastNlMeansDenoising(snip)
    ret, snip = cv2.threshold(snip, 125, 255, cv2.THRESH_OTSU | cv2.THRESH_BINARY)

    """
    # remove too small areas
    min_area_size = 20
    for y in range(snip.shape[0]):        
        for x in range(snip.shape[1]):
            #ignore white and already visited pixels
            if snip[y,x] == 1 or snip[y,x] == 2:
                continue       
            # run floodfill on current location
            e_mask = np.zeros( (snip.shape[0]+2, snip.shape[1]+2), dtype=np.uint8)
            ret, rect = cv2.floodFill(snip, e_mask, (x,y), (3),
                                      (0,0,0), (0,0,0), 8)
            # see if area too small, if so mark it in our mask
            if ret < min_area_size:
                snip[ snip == 3 ] = 1
            else: # mark as visited
                snip[ snip == 3 ] = 2

    # reset visited pixels
    snip[ snip == 2 ] = 0
    """
    # this approach is much faster:
    # remove too small areas
    inv = np.bitwise_not(snip)
    contours, _ = cv2.findContours(inv, cv2.RETR_EXTERNAL, cv2.CHAIN_APPROX_NONE)
    remove_contours = []
    for cont in contours:
        if len(cont) < 20:
            remove_contours.append(cont)
    cv2.drawContours(snip, remove_contours, -1, (255), cv2.cv.CV_FILLED)   

    snip /= 255
    # cut out 
    for min_y in range(snip.shape[0]):
        if snip[min_y].sum() != snip.shape[1]:
            break
    for max_y in range(snip.shape[0]-1, -1, -1 ):
        if snip[max_y].sum() != snip.shape[1]:
            break
    for min_x in range(snip.shape[1]):
        if snip[:,min_x].sum() != snip.shape[0]:
            break
    for max_x in range(snip.shape[1]-1, -1, -1 ):
        if snip[:,max_x].sum() != snip.shape[0]:
            break
    
    ret =  bbox[0] + min_x, bbox[1] + min_y, max_x-min_x, max_y-min_y
    if mask is None:
        return (ret, img[min_y:max_y, min_x:max_x], None)
    else:
        return (ret, img[min_y:max_y, min_x:max_x], mask[min_y:max_y,min_x:max_x])
   

def parseArguments(parser):
    parser.add_argument('--show_lines', default=False, 
                         action='store_true', 
                         help='show line image (gray: area which is considered'
                         ', white lines: mass of line (used for counting), '
                         'black line: profile maxima')
    parser.add_argument('--write_lines', default=False,
                        action='store_true',
                        help='write the line image, s. show')
    parser.add_argument('--draw_rota_height', default=False,
                        action='store_true',
                        help='draw rota and lines')
    parser.add_argument('--adjust_bbox', default=False,
                        action='store_true',
                        help='not implemented in a good way yet: try to adjust the bounding box, such that the annotated bbox is' 
                            ' adjusted to the real contour of the object')
    parser.add_argument('--relative_to', 
                        help='specify element and its size to which everything'
                        ' is computed in relation, e.g. rota@2 meaning use rota'
                        ' as reference which is 2 [au] wide')
    parser.add_argument('--filter_obj', nargs='*',
                         default=['textbereich', 'graphische_symbole', 
                                  'wort' ],
                         help='use the specified tags. For all values of these'
                         ' tags we compute the statistics')
    parser.add_argument('--group_obj', nargs='*',
                        default=['wort'],
                        help='gruppiere alle statistiken von allen values von'
                        ' den angegbenen tags')
    return parser

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description="Parse (an) xml file(s)\
            and give some stats")
    parser = parseArguments(parser)
    args = xml_snippets.parserArguments(parser)
     
    if os.path.isdir(args.xml):
        all_xml_files = glob.glob(os.path.join(args.xml, '*', '*.xml')) \
                        if glob.glob(os.path.join(args.xml, '*', '*.xml')) \
                        else glob.glob(os.path.join(args.xml,'*.xml'))
    else:
        all_xml_files = [args.xml]
    if not all_xml_files:
        raise IOError('no xml-file (or (sub-)folder containing xml-file(s)) given')
                            
    if not os.path.exists(args.output_folder):
        xml_snippets.mkdir_p(args.output_folder)

    filter_obj = args.filter_obj

    # all objects with this tag-value-pair will be grouped together
    group_obj = args.group_obj  #['wort']

    for xml_file in all_xml_files:        
        print '# process', xml_file
        xml_path = os.path.dirname(xml_file)

        file_obj_dict = xml_snippets.getObjectsOfXML(xml_file, args.img, args.year, filter_obj)
        if not file_obj_dict:
            print "- no objects found or all filtered out -> skip"
            continue

        if args.area:
            file_object_dict_area = xml_snippets.getObjectsOfXML(xml_file, args.img, args.year, args.area)

        for filepath, (object_nodes, all_filters_applied) in file_obj_dict.iteritems():

            # get objects of the filter selected in 'args.area'
            selection_mask = None
            if args.area:
                if not file_object_dict_area or len(file_object_dict_area) == 0:
                    print "- no area objects " + '_'.join(args.area) + " found --> skip"
                    continue
                area_obj_nodes, area_all_filters_applied = file_object_dict_area[filepath]
                selection_mask = xml_snippets.computeSelectionMask(filepath,
                                                                   area_obj_nodes,
                                                                   area_all_filters_applied, 
                                                                   xml_path, args.iffilter)
    
            file_snippet_dict_per_file = xml_snippets.getSnippets(filepath, 
                                                                  object_nodes,
                                                                  all_filters_applied , 
                                                                  args.mask, args.size, args.binarize,\
                                                                  xml_path, args.debug_img, args.translation, args.rotation, 
                                                                  selection_mask)                   

            if not file_snippet_dict_per_file:
                print "- no xml_snippets found -> skip"
                continue

            for filename, snippets in file_snippet_dict_per_file.iteritems():
                if len(snippets) == 0:
                    print ('\t-> skip (no objects)') 
                    continue
                
                # create dictionary: value - snippet 
                value_dict = {}
                for snippet in snippets:
                    # if object should be grouped, create an entry in value_dict
                    # with its tag instead of the value and append all snippets
                    if snippet['tag'][0] in group_obj:
                        value_dict.setdefault(snippet['tag'][0],
                                              []).append(snippet)
                    else:
                        value_dict.setdefault(snippet['value'][0],
                                              []).append(snippet)
              
                stats = {}
                # get lines of kontext 
                if 'kontext' in value_dict:
                    # search for largest kontext snippet
                    max_snip_size = 0
                    for ks in range(len(value_dict['kontext'])):
                        if value_dict['kontext'][ks]['snippet'].size > max_snip_size:
                            kontext_snippet = value_dict['kontext'][ks]
                            max_snip_size = kontext_snippet['snippet'].size

#                    kontext_snippet = value_dict['kontext'][0]
                    fname_lines = os.path.join(args.output_folder,
                                               os.path.basename(filename))
                    fname_lines = os.path.splitext(fname_lines)[0] + '_lines.png'
                    num_lines, mean_height, std_height, line_img =\
                            lineHeights(kontext_snippet['snippet'],
                                        fname_lines,
                                        show=args.show_lines,
                                        write=args.write_lines,
                                        return_img = True if\
                                        args.draw_rota_height else False)
                    
                    if mean_height > 0:
                        stats['kontext'] = [ ['lines', 'abs', num_lines], 
                                            ['mean line height', 'abs',
                                             mean_height],
                                            ['standard deviation (rel)', 'rel',
                                             std_height],
                                            ['width mean height (rel)', 'rel', 
                                             float(kontext_snippet['snippet'].shape[1]) /
                                             mean_height ]]

#                value_list = ['kontext', 'erste zeile', 'eschatokoll', 'benevalete',
#                              'rota', 'komma', 'wort', 'datumzeile', 'other']
                # compute stats
                for value, snippet in value_dict.iteritems():
                    height = 0.0
                    width = 0.0
                    width_height_ratio = 0.0
                    pos_x = 0.0
                    pos_y = 0.0
                    black_white_ratio = 0.0

                    all_stats = []
                    for snip in snippet:
                        polygon = polygonFromString(snip['poly'])
                        mask = maskFromPolygon(polygon, snip['bounding'])
                        # adjust bbox, snippet, and mask if available
                        if args.adjust_bbox:       
                            snip['bounding'], snip['snippet'], mask = adjustBbox( snip['snippet'], 
                                                                                 snip['bounding'], 
                                                                                 mask)

                        width += float(snip['bounding'][2])
                        height += float(snip['bounding'][3])
                        
                        # no idea how that can happen:
                        if snip['snippet'] is None:
                            continue
                        all_stats.append( computeStats(snip['snippet'], mask) )
                  
                    # average the statistics for multiple snippets e.g. for all
                    # lines / words / graphs
                    if len(all_stats) > 0 and all_stats[0] is not None:
                        avg_stats = all_stats[0]
                        cnt_st = 1
                        for i in xrange(1,len(all_stats)):
                            if all_stats[i] is None:
                                continue
                            cnt_st += 1
                            for k in avg_stats.keys():
                                avg_stats[k] += all_stats[i][k]
                          
                        for k in avg_stats.keys():
                            avg_stats[k] /= float(cnt_st)
                       
                        if avg_stats != None:
                            for k,v in avg_stats.iteritems():
                                stats.setdefault(value, []).append( [k, 'rel', v])

                    # average them 
                    width /= len(snippet)
                    height /= len(snippet)

                    stats.setdefault(value, []).append( ['height', 'abs', height])
                    if 'kontext' in value_dict and mean_height > 0:
                        stats[value].append( ['height lineheight ratio', 'rel',
                                                height / mean_height ] )

                    """
                    if 'kontext' in value_dict and value != 'kontext':
                        stats[value].append( ['distance to left kontext edge',
                                                  'abs',
                                                  pos_x - kontext_snippet['bounding'][0]] )
                        # distance to right kontext edge                                    
                        dist = float((kontext_snippet['bounding'][0]
                                  + kontext_snippet['bounding'][2])
                                 - (pos_x + width))
                        stats[value].append( ['distance to right kontext edge ',
                                                   'abs',
                                                   dist] )
                    """
                    # if it is a real average, let us know
                    if len(snippet) > 1:
                        for stat in stats[value]:
                            #print value, stat[0]
                            stat[0] += ' (avg)'

                if 'pergamentflaeche' in value_dict:
                    pergament_snippet = value_dict['pergamentflaeche'][0]
                elif 'pergamentfläche' in value_dict:
                    pergament_snippet = value_dict['pergamentfläche'][0]

                if 'rota' in value_dict:
                    rota_snippet = value_dict['rota'][0]
                    if 'kontext' in value_dict:                                                                          
                        # distance to the left edge of kontext
                        stats['rota'].append( [ 'distance to left kontext edge ',
                                                       'abs',
                                                       rota_snippet['bounding'][0]
                                                             - kontext_snippet['bounding'][0]
                                              ] )
                        # FIXME: has to write in orig-img not in line-img
                        if args.draw_rota_height:
                            rota_snip = rota_snippet['snippet']
                            fname_rota_lines = fname_lines[:-4] +\
                                 '_rota_lines.png'
                            writeLineRotaImage(line_img, rota_snip, mean_height,
                                               fname_rota_lines)

                            
                    if 'pergamentflaeche' in value_dict:
                        # distance to the left edge of kontext
                        dist_rota_perg = rota_snippet['bounding'][0] -\
                                        pergament_snippet['bounding'][0]

                        stats['rota'].append( [ 'distance to left'
                                               ' pergamentflaeche edge',
                                                       'abs', dist_rota_perg])
                                            
                                                                           
                if 'benevalete' in value_dict: 
                     # benevalete width-height ratio 
                    bene_snippet = value_dict['benevalete'][0]
                    if 'kontext' in value_dict:                                                                          
                        # distance to the right kontext edge
                        dist = float((kontext_snippet['bounding'][0]
                                      + kontext_snippet['bounding'][2])
                                     - (bene_snippet['bounding'][0]
                                        + bene_snippet['bounding'][2]))
                        stats['benevalete'].append( ['distance to right kontext edge ',
                                                       'abs',
                                                       dist] )

                if 'komma' in value_dict:
                    komma_snippet = value_dict['komma'][0]
                    if 'kontext' in value_dict:                                                                          
                        # distance to the right kontext edge
                        dist = float((kontext_snippet['bounding'][0]
                                      + kontext_snippet['bounding'][2])
                                     - (komma_snippet['bounding'][0]
                                        + komma_snippet['bounding'][2]))
                        stats['komma'].append( [ 'distance to right kontext edge',
                                                        'abs',
                                                        dist] )

                # distance to kontext and datum line
                symbol_kontext_dist = np.finfo(float).max
                symbol_datum_dist = np.finfo(float).max
                for symbol in ['rota', 'benevalete', 'komma']:
                    if symbol in value_dict:
                        bbox = value_dict[symbol][0]['bounding']
                        midpoint = [ bbox[0] + bbox[2]/2, bbox[1] + bbox[3]/2 ]
                        if 'kontext' in value_dict:
                            kontext_dist = pointSnippetDistance( midpoint , kontext_snippet)
                            if kontext_dist < symbol_kontext_dist:
                                symbol_kontext_dist = kontext_dist

                        if 'datumzeile' in value_dict:
                            datum_dist = pointSnippetDistance( midpoint, value_dict['datumzeile'][0] )
                            if datum_dist < symbol_datum_dist:
                                symbol_datum_dist = datum_dist
                        
                        # DEBUG
                        # save coordinates
                        fname = os.path.join(args.output_folder, symbol) +'.dat'
                        with open(fname, 'a') as f:
                            f.write('{} 1 {} {} {} {}\n'.format(filename,
                                                              bbox[0], bbox[1],
                                                              bbox[2], bbox[3]))                        
                    else:
                        fname = os.path.join(args.output_folder, symbol) +'_not.txt'
                        with open(fname, 'a') as f:
                            f.write('{}\n'.format(filename))


                if symbol_kontext_dist != np.finfo(float).max:
                    stats.setdefault('other',[]).append( 
                                                    ['minimum symbol distance (midpoint) to kontext ',
                                                     'abs',
                                                     symbol_kontext_dist] )

                if symbol_datum_dist != np.finfo(float).max:
                    stats.setdefault('other', []).append( 
                                                    ['minimum symbol distance (midpoint) to datumzeile ',
                                                     'abs',
                                                    symbol_datum_dist] )

                # distance between benevalete and rota
                if 'rota' in value_dict and 'benevalete' in value_dict:
                    dist_bene_rota =  bene_snippet['bounding'][0]\
                                       - (rota_snippet['bounding'][0] \
                                       +\
                                          rota_snippet['bounding'][2])
                    stats.setdefault('other',[]).append( ['distance rota - benevalete ',
                                                       'abs', dist_bene_rota])

                    if 'pergamentflaeche' in value_dict:
                        dist_rota_kon_ratio = float(dist_bene_rota) / float(dist_rota_perg)
                        stats.setdefault('other', []).append( ['dist rota_bene'
                                                               ' dist per_rota'
                                                               ' ratio',
                                                             'rel',
                                                             dist_rota_kon_ratio])
                # distance between benevalete and komma 
                if 'komma' in value_dict and 'benevalete' in value_dict:
                    stats.setdefault('other',[]).append(
                                                    ['distance benevalete -komma',
                                                     'abs',
                                                     komma_snippet['bounding'][0]
                                                        - (bene_snippet['bounding'][0] 
                                                        +
                                                           bene_snippet['bounding'][2])] )



                # Statistics, convert absolute values to relative ones
                unit = 1.0
                relative_to_text = ''
                if args.relative_to:
                    rel_name, rel_unit = args.relative_to.split('@')
                    rel_unit = float(rel_unit)
                    try:
                        unit = rel_unit / float(stats[rel_name][0][2])
                        relative_to_text = '(relative to: {})'.format(rel_name) 
                    except KeyError:
                        print 'relative_to field', rel_name, 'doesnt exist as statistic, possible are:', stats.keys()
                        print 'ignore relative_to assignment, values are now in pixels'
                        unit = 1.0
                        relative_to_text = ''

                statstr = ''
                for val, statistics in stats.iteritems():
                    val = val.encode('utf-8')
                    statstr += '\t- {}\n'.format(val)
                    for stat in statistics:
                        if stat[1] == 'abs':
                            statstr += '\t\t{}{}: {}\n'.format(stat[0], 
                                                               relative_to_text,
                                                               float(stat[2]) * unit)
                        else:
                            statstr += '\t\t{}: {}\n'.format(stat[0], stat[2]) 

                            # print indivudal stats-files
                            raw_fname = val + '_' + stat[0].replace(' ',
                                                              '_').translate(None, '()') +\
                                        '_stats.txt'                            
                            with open(os.path.join(args.output_folder, raw_fname), 'a') as f:
                                f.write(os.path.basename(filename) + ',')
                                f.write('{}\n'.format(stat[2]))

                # print statistics:
                print '\t--STATS--'
                print statstr
                

                with open(os.path.join(args.output_folder, 'statistics.txt'), 'a') as f:
                    f.write(os.path.basename(filename) + '\n')
                    f.write(statstr)
