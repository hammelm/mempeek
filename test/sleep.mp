#
# test case: sleep command
#
# output:
# below eps
# below eps

delta := 1000000
eps := 10000

defproc test t1 t2
  global delta
  global eps
  if t2 - t1 - delta < eps then print "below eps"
  else
    diff := t2 - t1
    if diff < delta then print "failed: -" dec delta - diff
    else print "failed: +" dec diff - delta
  endif
endproc

t1 := now
sleep delta
t2 := now
test t1 t2

t3 := now
sleep delta / 2
sleep until t3 + delta
t4 := now
test t3 t4
