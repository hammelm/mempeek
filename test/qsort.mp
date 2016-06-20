#
# test case: recursion
#
# output:
# 0 1 2 3 4 5 6 7 8 9
#

defproc qsort a[] start end
    i := start
    j := end
    pivot := a[ (start + end) >> 1 ]

    while i <= j do
        while a[i] < pivot do i := i + 1
        while a[j] > pivot do j := j - 1
        if i > j then break
        dummy := a[i]
        a[i] := a[j]
        a[j] := dummy
        i := i + 1
        j := j - 1
    endwhile

    if start < j then qsort a[] start j
    if i < end then qsort a[] i end
endproc

test[] := [ 3, 1, 4, 9, 7, 0, 6, 8, 2, 5 ]
qsort test[] 0 9

for i from 0 to test[?] - 1 do
    if i == 0 then print dec test[i] noendl
    else print dec " " test[i] noendl
endfor
print
