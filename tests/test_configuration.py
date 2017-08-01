#!/usr/bin/python
#
#  Tests that kdesk uses the correct configuration settings
#  by merging HOME settings into the main configuration file.
#

import os

def run_kdesk_configuration(configuration):

    # allow the tests to run either isolated on this repo, or on a kano os image / real RPI
    os.environ["PATH"] = '../src:' + os.environ['PATH']

    # we run kdesk in test mode, possibly with custom configuration file
    command='kdesk-dbg -v -t'
    if configuration:
        command += ' -c {}'.format(configuration)
    return os.popen(command).read()

def test_config_custom():
    config_file='configurations/kdeskrc_test'
    out=run_kdesk_configuration(config_file)
    assert(out.find('loading custom configuration file: {}'.format(config_file)) != -1)

def test_config_custom_ssaver():
    config_file='configurations/kdeskrc_test'
    out=run_kdesk_configuration(config_file)
    assert(out.find('found ScreenSaverTimeout: 30') != -1)
    assert(out.find('found ScreenSaverProgram: /usr/bin/qmlmatrix') != -1)

def test_config_combined_with_default():
    with open('{}/.kdeskrc'.format(os.getenv("HOME")), 'w') as f:
        f.write('ScreenSaverTimeout: 10')

    out=run_kdesk_configuration(None)
    assert(out.find('found ScreenSaverTimeout: 10') != -1)

def test_config_combined_with_custom():
    config_file='configurations/kdeskrc_test'
    with open('{}/.kdeskrc'.format(os.getenv("HOME")), 'w') as f:
        f.write('ScreenSaverTimeout: 10')

    out=run_kdesk_configuration(config_file)
    assert(out.find('found ScreenSaverTimeout: 10') != -1)
