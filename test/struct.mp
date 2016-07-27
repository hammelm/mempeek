#
# test case: def statement
#
# output:
# struct1: 0x1000 0x1002 0x1004 0x100c
# struct2: 0x2000 0x2000 0x2008 0x200c
# struct2: 0x0010 0x2100 0x210f 0x0006 0x2200 0x220a
# struct3: 0x0010 0x3100 0x310f 0x0004 0x3110 0x311c

pragma wordsize 16

def struct1 0x1000
def struct1.m1 0x00
def struct1.m2 0x04
def struct1.m3 0x0c

def struct2 0x2000 from struct1

def struct1.m1 0x02
def struct2.m2 0x08

print hex:16 "struct1: " struct1 " " struct1.m1 " " struct1.m2 " " struct1.m3
print hex:16 "struct2: " struct2 " " struct2.m1 " " struct2.m2 " " struct2.m3

def struct2.r0:8{16} 0x100
def struct2.r1:32{4} 0x110

def struct3 0x3000 from struct2

def struct2.r1{6} 0x200

print hex:16 "struct2: " struct2.r0{?} " " struct2.r0{0} " " struct2.r0{15} \
                     " " struct2.r1{?} " " struct2.r1{0} " " struct2.r1{5}

print hex:16 "struct3: " struct3.r0{?} " " struct3.r0{0} " " struct3.r0{15} \
                     " " struct3.r1{?} " " struct3.r1{0} " " struct3.r1{3}
