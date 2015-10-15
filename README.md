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

The following commands are available in the mempeek language:

assigning values to definition variables
----------------------------------------

        def <name> <expression>

Assign the result of evaluating *expression* to the definition variable *name*

        def <base>.<name> <expression>

The definition variable *base.name* is assigned the value *base* + *expression*

        def <base> <expression> from <name>

Clone all derived definition variables name.x to base.x and assign them to the new base
value *expression*

These operations are not allowed within for loops, while loops, and if statements.

assigning values to variables
-----------------------------

        name := expression

Assign the result of evaluating *expression* to the variable *name*. It is not allowed to
assign values to a definition variable.

All variables or definition variables are internally stored as unsigned 64 bit values. The
print, peek, and poke commands can be restricted to operate on 8, 16, and 32 bit values,
the default size is the native word size of the system. When a restriction occurs, the
upper bits are cut off (poke and print) or set to zero (peek).

Numerical values can be specified as decimal numbers, hexadecimal numbers starting with the
prefix "0x" or binary number starting with the prefix "0b".

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

Bit manipulation operators:

        <expr1> << <expr2>      left shift <expr1> by <expr2> bits
        <expr1> >> <expr2>      right shift <expr1> by <expr2> bits
        <expr1> & <expr2>       bitwise AND of <expr1> and <expr2>
        <expr1> | <expr2>       bitwise OR of <expr1> and <expr2>
        <expr1> ^ <expr2>       bitwise XOR of <expr1> and <expr2>
        ~ <expr>                bitwise NOT of <expr>

Boolean operators (expressions are treated as true or false by these operators, a value of
zero is false, all other values are true. The result of logical operators is 0 for false
and -1 for true ):

        <expr1> && <expr2>      logical AND of <expr1> and <expr2>
        <expr1> || <expr2>      logical OR of <expr1> and <expr2>
        <expr1> ^^ <expr2>      logical XOR of <expr1> and <expr2>
        ! <expr>                logical NOT of <expr>

Comparison operators (comparisons evaluate to the values 0 for false and -1 for true):

        <expr1> < <expr2>       true if <expr> is lower than <expr2>
        <expr1> > <expr2>       true if <expr> is greater than <expr2>
        <expr1> <= <expr2>      true if <expr> is lower than or equal to <expr2>
        <expr1> >= <expr2>      true if <expr> is greater than or equal to <expr2>
        <expr1> == <expr2>      true if <expr> is equal to <expr2>
        <expr1> != <expr2>      true if <expr> is not equal to <expr2>

index operator (can be applied on variables or definition variables):

        <name>[<expression>]    evaluates to value of <name> + result of <expression>

**WARNING** This feature is deprecated. The usage of the index operator will very likely
change in the future to allow the implementation of arrays.

operator precedence
-------------------

the operators have the following precedences, with higher precedence listed first:

        1:  ! ~ - (unary)
        2:  * / % &
        3:  + - | ^
        4:  << >>
        5:  < <= > >= == !=
        6:  &&
        7:  || ^^

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

        print [<modifier>] (<expression> | "string") ... [noendl]

Print the result of evaluating *expression* to the console, or print a string. When several
expressions or strings are given, they are concatenated into one line. The modifiers can be
"hex", "dec", "bin" or "neg". When a modifier is given, all subsequent expression results
are printed as hexadecimal, unsigned decimal, binary or signed decimal number. Each
modifier can be used with an optional suffix ":8", ":16", ":32" or ":64" which specifies
the number of bits of the result to be displayed. Several modifiers can be used within the
same print command. The default modifier for each print command is "hex" with the default
bit size of the system. The print command does add a newline at the end of an output line.
The last parameter of the print command can be the modifier "noendl" which suppresses the
newline.

mapping physical memory
-----------------------

        map <address> <size> ["device"]

Map *size* bytes of physical memory at address *address* to the process memory space. When
"device" is given, the mapping occurs on that device, default is /dev/mem. *address* and
*size* must be constants, dynamic calculation of address spaces is not allowed. This
operation is not allowed within for loops, while loops, and if statements

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

other commands
--------------

        import "file"

Execute the content of "file". All mappings, variable definitions, and assignments are
imported in the current scope. A break command in the imported file file stop execution and
return to the current scope.

        sleep <time>

Suspend execution for *time* microseconds

        quit

Terminate a program


comments, whitespace, newline
-----------------------------

When a line contains a '#', the rest of the line is ignored. An arbitrary number of tab or
space characters can be put between language keywords. Commands are completed by a newline
character or by a ";" when more than one command is used in a single line.
