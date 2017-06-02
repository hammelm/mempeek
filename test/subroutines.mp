#
# test case: subroutines
#
# output:
# [ 2 4 8 ]
# [ 10 10 10 ]
# [ 3 9 27 ]
# [ 0 0 0 ]
# [ 3 7 8 ]
# [ 3 9 27 ]
# [ 2 4 8 ]
# [ 125 25 5 ]
# 24
# 120
# 420 2226
# 0 0 0
# 1 1
# 2 2 2
# 3 3
# 4 4 4
# 5 quit

defproc printarray a[]
    print "[" noendl
    for i from 0 to a[?] - 1 do print " " dec a[i] noendl
    print " ]"
endproc

a1[] := [ 2, 4, 8 ]
a2[] := [ 3, 9, 27 ]
a3[] := [ 125, 25, 5 ]

defproc arraytest a[] b[]
	global a1[]
	static a2[] := [ 10, 10, 10 ]
	static a3[] := a[]
	static a4[3]
	printarray a1[]
	printarray a2[]
	printarray a3[]
	printarray a4[]
	a2[] := a[]
	a3[] := a1[]
	a4[] := b[]
	a1[0] := a1[0] + 1
endproc

arraytest a2[] a3[]
a1[1] := a1[1] + a1[0]
arraytest a3[] a1[]

deffunc fac( n )
  if n <= 1 then return := 1
  else return := n * fac( n - 1 )
endfunc

defproc decprint n
  print dec n
endproc

decprint fac(4)
decprint fac(5)

y := 10
z := 20
deffunc f( x )
  global y
  static z := x + y
  return := x + y + z
  z := z + 1
endfunc

print dec f( z * 10 ) noendl
y := 15
print " " dec f( z * 100 )

defproc exittest i
  print dec i noendl
  if i == 1 then exit
  if i == 3 then break
  if i == 5 then
    print " quit"
    quit
  endif
  print " " dec i noendl
endproc

for i from 0 to 6 do
  exittest i
  print " " dec i
endfor
