#! /usr/bin/bash
make debug
if ! ./test_bdd-g | grep FAILED; then
	echo "All PASSED";
fi
