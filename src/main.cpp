#include "parser.hpp"

int main()
{
#if defined( YYDEBUG ) && YYDEBUG != 0
    yydebug = 1;
#endif

    yyparse();
    return 0;
}
