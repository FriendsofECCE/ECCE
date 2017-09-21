#! /usr/bin/python

from os import environ, path
from subprocess import Popen
from sys import argv # should eventuall switch to argparse
import getpass
import tempfile
import time
import json
import argparse
import env

if "ECCE_PATH" in environ:
    ECCE_PATH = environ["ECCE_PATH"]
else:
    # Just assume that we are ../ relative to this script
    ECCE_PATH = path.abspath(path.dirname(__file__))
    ECCE_PATH = path.abspath("{}/../".format(ECCE_PATH))

ECCE_APP_PATH = "{}/apps".format(ECCE_PATH)
ECCE_SERVER_PATH = "{}/server".format(ECCE_PATH)

# Do some sanity checks to see if the path exists
APP_INSTALLED = path.exists(ECCE_APP_PATH)
SERVER_INSTALLED = path.exists(ECCE_SERVER_PATH)

if not APP_INSTALLED and not SERVER_INSTALLED:
    print("This script couldn't find an APP or a SERVER installation in the path {}".format(ECCE_PATH))
    exit("Please check your installation or set the ECCE_PATH env variable (should only be necessary for debugging)")

# Define major paths
def main():
    """
    Parses the arguments and calls the appropriate ecce script/program
    """
    if len(argv) == 1:
        default(None)
        return None

    parser = argparse.ArgumentParser()
    parser.set_defaults(func=default)
    subparsers = parser.add_subparsers()

    parser_envfile = subparsers.add_parser("envfile", help="Outputs \
    a file containing all ECCE related paths")
    parser_envfile.add_argument("outfile")
    parser_envfile.set_defaults(func=envfile)

    args = parser.parse_args()
    args.func(args)


def get_env():
    """
    Sets the default ecce environment variables based on the location of
    the installed ecce packags. Uses the settings.json file for these purpose
    """
    env = environ.copy()

    settings = SettingsBundle("settings.json", "defaults.json")
    if settings["SLAVE"]:
        # Download slave environment and load that settings file
        pass

    for key, value in settings.flatten().iteritems():
        cleaned = to_env(value)
        if not cleaned is None and not key in environ:
            # I don't want "False" values included
            # or values that are defined in the environment
            env[key] = cleaned

    env["ECCE_HOME"] = environ.get("ECCE_HOME", "{}/{}".format(ECCE_PATH, "apps"))
    env["ECCE_SERVER_PATH"] = ECCE_SERVER_PATH
    env["ECCE_REALUSERHOME"] = environ.get("ECCE_REALUSERHOME", environ["HOME"])
    env["ECCE_SYSDIR"] = "" # Needed for backwards compatibility
    env["ECCE_REALUSER"] = env["USER"] = environ.get("USER", getpass.getuser())
    env["ECCE_TMPDIR"] = environ.get("ECCE_TMPDIR", tempfile.gettempdir())
    env["HOST"] = environ.get("HOST", environ["HOSTNAME"])

    # Unless strange things are happening with fonts, disable custom fontconfig
    # env["FONTCONFIG_FILE"] = "{}/data/client/config/fonts.conf".format(env["ECCE_HOME"])
    # env["FL_FONT_PATH"] = "{}/data/client/fonts".format(env["ECCE_HOME"])

    # PATH manipulation
    env["ECCE_BIN_PATH"] = "{}/bin".format(env["ECCE_HOME"])
    env["ECCE_SCRIPT_PATH"] = "{}/scripts".format(env["ECCE_HOME"])
    env["ECCE_REMOTE_SERVER"] = True
    parsers = "{}/scripts/parsers".format(env["ECCE_HOME"])
    basepath = environ.get("PATH", "/usr/bin:/usr/local/bin:/usr/sbin")
    env["PATH"] = "{bin}:{scripts}:{parsers}:{basepath}".format(
        bin=env["ECCE_BIN_PATH"],
        scripts=env["ECCE_SCRIPT_PATH"],
        parsers=parsers,
        basepath=basepath)

    # LD_LIBRARY_PATH manipulation
    ecce_libs = ""
    xerces_libs = ""
    wxwidgets = ""
    baselibpath = env.get("LD_LIBRARY_PATH", "")

    # PYTHONPATH

    # Java configuration


    if env.get("DEBUG", "false") == "true":
        for k, v in env.iteritems():
            print("{}=\"{}\"".format(k, v))

    return env


def to_env(value):
    """
    Converts to a string which is compatible with being in an environment
    """
    if isinstance(value, basestring):
        return value
    elif isinstance(value, bool):
        if value:
            return "true"
        return None
    elif isinstance(value, int) or isinstance(value, float):
        return str(value)
    return None


def run(command, env=None, *args, **kwargs):
    """
    Runs a process using Popen. Should fail gracefully if it fails
    """
    if not env:
        env = get_env()

    cmd = [command] + list(args)
    process = Popen(cmd, env=env, **kwargs)
    process.wait()
    print("ECCE exited with return code {}".format(process.returncode))


def server_start():
    """
    Starts the ecce server using start_ecce_server
    """
    run("./start_ecce_server", cwd="{}/ecce-admin".format(ECCE_SERVER_PATH))
    # Give the server time to warm up
    time.sleep(10)

def server_stop():
    """
    Stops the ecce server using stop_ecce_server
    """
    run("./stop_ecce_server", cwd="{}/ecce-admin".format(ECCE_SERVER_PATH))


def default(args):
    """
    The default action to be taken when this function is called
    without a subcommand
    """
    server_start()
    server_stop()


def envfile(args):
    """
    Reads the environment data and outputs it to a specified file

    args:
        outfile -- path to output file
    """
    print_env(args.outfile)


def print_env(path):
    """
    Writes all process environment variables which define an
    environment with wich to run ecce binaries. This is mean for
    debugging.
    """
    with open(path, 'w') as outfile:
        outfile.write("# Automatically generated environment file for ECCE\n")
        # Check to see which shell the user is using.
        # Supported shells are bash, csh, and fish
        # if environ["SHELL"]
        # elif environ["SHELL"].con:
        #     pass
        for key, value in environ.iteritems():
            outfile.write('export {}="{}"\n'.format(key, value))

def ecce_ecce():
    """
    Run the scripts/ecce command with proper environment variables
    """
    pass


class SettingsBundle(object):
    """
    Contains the contents of multiple JSON files and caskades through
    a list of these JSON file until a match is found.
    """
    sources = []

    def __init__(self, *args):
        """
        Accepts a list of JSON file names or dictionaries to use as settings sources
        """
        for source in args:
            # Load a JSON file if if provided a string
            if isinstance(source, str):
                with open(source, "r") as json_file:
                    self.sources.insert(len(self.sources), json.load(json_file))
            # If it's just a dictionary, just added it to the settings sources
            elif isinstance(source, dict):
                self.sources.insert(len(self.sources), source)
            else:
                print("Type of {} is not supported in a SettingsBundle".format(type(source)))
                exit(1)


    def add_source(self, new_source, index=0):
        """
        Adds the given source to the source list and makes it the highest priority source
        unless index is specified
        """
        if isinstance(new_source, str):
            with open(new_source, "r") as json_file:
                self.sources.insert(index, json.load(json_file))
        # If it's just a dictionary, just added it to the settings sources
        elif isinstance(new_source, dict):
            self.sources.insert(index, new_source)


    def __getitem__(self, key):
        for source in self.sources:
            try:
                return source[key]
            except KeyError:
                pass
        raise KeyError(key)

    def keys(self):
        """
        Returns the keys in this object
        """
        keys = []
        for source in self.sources:
            keys = keys + list(source.keys())

        # Return a unique list of keys
        return list(set(keys))


    def flatten(self):
        """
        Takes the existing settings sources
        """
        flattened = {}
        # Perhaps not the most efficient way of doing this, but it shouldn't be called often
        for source in reversed(self.sources):
            for key, value in source.iteritems():
                flattened[key] = value
        return flattened

if __name__ == "__main__":
    main()
