import os
import sys
import argparse
import configparser

def parse_args(cmd_args):
    arg_parser = argparse.ArgumentParser(description="generate physical stripes")
    arg_parser.add_argument("-config_filename", type=str, required=True, help="configuration file name")
    args = arg_parser.parse_args(cmd_args)
    return args


def main():
    args = parse_args(sys.argv[1:])
    if not args:
        exit()

    # read config
    config_filename = args.config_filename
    config = configparser.ConfigParser()
    config.read(config_filename)
    



if __name__ == '__main__':
    main()