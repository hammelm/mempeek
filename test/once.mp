# test case: import and run
#
# output:
# import: cnt = 1
# import: cnt = 2
# cnt = 2
# import: cnt = 3
# cnt = 3
# import: cnt = 4
# cnt = 4

pragma loadpath "include"
pragma print dec

cnt := 0

run "import.mp"
run "import.mp"

print "cnt = " cnt

import "import.mp"
import "import.mp"

print "cnt = " cnt

run "import.mp"

print "cnt = " cnt
