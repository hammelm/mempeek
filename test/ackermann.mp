#
# test case: recursion
#
# output:
# 15
# 253

deffunc a( n, m )
  if n == 0 then
    return := m + 1
  else if m == 0 then
    return := a( n - 1, 1 )
  else
    return := a( n - 1, a( n, m - 1 ) )
  endif
endfunc

print dec a( 2, 6 )
print dec a( 3, 5 )
