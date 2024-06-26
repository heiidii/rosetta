# -*- mode:python;indent-tabs-mode:nil;show-trailing-whitespace:t; -*-
#
# Doxygen SCons tool
# (c) Copyright Rosetta Commons Member Institutions.
# (c) This file is part of the Rosetta software suite and is made available under license.
# (c) The Rosetta software is developed by the contributing members of the Rosetta Commons.
# (c) For more information, see http://www.rosettacommons.org. Questions about this can be
# (c) addressed to University of Washington CoMotion, email: license@uw.edu.

import os
import tools.build.doxyfile_constants as constants


class Doxyfile:
    """
    Doxyfile representation.
    """

    def __init__(self, filename = None, **settings):
        """Create a Doxygen configuration file, and load its options."""

        if filename is None: filename = "Doxyfile"
        self.filename = filename
        self.__settings = {}
        if os.path.exists(self.filename):
            # If the file exists read it in
            self.load()
        else:
            # Otherwise use the defaults
            self.initialize_defaults()
        # Apply all settings
        self.assign(**settings)


    # Convenience method.  We are not trying to make this class a dictionary.
    def __getitem__(self, key):
        return self.__settings[key]


    # Convenience method.  We are not trying to make this class a dictionary.
    def __setitem__(self, key, value):
        if key not in self.__settings:
            raise KeyError("Doxyfile has no such field: '%s'" % key)
        self.__settings[key] = value


    def assign(self, **settings):
        """
        Change the settings in the Doxyfile to those in 'settings'.
        Settings not part of the standard Doxygen options will be rejected.
        """
        for name, value in list(settings.items()):
            self[name] = value


    def initialize_defaults(self):
        for name in list(constants.defaults.keys()):
            self.__settings[name] = constants.defaults[name]


    def load(self, file = None):
        """
        Read in a Doxyfile.
        file - The readable stream to read from.  It must contain a valid
          Doxyfile or an exception will be thrown.   If this stream is None
          a file named self.filename is opened for writing.
        """
        if not file:
            file = open(self.filename, "r")
        for line in file:
            terms = line.split()
            # Note: the routine for parsing Doxyfiles is deliberately not trying
            # to be clever.  It assumes that all lines are of one or three forms:
            #    <name> = <value>
            #    <name> =
            #    #.+
            if len(terms) > 1 and terms[0] != "#":
                # <name> =
                if len(terms) == 2:
                    name, value = terms[0], ""
                # <name> = <value>
                elif len(terms) == 3:
                    name, value = terms[0], terms[2]
                # Add the fields
                if name not in self.__settings:
                    value = constants.defaults[name]
                self.__settings[name] = (value, constants.comments.get(name, ""))


    def save(self, file = None, with_comments = False):
        """
        Write out a Doxyfile.  The file will be overwritten if it exists.
        file - The writeable stream to write out the contents to.  If this
          stream is None a file named self.filename is opened for writing.
        with_comments - True if comments should be included.
        """
        # Default to wrt
        if not file:
            file = open(self.filename, "w+")
        file.write(
            "# Doxygen configuration.\n" + \
            "# NOTE: This file was automatically generated by the Rosetta build system.\n" + \
            "# Modifications to it will be overriden: use doc/<project>.doc.settings instead.\n" + \
            "\n"
            )
        if with_comments:
            self.write_comments(file, constants.header)
        # Write out each field
        items = list(self.__settings.items())
        items.sort()
        for name, value in items:
            if with_comments:
                if name in constants.comments:
                    self.write_comments(file, constants.comments[name])
            file.write("%-24s = %s\n" % (name, value))
        file.close()


    def write_comments(self, file, comment):
        lines = comment.split("\n")
        file.write("\n")
        for line in lines:
            file.write("# %s\n" % line)
        file.write("\n")



def build_doxyfile(target, source, env):
    """SCons Builder function for Doxyfiles"""

    assert len(target) == 1, \
           "Target for Doxyfile build should be filename not '%s'" % str(target)
    targets = str(target[0])
    sources = " ".join(map(str, source))
    settings = env["DOXYFILE_OPTIONS"] or {}

    doxyfile = Doxyfile(targets, **settings)
    # The inputs should be precisely those files in the sources
    doxyfile["INPUT"] = sources
    doxyfile.save()


def generate(env):
   """
   Add builders and construction variables for the Doxygen tool.
   """

   # Create a builder from the above function
   doxyfile_builder = env.Builder(action = build_doxyfile)

   # Add the Doxyfile() method to the environment
   env.Append(BUILDERS = { "Doxyfile" : doxyfile_builder })

   # Doxygen has no command line input: all configuration is done via a file.
   # This specifies the name of the file
   if "DOXYFILE_FILE" not in env:
       env["DOXYFILE_FILE"] = "Doxyfile"
   # Options to add to $DOXYFILE to modify the run
   if "DOXYFILE_OPTIONS" not in env:
       env["DOXYFILE_OPTIONS"] = {}



def exists(env):
   """
   Make sure doxygen exists.
   """
   return env.Detect("doxygen")
