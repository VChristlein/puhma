"""
author: Vincent Christlein
lincense: GPL v.2.0, see LICENSE

description:
    this tool plots stats which were created by the layout_stats.py - tool
    however it might also work with other stats as long as they are in
    the same format
"""
import argparse
import matplotlib.pyplot as plt
import numpy as np
import re
import os
import itertools


def parseArguments(parser):
    parser.add_argument('stats_file', 
                        help='stats w. the format filename,<stat>')
    parser.add_argument('-r','--resolution',
                        type=int,
                        default=10000,
                        help='resolution in days!')
    parser.add_argument('-s', '--select', 
                        help='select filenames listed in this file')
    parser.add_argument('-f', '--field', default=[], nargs='*', 
                        help='if the selection-file has another column with'
                        ' values, they can be selected here')
    parser.add_argument('--rmoutliers', type=float,
                        help='remove all stats which are <> mean -+ rmoutliers*std')
    parser.add_argument('--img_labels',
                        help='additional label from file to a group (e.g.'
                        ' a pontificate), each line contains the filename and'
                        ' its label')
    parser.add_argument('--hline', default=[], nargs='*', type=float,
                        help='add additional dotted horizontal line(s) at this'
                        ' y-value')
    parser.add_argument('--yearstep', default=5, type=int,
                        help='show each yearstep year four the label at the x-axis')
    return parser

def selectFiles(s_file):
    with open(args.select, 'r') as f:
        lines = f.readlines()

    sel_files = []
    for l in lines:
        s = l.split()
        if len(s) == 1\
           or (len(s) > 1 and s[1] in args.field):                
            # add to selected files but remove path and extension
            s_p = os.path.basename(s[0]).rsplit('.',1)[0]
            sel_files.append(s_p)
    print 'keep {} from {} file-statistics ({} %)'.format(len(sel_files),
                                                          len(lines),
                                                          len(sel_files) /
                                                          float(len(lines)))
    return sel_files

if __name__ == '__main__':
    parser = argparse.ArgumentParser('plot stats files of years')
    parser = parseArguments(parser)
    args = parser.parse_args()

    res = args.resolution
    if args.select:
        sel_files = selectFiles(args.select)

    # year -> [ stat1, stat2, ... ]
    year_stats = {}
    all_stats = []
    all_filenames = []
    all_years = []
    fname_stats = {}
    with open(args.stats_file, 'r') as f:
        for line in f:
            if line.count(',') > 1:
                print 'WARNING, wrong filename-format:', line.rsplit(',',1)[0] 
                continue
            fname,stat = line.split(',')
            fname_parts = re.split('[(_ )]+', fname)
            if len(fname_parts[1]) != 8 or\
               fname_parts[1].startswith('9') or\
               fname_parts[1].count('9') > 4:
                print 'WARNING, wrong datum-format of', fname
                continue
            if args.select and fname.split('.',1)[0] not in sel_files:
                continue
            year = int(fname_parts[1])
            all_years.append(year)
            stat = float(stat)
            all_stats.append(stat)
            all_filenames.append(fname)
            year_stats.setdefault(year, []).append(stat)
            fname_stats[fname] = (year,stat)
    print 'plot stats of {} files'.format(len(all_filenames))

    mean = np.mean(all_stats)
    std = np.std(all_stats)    
    print('stat {}: mean: {}, std: {}'.format(len(all_stats),
                                                      mean, std))

    if args.rmoutliers:
        all_stats_new = []
        outlier_fnames = []
        bound_std = args.rmoutliers * std
        for e,st in enumerate(all_stats):
            if st >= mean - bound_std and st <= mean + bound_std:
                all_stats_new.append(st)
            else:
                outlier_fnames.append(all_filenames[e])

        print 'outliers: ', outlier_fnames
        # clean            
        for y,s in year_stats.iteritems():
            for ss in s:
                if ss < mean - bound_std or ss > mean + bound_std:
                    year_stats[y].remove(ss)
                    
        mean_new = np.mean(all_stats_new)
        std_new = np.std(all_stats_new)
        print('stat {}: mean new: {}, std_new: {}'.format(len(all_stats_new),
                                                          mean_new, std_new))

    # let's deal with grouping
    years = sorted(year_stats.keys())
    start_year = years[0]
    end_year = years[len(years)-1]
    print 'year-range: {} - {}'.format(start_year, end_year)

    year_groups = {}
    curr_year = start_year
    curr_year_group = []
    for y in years:
        if y >= curr_year + res:
           year_groups[curr_year] = curr_year_group
           curr_year_group = []
           curr_year += res
        curr_year_group.extend(year_stats[y])

    print 'resolution:', res
    # compute mean and stddev
    means = []
    stds = []
    for y, v in sorted(year_groups.items()):
        means.append( np.mean(v) )
        stds.append( np.std(v) )

    if res == 0:
        x = all_years
        y = all_stats
   
        # this is only for the rota test:
        """
        x2 = []
        y2 = []
        x3 = []
        y3 = []
        for i,s in enumerate(all_stats):
            if s < 3.3 and s > 2.8:
                y2.append(s)
                x2.append(all_years[i])

            if s < 4.3 and s > 3.8:
                y3.append(s)
                x3.append(all_years[i])
        print ('have {} near ratio 3 and {} near a '
               'ratio 4 from a total of {}').format(len(y2),
                                                    len(y3),
                                                    len(y))
        """
    else:
        x = sorted(year_groups.keys())

    print x
    
    # xticks labels
    min_year = min(all_years) / 10000
    max_year = max(all_years) / 10000
    x_t = np.arange(min_year, max_year, args.yearstep) * 10000
    labels = [ str(b)[:4] for b in x_t ]
    
    colors = itertools.cycle(['r', 'g', 'b', 'y', 'c', 'm', 'k', 'orange'])
    markers = itertools.cycle(['o', 'v', '^', 's', '*'])
    
    fig = plt.figure()
    ax = fig.add_axes([0.1, 0.1, 0.7, 0.85])
    ax.margins(0.02)
    plt.xticks(x_t, labels, rotation='vertical')

    if args.img_labels:
        with open(args.img_labels, 'r') as f:
            img_group_lines = f.readlines()
        img_groups = {}
        for line in img_group_lines:
            fname, group = line.split()
            img_groups[fname] = group

        group_stats = {}
        for f, year_stat in fname_stats.iteritems():
            # TODO change to basename and strip extension
            group_stats.setdefault(img_groups[f], []).append(year_stat)
        for gr in group_stats:
            x_gr = []
            y_gr = []
            for st in group_stats[gr]:
                x_gr.append(st[0])
                y_gr.append(st[1])
            ax.plot(x_gr, y_gr, linestyle='None', marker=next(markers),
                     color=next(colors), label=gr.upper().replace('_',' '))
        ax.legend(bbox_to_anchor=(1.05, 1), loc=2, borderaxespad=0., numpoints=1)

    # Pad margins so that markers don't get clipped by the axes
    else:
        if res == 0:
            ax.plot(x, y, linestyle='None', marker='^')
            # this is only for our rota-visualization
#            ax.plot(x2, y2, linestyle='None', marker='^', color='red')
#            ax.plot(x3, y3, linestyle='None', marker='^', color='orange')
        else:
            ax.errorbar(x, means, stds, linestyle='None', marker='^')

    for ah in args.hline:
        ax.axhline(ah, color='black', linestyle='dotted')

    plt.show()
