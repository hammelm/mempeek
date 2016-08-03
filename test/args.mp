#
# test case: variadic arguments
#
# output:
# 0 10 [ 1 2 3 ] 42 [ ] [ 1 1 2 3 5 8 ]
# 2 10
# 1 10 [ 1 2 3 ] 42 [ ] [ 1 1 2 3 5 8 ]
# 2 [ 1 2 3 ]

pragma print dec

defproc test n ...
    print n noendl
    for i from 0 to args{?} - 1 do
        if args{i}[]? then
            print " [" noendl
            for j from 0 to args{i}[?] - 1 do
                print " " args{i}[j] noendl
            endfor
            print " ]" noendl
        else
            print " " args{i} noendl
        endif
    endfor
    print
    if n < 2 then
        if args{n}[]? then
            tmp[] := args{n}[]
            test 2 tmp[]
        else
            tmp := args{n}
            test 2 tmp
        endif
    endif
endproc

a := 10
b := 42
c[] := [ 1, 2, 3 ]
d[] := []
e[] := [ 1, 1, 2, 3, 5, 8 ]

test 0 a c[] b d[] e[]
test 1 a c[] b d[] e[]
