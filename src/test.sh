#! /usr/bin/bash
make clean all debug
if ! ./test_bdd-g | grep FAILED; then
	echo "All tests PASSED";
fi
