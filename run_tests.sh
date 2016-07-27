#!/bin/bash

desired_output=$(mktemp -p generated desired.XXXXXXXX)
actual_output=$(mktemp -p generated actual.XXXXXXXX)
awk_script=$(mktemp -p generated awk.XXXXXXXX)

function finish {
	rm $desired_output
	rm $actual_output
	rm $awk_script
}
trap finish EXIT

cat > $awk_script <<EOF
BEGIN { state = 0 }
/^# output:/ { state = 1 }
/^#/ { if( state == 1 ) state = 2
       else if( state == 2 ) print substr( \$0, 3 ) }
!/^#/ { state = -1 }
EOF

for test in test/*.mp; do
	echo
	echo "######################################################################"
	echo "#"
	echo "# file: $test"
	echo

	awk -f $awk_script $test > $desired_output
	./bin/mempeek $test | tee $actual_output

	if ! cmp -s $desired_output $actual_output; then
		echo
		echo "*** TEST FAILED!"
		echo
		diff -u $desired_output $actual_output
		echo
		exit 1
	fi
done

echo
echo "######################################################################"
echo
echo "all tests ok!"
echo
