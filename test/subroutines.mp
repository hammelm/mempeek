#
# test case: subroutines
#
# output:
# 24
# 120
# 0 0 0
# 1 1
# 2 2 2
# 3 3 
# 4 4 4
# 5 quit
#

deffunc fac( n )
  if n <= 1 then return := 1
  else return := n * fac( n - 1 )
endfunc

defproc decprint n
  print dec n
endproc

decprint fac(4)
decprint fac(5)

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
