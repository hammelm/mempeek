#
# test case: arrays
#
# output:
# a: 20
# b: [ 1 2 3 ]
# 13
# [ 42 ]
# [ ]
# 343
# 13002
# 19032
# 0: [ 1 2 3 ]
# 1: [ 10 20 ]
# 2: [ 1 2 3 ]
# 42

defproc printarray a[]
    print "[" noendl
    for i from 0 to a[?] - 1 do print " " dec a[i] noendl
    print " ]"
endproc

deffunc sum( a[] )
    return := 0
    for i from 0 to a[?] - 1 do return := return + a[i]
endfunc

deffunc f( a, b[] )
    return := a + b[?]
    dim b[1]
    b[0] := 42
endfunc

defproc p a b[]
    print "a: " dec a
    print "b: " noendl
    printarray b[]
endproc

x[] := [ 1, 2, 3 ]

p 20 x[]

y := f( 10, x[] )

print dec y
printarray x[]

z[] := [ 1, 1, 2, 3, 5, 8 ]
z[] := []
printarray z[]

deffunc g( a[], b[], c )
    x[] := [ 100, 200 ]
    if c == 0 then return := g( b[], x[], 1 ) + a[?]
    else return := sum( a[] ) + sum( b[] )
endfunc

print dec g( x[], x[], 0 )

s[] := [ 1000, 2000 ]
t[] := [ 3000, 4000 ]
deffunc h( r[] )
    global s[]
    static t[] := [ r[?], r[0] ]
    return := sum( r[] ) + sum( s[] ) + sum( t[] )
    t[] := [ 100, 200, 300 ]
endfunc

print dec h( t[] )
t[] := [ 111, 1000, 4321, 10000 ]
print dec h( t[] )

defproc r n a[]
    print dec n ": [" noendl
    for i from 0 to a[?] - 1 do print " " dec a[i] noendl
    print " ]"
    if n == 0 then
        b[] := [ 10, 20 ]
        r 1 b[]
        r 2 a[]
    else a[0] := 42
endproc

t[] := [ 1, 2, 3 ]
r 0 t[]
print dec t[0]
