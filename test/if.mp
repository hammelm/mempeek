#
# test case: conditional blocks
#
# output:
# 0: dfhjk
# 1: abcegil
# 2: abcegim

for i from 0 to 2 do

    print dec i ": " noendl

    if i > 0 then print "a" noendl

    if i > 0 then
       print "b" noendl
    endif

    if i > 0 then print "c" noendl
    else print "d" noendl

    if i > 0 then
       print "e" noendl
    else print "f" noendl

    if i > 0 then print "g" noendl
    else
        print "h" noendl
    endif

    if i > 0 then
       print "i" noendl
    else
        print "j" noendl
    endif

    if i == 0 then print "k" noendl
    else if i == 1 then print "l" noendl
    else print "m" noendl

    print

endfor
