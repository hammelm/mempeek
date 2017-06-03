#!/usr/bin/env bash

set -e

for test in test/*.mp; do
	echo
	echo "######################################################################"
	echo "#"
	echo "# file: $test"
	echo

	valgrind -q --leak-check=full --error-exitcode=1 ./bin/mempeek $test

done

echo
echo "######################################################################"
echo
echo "all tests ok!"
echo
