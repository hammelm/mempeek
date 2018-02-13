Mempeek
=======

Mempeek is a simple programming language designed to operate on the physical memory of a
computer system. It was implemented to help in the development of embedded systems running
under a Posix compliant operating system. It enables the developer to directly interact
with memory mapped peripheral devices without the need to write a kernel driver. Using
mempeek makes it very easy to interactively explore the features of new peripherals.

    Usage: mempeek [options] [script] ...

    Options:
        -i          Enter interactive mode when all scripts and commands are completed
        -I <path>   Add <path> to the search path of the "import" command
        -c <stmt>   Execute the mempeek command <stmt>
        -l <file>   Write output and interactive input to <file>
        -ll <file>  Append output and interactive input to <file>
        -v          Print version
        -h          Print usage

The -I and -c options can be used several times to add more than one include path or to
execute more than one command. Options and commands can be mixed and are executed in the
same order as in the args list.

Interactive mode starts an interactive console after all scripts and commands are executed.
When no script or -c option is used in the args, the program enters interactive mode even
if no -i option is used. Entering the command "quit" finishes interactive mode.


Mempeek language description
============================

Mempeek allows the user to map physical memory regions into the process memory space and to
read or write values with different word sizes from these memory regions. The values can be
stored in variables to be processed with arithmetical or logical expressions. The language
provides control structures for conditional execution or loops in the control flow.

There is a distinction between two types of variables in mempeek. Definition variables are
known at compile time and can be used in memory mapping commands. Assignment of values to
definition variables is restricted to ensure that the value of such a variable is always
known at compile time. Normal variables have no restrictions, but some commands do not
allow the usage of normal variables.

Definition variables are handled differently within functions and procedures. Normal
variables defined at global scope are not visible within the function or procedure scope
unless they are declared global. Definition variables are visible within the function or
procedure scope without any extra global declaration.

The following commands are available in the mempeek language:

assigning values to definition variables
----------------------------------------

        def <name> <expression>

Assign the result of evaluating *expression* to the definition variable *name*

        def <base>.<name> <expression>

The definition variable *base.name* is assigned the value *base* + *expression*

        def <base>.<name>[size]{ <expr1> } <expr2>

The definition variable *base.name* is assigned the value *base* + *expr2*. Additionally,
an index access is set up to access *expr1* consecutive registers starting at
*base.name*. *size* can be ":8", ":16", ":32" or ":64" and defines the register size in
bits, i.e. *base.name{i}* is translated to the address *base.name* + *i* * *size*/8. The
default size is the system word size.

        def <base> <expression> from <name>

Clone all derived definition variables name.x to base.x and assign them to the new base
value *expression*

These operations are not allowed within for loops, while loops, and if statements. All
expressions in the definitions must be constant and are evaluated at compile time.

assigning values to variables
-----------------------------

        name := <expression>

Assign the result of evaluating *expression* to the variable *name*. It is not allowed to
assign values to a definition variable.

All variables or definition variables are internally stored as unsigned 64 bit values. The
print, peek, and poke commands can be restricted to operate on 8, 16, and 32 bit values,
the default size is the native word size of the system. When a restriction occurs, the
upper bits are cut off (poke and print) or set to zero (peek).

Numerical values can be specified as decimal numbers, hexadecimal numbers starting with the
prefix "0x" or binary number starting with the prefix "0b".

Numerical values containing a decimal point or exponential values (e.g. 3.14, .5 or 1e-6)
are interpreted as floating point numbers and are converted to a 64 bit integer value
representing the IEEE 754 double precision encoding of the floating point number.

arrays
------

        dim name[ <expression> ]

Create or resize the array *name* to contain *expression* elements. When the array size
is increased or a new array is created, the new elements are initialized with zero. When
the array size is decreased, the remainig elements are not changed.

        name[?]
        name[ <expression> ]

The first line retrieves the size of the array, the second line retrieves the value at
index position *expression* from the array.

        name[] := [ <expression>, ... ]
        name[] := array[]
        name[] := function()

The first line assigns an array that contains the elements defined by the expressions
within the square brackets to name. The second line copies array to name. The third line
evaluates an array function and assigns the result to name.

expressions
-----------

Expressions consist of variables or definition variables, arithmetic expressions, logical
expressions, comparisons, constant values or a peek command

Arithmetic operators:

        <expr1> + <expr2>       add <expr1> to <expr2>
        <expr1> - <expr2>       subtract <expr2> from <expr1>
        <expr1> * <expr2>       multiply <expr1> with <expr2>
        <expr1> / <expr2>       divide <expr1> by <expr2>
        <expr1> % <expr2>       remainder of dividing <expr1> by <expr2>
        - <expr>                negate <expr> (calculate two's complement)

Addition, substraction, and multiplication results are identical for signed and unsigned
operands, whereas the division and remainder operators have different results. Therefore
signed division and remainder operators are provided which treat their operands as signed
integers in two's complement. The division result is always truncated towards zero, and
the remainder fulfills the equation `(x -/ y) * y + (x -% y) == x`.

        <expr1> -/ <expr2>      signed division of <expr1> with <expr2>
        <expr1> -% <expr2>      signed remainder of dividing <expr1> by <expr2>

Bit manipulation operators:

        <expr1> << <expr2>      left shift <expr1> by <expr2> bits
        <expr1> >> <expr2>      right shift <expr1> by <expr2> bits
        <expr1> & <expr2>       bitwise AND of <expr1> and <expr2>
        <expr1> | <expr2>       bitwise OR of <expr1> and <expr2>
        <expr1> ^ <expr2>       bitwise XOR of <expr1> and <expr2>
        ~ <expr>                bitwise NOT of <expr>

Boolean operators (expressions are treated as true or false by these operators. A value of
zero is false, all other values are true. The result of logical operators is 0 for false
and -1 for true ):

        <expr1> && <expr2>      logical AND of <expr1> and <expr2>
        <expr1> || <expr2>      logical OR of <expr1> and <expr2>
        <expr1> ^^ <expr2>      logical XOR of <expr1> and <expr2>
        ! <expr>                logical NOT of <expr>

Comparison operators (comparisons evaluate to the values 0 for false and -1 for true):

        <expr1> < <expr2>       true if <expr1> is lower than <expr2>
        <expr1> > <expr2>       true if <expr1> is greater than <expr2>
        <expr1> <= <expr2>      true if <expr1> is lower than or equal to <expr2>
        <expr1> >= <expr2>      true if <expr1> is greater than or equal to <expr2>
        <expr1> == <expr2>      true if <expr1> is equal to <expr2>
        <expr1> != <expr2>      true if <expr1> is not equal to <expr2>

Comparison operators for signed integers. These operators are identical to the previous
operators, but they treat their arguments as signed integers in two's complement.

        <expr1> -< <expr2>       true if <expr1> is lower than <expr2>
        <expr1> -> <expr2>       true if <expr1> is greater than <expr2>
        <expr1> -<= <expr2>      true if <expr1> is lower than or equal to <expr2>
        <expr1> ->= <expr2>      true if <expr1> is greater than or equal to <expr2>

operator precedence
-------------------

the operators have the following precedences, with higher precedence listed first:

        1:  ! ~ - (unary)
        2:  * / % -/ -% &
        3:  + - | ^
        4:  << >>
        5:  < <= > >= == != -< -<= -> ->=
        6:  &&
        7:  || ^^

literals
--------

Mempeek provides three different types of literals: integer literals, floating point
literals, and string literals. Integer literals are signed or unsigned decimal numbers,
hexadecimal numbers starting with *0x* or binary numbers starting with *0b*. Integer
literals can be used in expressions and are transformed to a 64 bit value. Signed decimal
literals are transformed to the 64 bit value representing the negative number in two's
complement.

Floating point literals are written as decimal numbers either with a decimal point or as
numbers in scientific notation using the common *e* notation (e.g. *6.022e-23*). Floating
point literals can be used in expressions and are transformed to a 64 bit value in IEEE
754 double precision encoding.

String literals are only allowed in print commands, assignments, and as subroutine
arguments and consist of printable ASCII characters between two double quotes. The
backslash character has a special meaning and is used to escape some special characters
which cannot be encoded as printable ASCII characters:

        \n    newline
        \r    carriage return
        \t    tabulator
        \"    double quote
        \\    backslash
        \x??  ?? is a two digit hexadecimal number representing the character value

conditional execution
---------------------

        if <expr> then <command>
        [else <command>]

        if <expr> then
            <command>
            ...
        [else
            <command>
            ...]
        endif

Evaluate *expr* and execute the command(s) after the "then" keyword if *expr* is true or
execute the command(s) after the optional "else" keyword if *expr* is false When the
command is written behind "then" or "else" without a newline, only this single command is
executed. When a newline is found, all commands until the next "else" or "endif" keyword
are executed.

for loops
---------

        for <name> from <expr1> to <expr2> [step <expr3>] do <command>

        for <name> from <expr1> to <expr2> [step <expr3>] do
            <command>
            ...
            [break]
            ...
        endfor

Assign the value of *expr1* to variable *name*, then execute command(s) until the value of
the variable is greater than *expr2*. After each iteration increase the variable by *expr3*
when the "step" keyword is found, or else increase it by one. When the step value is a
negative number, stop iterating when the variable is lower than *expr2*. When the command
is written behind "do" without a newline, only this single command is executed. When a
newline is found, all commands until the next "endfor" keyword are executed. When a "break"
keyword is encountered within the loop, the loop is left immediately.

while loops
-----------

    while <expr> do <command>

    while <expr> do
        <command>
        ...
        [break]
        ...
    endwhile

Execute command(s) until the value of <expr> evaluates to 0. When the command is written
behind "do" without a newline, only this single command is executed. When a newline is
found, all commands until the next "endwhile" keyword are executed. When a "break" keyword
is encountered within the loop, the loop is left immediately.

output
------

        print [<modifier>] (<expression> | array[] | "string") ... [noendl]

Print the result of evaluating *expression* to the console, or print a string. When several
expressions or strings are given, they are concatenated into one line. The modifiers can be
"hex", "dec", "bin", "neg" or "float". When a modifier is given, all subsequent expression
results are printed as hexadecimal, unsigned decimal, binary, signed decimal or floating
point number. Each modifier except "float" can be used with an optional suffix ":8", ":16",
":32" or ":64" which specifies the number of bits of the result to be displayed. Several
modifiers can be used within the same print command. The default modifier for each print
command is "hex" with the default bit size of the system. The print command does add a
newline at the end of an output line. The last parameter of the print command can be the
modifier "noendl" which suppresses the newline.

When called on arrays or functions returning arrays, the print command writes the array
content using the currently active modifier. Two more modifiers "array" and "string" are
available for array parameters. The default modifier is "array" which writes the array
content between square brackets, the modifier "string" treats arrays as strings and writes
the ASCII string contained in the array.

mapping physical memory
-----------------------

        map <address> <size> ["device"] [at <base>]

Map *size* bytes of physical memory at address *address* to the process memory space. When
"device" is given, the mapping occurs on that device, default is /dev/mem. The physical
memory is mapped at the same address in the process memory space unless *base* is given.
In this case *base* is used as address in the process memory space. *address*, *size*, and
*base* must be constants, dynamic calculation of address spaces is not allowed. This
operation is not allowed within for loops, while loops, and if statements.

No memory can be accessed by peek and poke commands before it has been mapped with this
command.

memory access
-------------

        poke[size] <address> <value> [mask <mask>]

Write *value* to memory at *address*. [size] can be ":8", ":16", ":32" or ":64",
restricting the memory access to this bit size. Default size is the system bit size. When
the "mask" keyword is given, only the bits in memory which are set in *mask* are modified.

        peek[size]( <address> )

Read a value from memory at *address*. [size] can be ":8", ":16", ":32" or ":64",
restricting the memory access to this bit size. Default size is the system bit size.

functions and procedures
------------------------

Mempeek allows the definition of subroutines. There is a distinction between functions and
procedures which have slightly different syntax and semantics. Functions can be used in
expressions, and parameters are put between braces and are comma separated. A function
returns a value which is used for further calculations in the expression. Procedures can be
used like statements and do not have return values. The procedure parameters are written
without parentheses and are separated only by white space.

        defproc <name> [<parameter> [<parameter> [...]]]
            <command>
            ...
            [exit]
            ...
        endproc

        deffunc <name> ( [<parameter> [, <parameter> [, ...]]] )
            <command>
            ...
            [exit]
            ...
        endfunc

        deffunc[] <name> ( [<parameter> [, <parameter> [, ...]]] )
            <command>
            ...
            [exit]
            ...
        endfunc

Define a function or procedure with name *name* and optional parameters. The body of the
subroutine consists of a list of commands which are executed when the subroutine is called.
The parameters can be accessed as variables within the subroutine body. When an "exit"
keyword is encountered, the execution of the subroutine is stopped and control flow returns
to the caller.

Parameters are either scalars or arrays. To use arrays as function parameters, empty
square brackets must be added to the parameter name. Scalar parameters are passed by
value, and arrays are passed by reference. When changing an array in a function or
procedure, the array is also changed in the scope of the caller. This is not the case for
scalar variables. Instead of arrays it is possible to add array functions or string literals
as subroutine parameters. A temporary array is created in this case to hold the function
result or string literal.

Variables which are defined within the subroutine body are not visible outside of the
subroutine body, and variables defined outside of the subroutine body are not visible
within. Definition variables are visible within the subroutine body, but it is not allowed
to create or change definition variables from within a subroutine.

The function definition comes in two flavors. The first one defines a normal function with
scalar return value, the second one defines an array function which returns an array.

        return
        return[]

To return a value from a function, the special variable *return* is defined within the
function body. The result of the function must be assigned to this variable. On exit, the
value stored in the variable is used as return value. The variable *return* is a scalar
variable for normal functions, and an array variable for array functions.

The *return* variable is always initialized with zero, and a *return[]* array has always
size zero.

        global <var>
        static <var> := <expression>

The keyword "global" can be used to make a variable visible within the function scope which
was defined outside of the function. The keyword "static" can be used to define a variable
which is only visible within the function and remains unchanged between function calls.
The result of *expression* is used as initial value for the variable when the "static"
keyword is executed for the first time.

        global <var>[]
        static <var>[ <expression> ]
        static <var>[] := [ <expression>, ... ]
        static <var>[] := <local>[]

The "global" and "static" keywords are also allowed for arrays, with the same semantics as
for scalar variables. The three variants of the "static" command initialize the array
either with a given size and all elements set to zero, or copies the content from a square
bracket list of expressions or a local array.

        drop procedure
        drop function()

Functions and procedures cannot be redefined once they exist, but it is possible to remove
an existing function or procedure with the "drop" command where *procedure* is the name of
the procedure to be removed, and *function* is the name of the function to be removed.

variable arguments
------------------

Function and procedure definitions can use an ellipsis "..." as last parameter in the
parameter list. The ellipsis allows to add zero or more optional parameters to the
function. To retrieve the optional parameters the special keyword "args" is defined with
the following functions:

        args{?}                     retrieve number of optional parameters
        args{ <expr> }              retrieve value of parameter <expr>
        args{ <expr> }[]?           true if parameter <expr> is an array, false otherwise
        args{ <expr> }[?]           retrieve array size of parameter <expr>
        args{ <expr1> }[ <expr2> ]  retrieve array index <expr2> of parameter <exp1>

The keyword "args" can be used in the toplevel scope of a mempeek script to retrieve
parameters which were defined on the mempeek command line using the option -a.

strings
-------

Mempeek can treat arrays as strings. The content of the array is treated as contiguous
memory and the content is a zero terminated ASCII string. The length of the string is the
number of bytes until the first zero byte and is not connected to the size of the array.
When mempeek creates arrays to hold strings, the array size is chosen to be the smallest
size possible to hold the string, padded with zero bytes at the end if necessary. String
literals can be used in assignments or as subroutine arguments instead of arrays. The
following functions are available for string handling:

        strlen( str[] )                     return the string length in bytes
        strcmp( str1[], str2[] )            compare strings lexicographically and return 0 / 1 / 2
                                            when str1 is before / is equal to / is after str2
        str2int( str[] )                    convert the string to an integer
        str2float( str[] )                  convert the string to a float
        tokenize( str[] [, delimiter[]] )   split up str into tokens using the optional
                                            regexp delimiter (default is whitespace), and
                                            return the number of tokens

        strcat( str1[], str2[] )            concatenate the two strings
        substr( str[], pos, len )           return the substring starting at byte pos
                                            and length len
        getline()                           wait for console input and return entered
                                            text until return is pressed
        gettoken( i )                       return token i after calling tokenize()
        int2str( value )                    convert value to an unsigned decimal string
        signed2str( value )                 convert value to a signed decimal string
        hex2str( value [, digits] )         convert value to a hexadecimal string
        bin2str( value [, digits] )         convert value to a binary string
        float2str( value )                  convert value to a floating point string

floating point numbers
----------------------

Floating point calculations in mempeek can be done using a set of builtin functions which
interpret their arguments as IEEE 754 double precision floating point values. The following
arithmetic and conversion functions are available:

        int2float( i )      convert the integer i to a floating point number
        float2int( f )      convert the floating point number f to an integer
        fadd( a, b )        calculate a + b
        fsub( a, b )        calculate a - b
        fmul( a, b )        calculate a * b
        fdiv( a, b )        calculate a / b
        fsqrt( a )          calculate square root of a
        fpow( a, b )        calculate a ^ b
        flog( a )           calculate natural logarithm of a
        fexp( a )           calculate e ^ a
        fsin( a )           calculate sin( a )
        fcos( a )           calculate cos( a )
        ftan( a )           calculate tan( a )
        fasin( a )          calculate arcsin( a )
        facos( a )          calculate arccos( a )
        fatan( a )          calculate arctan( a )
        fabs( a )           calculate | a |
        ffloor( a )         returns largest integer not greater than a
        fceil( a )          returns smallest integer not less than a
        fround( a )         rounds a to the nearest integer, away from zero

IEEE 754 encoding preserves the ordering of floating point values when treated as integer
values in two's complement. Therefore the signed integer comparison operators can be used
to compare floating point values.

other commands
--------------

        import "file"
        run "file"

Execute the content of "file". All mappings, variable definitions, and assignments are
imported in the current scope. An exit command in the file stops execution and returns to
the current scope. The difference between import and run is that import executes the file
only once when the import statement occurs several times, whereas run does always execute
the file even if it was called before. Import decides wether the file was already executed
or not based on the MD5 hash of the file.

        sleep <time>
        sleep until <time>
        now

The first command suspends execution for *time* microseconds. The second command waits
until a fixed point in time. The keyword "now" can be used in expressions to get the
current point in time in microseconds since a fixed reference time in the past. This
value can be used as argument for the sleep until command.

        quit

Terminate a program

compiler options
----------------

        pragma loadpath "path"
        pragma wordsize (16 | 32 | 64)
        pragma print <modifier>

The first command adds *path* to the search path which is used to find files in the
import and run commands. The second command changes the default wordsize for the print
command. The third command changes the default modifier for the print command. *modifier*
is any modifier that is allowed in the print command.

*wordsize* and *print* apply only to the file in which they are used and to files which
are imported from that file. The defaults of files which import a file with these pragmas
are not changed.

comments, whitespace, newline
-----------------------------

When a line contains a '#', the rest of the line is ignored. An arbitrary number of tab or
space characters can be put between language keywords. Commands are completed by a newline
character or by a ";" when more than one command is used in a single line.
