#
# test case: while loop
#
# output:
# 1024
#

pow := 1
cnt := 10

while cnt > 0 do
  pow := pow * 2
  cnt := cnt - 1
endwhile

print dec pow
