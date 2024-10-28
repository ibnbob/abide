#! /usr/bin/bash
make clean all debug
test_out=$(mktemp TEST.OUT.XXXX)
if ./bdd_test-g > $test_out; then
    if ! grep FAILED $test_out; then
	echo "All tests PASSED";
	rm $test_out
    fi
fi
