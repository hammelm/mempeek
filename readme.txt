assign values to definition variables:

    def <name> <expression>
    
        Assign the result of evaluating <expression> to the definition variable <name>

    def <base>.<name> <expression>
    
        The definition variable base.name is assigned the value base + <expression>

    def <base> <expression> from <name>
    
        Clone all derived definition variables name.x to base.x and assign them
        to the new base value <expression>
        This operation is not allowed within for loops, while loops, and if statements


assign values to variables:

    name := expression
        Assign the result of evaluating <expression> to the variable <name>
        It is not allowed to assign values to a definition variable

All variables or definition variables are internally stored as unsigned 64 bit values.
The print, peek, and poke commands can be restricted to operate on 8, 16, and 32 bit
values, the default size is the native word size of the system. When a restriction occurs,
the upper bits are cut off (poke and print) or set to zero (peek).

expressions:

    Expressions consist of variables or definition variables, arithmetic expressions,
    logical expressions, or comparisons:
    
    Arithmetic operators:
    
        <expr1> + <expr2>       add <expr1> to <expr2>
        <expr1> - <expr2>       subtract <expr2> from <expr1>
        <expr1> * <expr2>       multiply <expr1> with <expr2>
        <expr1> / <expr2>       divide <expr1> by <expr2>
        - <expr>                negate <expr> (calculate two's complement)
        
    Bit manipulation operators:
    
        <expr1> << <expr2>      left shift <expr1> by <expr2> bits 
        <expr1> >> <expr2>      right shift <expr1> by <expr2> bits 
        <expr1> & <expr2>       bitwise AND of <expr1> and <expr2>
        <expr1> | <expr2>       bitwise OR of <expr1> and <expr2>
        <expr1> ^ <expr2>       bitwise XOR of <expr1> and <expr2>
        ~ <expr>                bitwise NOT of <expr>
        
    Boolean operators (expressions are treated as true or false by these operators,
    a value of zero is false, all other values are true. The result of logical
    operators is 0 for false) and -1 for true ): 
    
        <expr1> && <expr2>      logical AND of <expr1> and <expr2>
        <expr1> || <expr2>      logical OR of <expr1> and <expr2>
        <expr1> ^^ <expr2>      logical XOR of <expr1> and <expr2>
        ! <expr>                logical NOT of <expr>
        
    Comparison operators (comparisons evaluate to the values 0 for false and -1 for true):
    
        <expr1> < <expr2>       true if <expr> is lower than <expr2>
        <expr1> > <expr2>       true if <expr> is greater than <expr2>
        <expr1> <= <expr2>       true if <expr> is lower than or equal to <expr2>
        <expr1> >= <expr2>       true if <expr> is greater than or equal to <expr2>
        <expr1> == <expr2>       true if <expr> is equal to <expr2>
        <expr1> != <expr2>       true if <expr> is not equal to <expr2>
     
    index operator (can be applied on variables or definition variables)
    
        <name>[ <expression> ]  evaluates to value of <name> + result of <expression>


conditional execution:

    if <expr> then <command>
   [else <command>]
    
    if <expr> then
        <command>
        ...
   [else
        <command>
        ...]
    endif
    
        Evaluate <expr> and execute the command(s) after the "then" keyword if <expr> is true,
        or else execute the command(s) after the optional "else" keyword
        When the command is written behind "then" or "else" without a newline, only this single command is executed.
        When a newline is found, all commands until the next "else" or "endif" keyword are executed   


for loops:

    for <name> from <expr1> to <expr2> [step <expr3>] do <command>
    
    for <name> from <expr1> to <expr2> [step <expr3>] do
        <command>
        ...
        [break]
        ...
    endfor
    
        Create a variable "name" and assign it the value of <expr1>. Execute command(s) until the value
        of the variable is greater than <expr2>. After each iteration increase the variable by <expr3> when
        the "step" keyword is found, or else increase it by one. When the step value is a negative number,
        stop iterating when the variable is lower than <expr2>.
        When the command is written behind "do" without a newline, only this single command is executed.
        When a newline is found, all commands until the next "endfor" keyword are executed.
        When a "break" keyword is encountered within the loop, the loop is left immediately


while loops:

    while <expr> do <command>
    
    while <expr> do
        <command>
        ...
        [break]
        ...
    endwhile
    
        Execute command(s) until the value of <expr> evaluates to 0.
        When the command is written behind "do" without a newline, only this single command is executed.
        When a newline is found, all commands until the next "endwhile" keyword are executed.
        When a "break" keyword is encountered within the loop, the loop is left immediately
