"""
author: Vincent Christlein
lincense: GPL v.2.0, see LICENSE

description:
this tool allows to create dendrograms of global descriptors
"""
import argparse
import matplotlib.pyplot as plt
import cPickle
import numpy as np
import gzip
from ast import literal_eval
import glob
from scipy import cluster
#import progressbar
import os

def loadDescriptors(files, reshape=False, max_descs=0, 
                    max_descs_per_file=0,
                    rand=True, ax=0, rm_zero_rows=False,
                    min_descs_per_file=0, 
                    show_progress=True, 
                    maskfiles=None, return_index_list=False):
    """load descriptors , pkl.gz or siftgeo or csv format
    """

    if len(files) == 0:
        print 'WARNING: laodDescriptor() called with no files'
        return
    if isinstance(files, basestring):
        files = [files]
    if maskfiles is not None and isinstance(maskfiles, basestring):
        maskfiles = [maskfiles]

    index_list = [0]
    descriptors = []
    desc_length = 0
#    if len(files) > 10 and show_progress:
#        widgets = [progressbar.Percentage(), ' ', progressbar.Bar(), ' ',
#               progressbar.ETA()]
#        progress = progressbar.ProgressBar(widgets=widgets)
#    else:
    def progress(x):
        return x

    for i in progress(range(len(files))):
        f = files[i]
        try:
            if f.endswith('pkl.gz'):
                with gzip.open(f, 'rb') as ff:
                    desc = cPickle.load(ff)               
            else:
                desc = np.loadtxt(f, delimiter=',',ndmin=2, dtype=np.float32)
            
            if desc.dtype != np.float32 and desc.dtype != np.float64:
                print ('WARNING: desc.dtype ({}) != np.float32 and !='
                       ' np.float64 ->'
                       ' convert it'.format(desc.dtype))
                desc = desc.astype(np.float32)
        except ValueError:
            print 'Error at file', f
            raise

        assert(np.isfinite(desc).all())

        # apply mask if available
        if maskfiles != None and maskfiles[i] != None:
            mask = cv2.imread(maskfiles[i], cv2.CV_LOAD_IMAGE_GRAYSCALE)
            if mask == None:
                print 'WARNING couldnt read maskfile {}'.format(maskfiles[i])            
            else:
                mask = mask.ravel()
                #print mask.shape, desc.shape, np.count_nonzero(mask)
                desc = desc[ np.where(mask != 0) ]
       
        if len(desc) == 0:
            print 'no descriptors of file {}?'.format(f)
            if maskfiles != None and maskfiles[i] != None:
                print ' and its maskfile {}'.format(maskfiles[i])
                print ('it has {} 0-entries '
                        'and {} !=0'.format(len(mask[mask==0]),
                                           len(mask[mask!=0])) )
            continue
        
        # remove zero-rows
        if rm_zero_rows:
            desc = desc[~np.all(desc == 0, axis=1)]
        # skip if too few descriptors per file
        if min_descs_per_file > 0 and len(desc) <= min_descs_per_file:
            continue
        # reshape the descriptor
        if reshape:
            desc = desc.reshape(1,-1)
        # pick max_descs_per_file, either random or the first ones
        if max_descs_per_file != 0:
            if rand:
                #if len(desc) < max_descs_per_file:
                #    print "Oh Noes, error for file", f
                #    print "pixels in file", desc.shape
                #    print "maskpx in file", mask.shape
                desc = desc[ np.random.choice(len(desc), 
                                              min(len(desc),
                                              max_descs_per_file))]
                # this is probably slower
#                desc = np.array( random.sample(desc, min( len(desc), max_descs_per_file) ) )                
            else:
                desc = desc[:max_descs_per_file]
        
        descriptors.append(desc)

        desc_length += len(desc)
        if max_descs != 0 and desc_length > max_descs:
            break

        if return_index_list:
            index_list.append(desc_length)
            
    
    if len(descriptors) == 0:
        if min_descs_per_file == 0:
            print 'couldnt load ', ' '.join(files)
        return None 

    descriptors = np.concatenate(descriptors, axis=ax)

    if return_index_list:
        return descriptors, index_list

    return descriptors

def getFiles(folder, pattern, labelfile=None, exact=True, concat=False,
             ret_label=None):
    
    if not os.path.exists(folder):
        raise ValueError('file or folder {} doesnt exist'.format(folder))
    
    if ret_label != None:
        print ('WARNING: getFiles(): option `ret_label` is deprecated, if'
               ' ret_label==False everything might get wrong (2 instead of 1'
               ' argument')

    if not os.path.isdir(folder):
        return  [ folder ], None

    if labelfile:
        labels = []
        with open(labelfile, 'r') as f:
            all_lines = f.readlines()
        all_files = []
        check = True
        for line in all_lines:
            try:
                img_name, class_id = line.split()
            except ValueError:
                if check:
                    print ('WARNING: labelfile apparently doesnt contain a label, '
                          ' you can ignore this warning if you dont use the'
                          ' label, e.g. if you just want the'
                          ' files to be read in a certain order')
                    check = False
                else:
                    img_name = line, class_id = None
            except:
                raise

            if exact:
                file_name = os.path.join(folder, os.path.splitext(img_name)[0] + pattern )
                all_files.append(file_name)
            else:
                search_pattern = os.path.join(folder, 
                                              os.path.splitext(img_name)[0] 
                                              + '*' + pattern)
                files = glob.glob(search_pattern)
                if concat:
                    all_files.append(files)
                else:
                    all_files.extend(files)
            labels.append(class_id)                

        return all_files, labels

    return glob.glob(os.path.join(folder, '*' + pattern)), None

def mkdir_p(path):
    try:
        os.makedirs(path)
    except OSError as exc:
        import errno
        if exc.errno == errno.EEXIST:
            pass
        else: 
            raise
    else:
        print 'created', path

def parserArguments(parser):
    io_group = parser.add_argument_group('input output options')
    io_group.add_argument('-l', '--labelfile',\
                        help='label-file containing the images / features to load + labels')
    io_group.add_argument('-i', '--inputfolder',\
                        help='the input folder of the images / features')
    io_group.add_argument('-s', '--suffix', default='',\
                        help='only choose those images with a specific suffix')
    io_group.add_argument('--exact', type=literal_eval, default=True, 
                        help='between (stripped) label and suffix nothing is'
                          ' allowed, if set to false anything can be there')
    io_group.add_argument('-o', '--outputfolder', default='.',\
                        help='the output folder for the descriptors')
    parser.add_argument('--dist_matrix', 
                        help='distance-matrix')
    parser.add_argument('--cluster', choices=['ward','single','centroid',
                                              'kmeans'],
                        help='cluster the results and print cluster results and'
                        ' dendogram')
    parser.add_argument('--label_angle', type=float, default=90.0,
                        help='rotate labels in dendrogram about this angle')
    return parser

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description="Dendrogram")
    parser = parserArguments(parser)
    args = parser.parse_args()

    if args.outputfolder and not os.path.exists(args.outputfolder):
        mkdir_p(args.outputfolder)

    files, labels = getFiles(args.inputfolder, args.suffix, 
                             labelfile=args.labelfile, exact=True)
    descriptors = loadDescriptors(files)

    assert(args.cluster != None)
    
    if args.cluster == 'kmeans':
        print 'TODO'
    else:
        z = cluster.hierarchy.linkage(descriptors, args.cluster)

        fig = plt.figure(figsize=(12,8))
        ax = fig.add_subplot(111)
        den = cluster.hierarchy.dendrogram(z)
        labels = np.array(labels)
        label_ind = np.array(den['ivl']).astype(int)
        xlabelsL = ax.set_xticklabels(labels[label_ind])
        # rotate labels 90 degrees
        for label in xlabelsL:
            label.set_rotation(args.label_angle)
        fig.savefig('dendrogram_{}.pdf'.format(args.cluster))

