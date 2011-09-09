#!/usr/bin/env python
# :noTabs=true:
# (c) Copyright Rosetta Commons Member Institutions.
# (c) This file is part of the Rosetta software suite and is made available under license.
# (c) The Rosetta software is developed by the contributing members of the Rosetta Commons.
# (c) For more information, see http://www.rosettacommons.org. Questions about this can be
# (c) addressed to University of Washington UW TechTransfer, email: license@u.washington.edu.

## @file   features.py
## @brief  A script to help debug/run Features scientic tests
## @author Sergey Lyskov

import sys, commands, subprocess, re

from os import path, environ
from optparse import OptionParser, IndentedHelpFormatter


def main(argv):
    '''
A script to help debug/run Features scientic tests
Please note that this script is for debuging/testing purposes only, it is currently not used in ScientificProfile daemon.
    '''
    parser = OptionParser(usage="usage: %prog [OPTIONS] [TESTS]")
    parser.set_description(main.__doc__)
    parser.add_option("-d", "--database",
      default="", # processed below
      help="Path to Rosetta database. (default: $ROSETTA3_DB, ~/rosetta_database)",
    )

    parser.add_option("-m", "--mini_home",
      #default=path.join( path.expanduser("~"), "mini"),
      default= path.join( path.dirname( path.dirname( path.dirname(path.abspath(sys.argv[0])) ) ), 'rosetta_source'),
      help="Directory where rosetta_source is found (default: ../../rosetta_source/)",

    )

    parser.add_option("--mode",
      default="release",
      help="In selecting binaries, which mode was used? (default: release)",
    )

    parser.add_option("-c", "--compiler",
      default="gcc",
      help="In selecting binaries, which compiler was used? (default: gcc)",
    )

    parser.add_option("--extras",
      default="default",
      dest="extras",
      help="in selecting binaries, which options were specified? (default: default)",
    )

    parser.add_option("--lsf-queue_name",
      default="128cpu", action="store",
      help="The name of the queue when submitting jobs on the lsf cluster, to see available queues run 'bqueues' (use with 'submit_lsf' action). [default %default]"
    )

    parser.add_option("--num-cores",
      default=128, action="store",
      help="How many cores to request when submitting jobs. [default %default]"
    )

    parser.add_option("--output-dir",
      default="output", action="store",
      help="Base directory of where to deposit data and results. [default %default]"
    )

    parser.add_option("--run-type",
      default="condor", action="store",
      help="Indicate which type of the action should be run. Eg, 'lsf' means execute benchmark on a Load Sharing Facility cluster, while 'dryrun' means simulate the output of running the benchmark for debugging purposes. [default %default]"
    )

    (options, args) = parser.parse_args(args=argv)

    if options.database == parser.get_default_values().database:
        if environ.get('ROSETTA3_DB') is not None and \
                path.isdir(environ.get('ROSETTA3_DB')):
            options.database = environ.get('ROSETTA3_DB')
        elif path.isdir( path.join( path.expanduser("~"), "rosetta_database") ):
            options.database = path.join( path.expanduser("~"), "rosetta_database")
        else:
            print "Can't find database at %s; please set $ROSETTA3_DB or use -d" % options.database
            return 1
    # Normalize path before we change directories!
    options.database = path.abspath(options.database)


    if len(args) != 1:
        print 'You must supplies action and test name in command line! For example: "./feature.py submit " or ""./deature.py analyze ""'
        return

    valid_actions = ["submit", "analyze"]

    action = args[0]

    if action not in valid_actions:
        print "ERROR: Action must be one of ['%s']" % "', '".join(valid_actions)
        return 1

    print 'Perform %s...' % action

    workdir = path.abspath( '.' )
    minidir = options.mini_home

    #print 'minidir:', minidir
    #return

    database = options.database
    print 'Database:', database
    bin = path.join(minidir, "bin")
    pyapps = path.join(minidir, "src", "python", "apps")
    if sys.platform.startswith("linux"): platform = "linux" # can be linux1, linux2, etc
    elif sys.platform == "darwin": platform = "macos"
    else: platform = "_unknown_"
    compiler = options.compiler
    mode = options.mode
    binext = platform+compiler+mode
    lsf_queue_name = options.lsf_queue_name
    num_cores = options.num_cores
    output_dir = options.output_dir
    run_type = options.run_type
    # Read the command from the file "command"

    try:
        ''' p = subprocess.Popen(
            ['svn', 'info'],
            stdout=subprocess.PIPE,
            cwd=path.join("..","..",".."))
        p.wait()
        svn_info = p.communicate()[0]'''

        svn_info = commands.getoutput('svn info %s' % minidir)  # ../../../rosetta_source')
        svn_url = re.search("URL: (\S*)", svn_info).group(1)
        svn_revision = int(re.search("Revision: (\d*)", svn_info).group(1))
    except:
        print "WARNING: Unable to get svn info for path: %s" % minidir
        svn_url="UNKNOWN"
        svn_revision=0

    cmd = file(path.join(workdir, action)).read().strip()
    # cmd = cmd % vars() # variable substitution using Python printf style
    mvars = dict(minidir=minidir, database=database, workdir=workdir, platform=platform, bin=bin, compiler=compiler, mode=mode, binext=binext, svn_url=svn_url, svn_revision=svn_revision, lsf_queue_name=lsf_queue_name, num_cores=num_cores, output_dir=output_dir, run_type=run_type)
    cmd = cmd % mvars

    # Writing result to .sh file for future reference.
    f = file(path.join(workdir, action)+'.sh', 'w');  f.write(cmd);  f.close()

    # Creating python style file with arguments in case test want to use Python as script language
    f = file(path.join(workdir, '_arguments.py'), 'w');  f.write( str(mvars) );  f.close()

    # Executing created bash script...
    print commands.getoutput('sh %s.sh' % action )



if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
