#include "parser.hpp"

#include "mempeek_ast.h"

#include <iostream>

using namespace std;


int main()
{
#if defined( YYDEBUG ) && YYDEBUG != 0
    yydebug = 1;
#endif

    yyparse();

    if( yyroot ) {
    	cout << "executing AST[" << yyroot << "]" << endl;
    	yyroot->execute();
    }

    return 0;
}
