#!/bin/bash
# Creates a demo html page with unittest output.
# This excutes the unittest which must be built in ../bld
# and uses ansi2html.sh to convert the ANSI colors to HTML.
# Run this before executing doxygen.

# ansi2html.sh options
OPTIONS=

../bld/test/test_Test -h | ./ansi2html.sh $OPTIONS > demo_help.html
../bld/test/test_Test -c always | ./ansi2html.sh $OPTIONS > demo.html

