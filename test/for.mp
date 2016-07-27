#
# test case: for loop
#
# output:
# 1 2 3 4 
# -1 0 1 
# 3 2 1 0 
# 1 3 
# 4 1 
# -1 -5 -9 

for i from 1 to 4 do
    print neg i " " noendl
endfor
print

for i from -1 to 1 do print neg i " " noendl
print

for i from 3 to 0 step -1 do print neg i " " noendl
print

for i from 1 to 4 step 2 do print neg i " " noendl
print

for i from 4 to 1 step -3 do
    print neg i " " noendl
endfor
print

for i from -1 to -10 step -4 do
    print neg i " " noendl
endfor
print
