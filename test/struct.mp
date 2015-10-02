#
# test case: def statement
#
# output:
# struct1: 0x1000 0x1002 0x1004 0x100c
# struct2: 0x2000 0x2000 0x2008 0x200c
#

def struct1 0x1000
def struct1.m1 0x00
def struct1.m2 0x04
def struct1.m3 0x0c

def struct2 0x2000 from struct1

def struct1.m1 0x02
def struct2.m2 0x08

print hex:16 "struct1: " struct1 " " struct1.m1 " " struct1.m2 " " struct1.m3
print hex:16 "struct2: " struct2 " " struct2.m1 " " struct2.m2 " " struct2.m3
