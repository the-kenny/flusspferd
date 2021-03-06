#!/bin/sh
# vim:ts=2:sw=2:expandtab:autoindent:
#
# Copyright (c) 2008, 2009 Aristid Breitkreuz, Ash Berlin, Ruediger Sonderfeld
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:

# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.

# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.
#

if [ ! -d ./build ]
then
  echo "ERROR: Must be in global project directory!" 1>&2
  exit 2
fi

echo "PROGRESS: Clearing test coverage counters" 1>&2

LCOV_MODE=-z ./util/lcov.sh

echo "PROGRESS: Running tests" 1>&2

for prog in ./build/bin/test_*
do
  if [ -x $prog ]
  then
    echo "Testing '$prog'" 1>&2
    $prog 2>&1
  fi
done

echo "PROGRESS: Analyzing test coverage" 1>&2

./util/lcov.sh

lcov -q -r ./build/coverage.info '/usr*' 'test/*' -o ./build/coverage.info

echo "PROGRESS: Visualizing test coverage" 1>&2

./util/genhtml.sh
