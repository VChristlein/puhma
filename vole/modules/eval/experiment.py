#!/usr/bin/python

"""
Utility script to run multiple experiments, possibly with different configurations
with multiple processes on multiple hosts.

This script takes no command line arguments - it reads the setup.py file in
the current directory and then performs the experiments specified in this file.

To create a setup.py file use the template at the end of this script. Scrolling
down you will also find all values that you can set and all utility functions
that you might want to use in the setup.py file.
"""

import sys, os, errno, collections, multiprocessing
from signal import signal, SIGTERM
from math import ceil
from optparse import OptionParser
from datetime import datetime
from subprocess import Popen, PIPE, STDOUT
from multiprocessing.pool import Pool, ThreadPool
from threading import Lock

sys.path.insert(0, '.')
import setup

lock = Lock()

class Experiments(object):
    def run(self):
        tasks = get_tasks()
        print 'experimenting with %s configuration(s) and %s input(s) (= %s task(s))' % (len(setup.configs), len(setup.inputs), len(tasks))

        setup.pre()

        # distribute tasks between specified hosts
        processes = []
        frm = 0
        for host in setup.hosts:
            if frm == len(tasks): # all tasks distributed
                break
            chunk_len = int(ceil(1.0 / len(setup.hosts) * len(tasks)))
            to = min(frm + chunk_len, len(tasks))
            cmd = 'cd "{0}"; experiment.py --host={1},{2}'.format(os.getcwd(),frm,to)
            args = ['ssh', host, cmd]
            processes.append(Popen(args, stdin=PIPE))
            frm = to

        try:
            for p in processes:
                p.communicate()
        except KeyboardInterrupt:
            print 'killing all running tasks...'
            for p in processes:
                try:
                    p.terminate()
                except EnvironmentError: # most likely already dead, ignore errors
                    pass
            for host in setup.hosts:
                # this is a brutal soluation :/ - but at least it is a solution...
                cmd = 'pid=$(pgrep -f "^/usr/bin/python.*experiment.py --host"); kill -TERM $pid $(pgrep -P $pid)'
                Popen(['ssh', host, cmd]).communicate()

        setup.post()

class Experiment(object):
    def __init__(self, range):
        self.range = range

    def run(self):
        # line buffered, because we are called via ssh
        sys.stdout = os.fdopen(sys.stdout.fileno(), 'w', 1)

        if not all(k in setup.__dict__ for k in ('inputs', 'command')):
            print 'setup: missing values (inputs or command)'
            return

        mkdir(setup.results_path)
        for input in setup.inputs:
            mkdir(os.path.join(setup.results_path, input_str(input)))

        process_count = setup.process_count
        if process_count is None:
            if setup.matlab:
                # MATLAB does its own multiprocessing
                process_count = 1
            else:
                process_count = multiprocessing.cpu_count()

        tasks = get_tasks()[self.range[0]:self.range[1]]
        notify('{0} task(s) with {1} process(es)'.format(len(tasks), process_count))
        #print 'tasks:', tasks

        pool = ThreadPool(process_count)
        try:
            pool.map(run_task, tasks)
        except KeyboardInterrupt:
            print "INT MAIN"
            pass
            # with ThreadPool: only main thread gets interrupted, subprocesses stay alive :(
            # with Pool: only subprocesses get interrupted, main process gets not interrupted until all tasks are done :(
            # with both: all other signals are ignored completly :(

class Task(object):
    def __init__(self, input, config={}):
        self.input  = input
        self.config = config

        self.name = '{0}#{1}'.format(input_str(input), config_str(config))
        self.result_path = os.path.join(setup.results_path, input_str(input), 'c' + config_str(config))

    def get_args(self):
        args = list(setup.static_args)
        args += setup.get_args(self.input, self.config)
        args.append((setup.result_path_name, self.result_path))
        return args

    def get_pargs(self):
        args, opts = split_args_opts(self.get_args())
        if setup.matlab:
            from itertools import chain
            args = ','.join("'{0}'".format(a) for a in args)
            opts = ','.join("'{0}'".format(o) for o in chain.from_iterable(opts))
            return [setup.command, '-nodesktop', '-nosplash', '-r', "{0}({1},struct({2}))".format(setup.matlab_function, args, opts)]
        else:
            opts = ['--{0}={1}'.format(k, v) for k, v in opts]
            return [setup.command] + args + opts
        
    def __str__(self):
        return '<{0} {1}>'.format(type(self).__name__, self.name)
    __repr__ = __str__

def error_info(f):
    # neat trick to print a traceback on error
    def wrapper(task):
        try:
            return f(task)
        except:
            from traceback import print_exc
            print_exc()
            raise
    return wrapper

@error_info
def run_task(task):
    args = task.get_pargs()
    commandline = ' '.join(args)

    notify('[...] ' + task.name)

    mkdir(task.result_path)
    f = open(os.path.join(task.result_path, 'experiment.txt'), 'w')
    f.writelines([datetime.now().ctime() + '\n', commandline + '\n'])
    if setup.silent:
        f.writelines(['\n', '==== stdout + stderr ====\n'])
        stdout = f
    else:
        stdout = None
    f.flush()

    setup.pre_task(task)

    env = dict(os.environ)
    env.update(setup.env)
    p = Popen(args, env=env, stdin=PIPE, stdout=stdout, stderr=STDOUT)
    try:
        p.communicate()
    except KeyboardInterrupt:
        print 'INT TASK'
        p.terminate()
        return
    finally:
        f.writelines(['\n', datetime.now().ctime() + '\n'])
        f.close()

    if p.returncode == 0:
        setup.post_task(task)
        notify('[ x ] ' + task.name)
    else:
        notify('[err] ' + task.name)

def get_tasks():
    tasks = []
    for input in setup.inputs:
        tasks += get_tasks_for_input(input)
    return tasks

def get_tasks_for_input(input):
    return [Task(input, c) for c in setup.configs]

def get_tasks_for_config(config):
    return [Task(i, config) for i in setup.inputs]

def notify(msg):
    from socket import gethostname
    lock.acquire()
    print '{0} {1}: {2}'.format(gethostname(), datetime.now().ctime(), msg)
    lock.release()

def split_args_opts(task):
    args = []
    opts = []
    for arg in task:
        if isinstance(arg, tuple):
            opts.append(arg)
        else:
            args.append(arg)
    return args, opts

def mkdir(path):
    """
    mkdir that handles already existing directories.
    """
    try:
        os.mkdir(path)
    except OSError, e:
        if e.errno == errno.EEXIST:
            pass
        else:
            raise

def config_str(config):
    return ','.join('{0}={1}'.format(k, v) for k, v in sorted(config.items()))

def input_str(input):
    if isinstance(input, tuple):
        input = '_'.join(input)
    return input.replace('/', '_').replace('\\', '_')

if __name__ == '__main__':
    parser = OptionParser()
    parser.add_option('--host')
    opts, args = parser.parse_args()
    if opts.host:
        range = tuple(int(t) for t in opts.host.split(','))
        Experiment(range).run()
    else:
        Experiments().run()

###############################
# default values for setup.py #
###############################

results_path     = 'exres'
input_path       = os.curdir
env              = {}
matlab           = False
matlab_function  = None
result_path_name = 'result_path'
static_args      = []
silent           = True
# None = auto (number of CPUs, 1 for MATLAB (which has its own multiprocessing support))
process_count    = None
hosts            = ['localhost']
configs          = [{}]

def get_args(input, config):
    """
    Assembles a list of arguments from input and config. An argument can be 
    either a single value or a tuple, in which case it is a named option. For
    example ['foo.txt', ('bar', '2')] is the same as foo.txt --bar=2 in terms
    of the command line.
    """
    return [os.path.join(setup.input_path, input)] + config.items()

def pre(): pass
def post(): pass
def pre_task(task): pass
def post_task(task): pass

################################
# utility methods for setup.py #
################################

def frange(offset, step, count):
    return (x * step + offset for x in xrange(count))

def configs_from_values(name, values):
    """
    configs = configs_from_values('x', ('1', '2'))
    =>
    configs = [{'x': '1'}, {'x': '2'}]
    """
    return list({name: v} for v in values)

def configs_cross(choices):
    configs = [{}]
    for name, values in choices.items():
        al = configs
        bl = configs_from_values(name, values)
        configs = []
        for a in al:
            for b in list(bl):
                configs.append(dict(a.items() + b.items()))
    return configs

#########################
# template for setup.py #
#########################

"""COPY BELOW

from experiment import *

command = 'example'

input_path = '/path/to/inputs'

inputs = ['input.png']

configs = configs_cross({
    'x': ('1', '2'),
    'y': ('a', 'b')
})

COPY ABOVE"""
