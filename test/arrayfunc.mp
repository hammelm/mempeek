#
# test case: array functions
#
# output:
# 0
# [ 1 1 2 3 5 8 13 21 34 55 ]
# 0
# [ 3 4 6 9 14 22 35 56 90 145 ]

defproc printarray a[]
    print "[" noendl
    for i from 0 to a[?] - 1 do print " " dec a[i] noendl
    print " ]"
endproc

deffunc[] fibonacci( n )
    print dec return[?]
    dim return[n]
    for i from 1 to 2 do
        return[ i - 1 ] := 1
    endfor
    for i from 3 to n do
        return[ i - 1 ] := return[ i - 2 ] + return[ i - 3]
    endfor
endfunc

deffunc[] sum( a[], b )
    print dec return[?]
    dim return[ a[?] ]
    for i from 1 to a[?] do
        sum := a[ 0 ]
        for j from 2 to i do
            sum := sum + a[ j - 1 ]
        endfor
        return[ i - 1 ] := sum + b
    endfor
endfunc

fib[] := fibonacci( 10 )
printarray fib[]

fib[] := sum( fib[], 2 )
printarray fib[]
