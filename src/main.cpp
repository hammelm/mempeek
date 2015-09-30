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
#ifdef ASTDEBUG
    	cout << "executing ASTNode[" << yyroot << "]" << endl;
#endif
    	yyroot->execute();
    }

    return 0;
}
