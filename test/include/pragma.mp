# helper script for print modifier test

defproc printtest1 value
    print "default: " value
    print dec "dec: " value
    print hex "hex: " value
endproc

pragma print bin

defproc printtest2 value
    print "default: " value
    print dec "dec: " value
    print hex "hex: " value
endproc

pragma wordsize 32

defproc printtest3 value
    print "default: " value
    print dec "dec: " value
    print hex "hex: " value
endproc
