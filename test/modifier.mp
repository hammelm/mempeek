# test case: pragma for print modifiers
#
# output:
#
# default: 0xcdef
# dec: 52719
# hex: 0xcdef
# default: 1100 1101 1110 1111
# dec: 52719
# hex: 0xcdef
# default: 1001 0000 1010 1011 1100 1101 1110 1111
# dec: 2427178479
# hex: 0x90abcdef
# --------------------------------
# default: 0xcdef
# dec: 52719
# hex: 0xcdef
# default: 1100 1101 1110 1111
# dec: 52719
# hex: 0xcdef
# default: 1001 0000 1010 1011 1100 1101 1110 1111
# dec: 2427178479
# hex: 0x90abcdef
#

pragma loadpath "include"
pragma wordsize 16

import "pragma.mp"

v := 0x1234567890abcdef

printtest1 v
printtest2 v
printtest3 v

print "--------------------------------"

pragma wordsize 64

printtest1 v
printtest2 v
printtest3 v
