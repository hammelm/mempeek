#
# test case: strings
#
# output:
# [ 0x0000006f6c6c6568 ] [ 0x000000646c726f77 ] [ 0x726f776f6c6c6568 0x000000000000646c ]
# hello world
# 11:hello world
# [ 0x6f77206f6c6c6568 0x0000000021646c72 ]
# 10 10
# 0 2 1
# 1234 -1234 0
# 3.14 6.022e+23 0
# 0x1fe 0x1fe 0x01fe
# 0b1011 0b1011 0b001011
# 387 -42 3.14
# oworl  ld
# 2:hello,world,-
# 4:he,,o wor,d,-
# 3:he,o wor,d,,-

hello[] := "hello"
world[] := strcat( "wo", "rld" )
helloworld[] := [ 0x6f77206f6c6c6568, 0x0032313000646c72 ]

print array hex:64 hello[] " " world[] " " strcat( "hello", "world" )
print string hello[] " " world[]
print string dec strlen( helloworld[] ) ":" helloworld[]
print array hex:64 strcat( helloworld[], "!" )
print dec strlen( strcat( hello[], world[] ) ) " " strlen( hello[] ) + strlen( world[] )

print dec strcmp( "1234", "13" ) " " strcmp( "1234", "12" ) " " strcmp( "1234", "1234" )

print neg str2int( "1234" ) " " str2int( "-1234" ) " " str2float( hello[] )
print float str2float( "3.14" ) " " str2float( "6.022e23" ) " " str2float( world[] )

defproc strprint3 a[] b[] c[]
    print string a[] " " b[] " " c[]
endproc

strprint3 hex2str( 510 ) hex2str( 510, 2 ) hex2str( 510, 4 )
strprint3 bin2str( 11 ) bin2str( 11, 3 ) bin2str( 11, 6 )
strprint3 int2str( 387 ) signed2str( -42 ) float2str( 3.14 )
strprint3 substr( strcat( hello[], world[] ), 4, 5 ) substr( hello[], 6, 5 ) substr( world[], 3, 5 )

num_tokens := tokenize( "hello \t world" )
print dec num_tokens ":" string gettoken( 0 ) "," gettoken( 1 ) "," gettoken( 2 ) "-"

num_tokens := tokenize( "hello world", "l" )
print dec num_tokens ":" string gettoken( 0 ) "," gettoken( 1 ) "," gettoken( 2 ) "," gettoken( 3 ) "," gettoken( 4 ) "-"

num_tokens := tokenize( "hello world", "l+" )
print dec num_tokens ":" string gettoken( 0 ) "," gettoken( 1 ) "," gettoken( 2 ) "," gettoken( 3 ) "," gettoken( 4 ) "-"
