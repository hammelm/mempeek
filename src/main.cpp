#include "lexer.h"
#include "parser.h"

#include "mempeek_ast.h"
#include "console.h"

#include <iostream>


using namespace std;


int main()
{
#if defined( YYDEBUG ) && YYDEBUG != 0
    yydebug = 1;
#endif

    Console console( "mempeek", "~/.mempeek_history" );

    yyroot = nullptr;

    for(;;) {
        string line = console.get_line();

        YY_BUFFER_STATE lex_buffer = yy_scan_string( line.c_str() );
        yy_switch_to_buffer( lex_buffer );

        yyparse();

        if( yyroot ) {
#ifdef ASTDEBUG
    	    cout << "executing ASTNode[" << yyroot << "]" << endl;
#endif
            try {
                yyroot->execute();
            }
            catch( ASTExceptionBreak& ) {
                // ignored in interactive mode
            }
            catch( ASTExceptionQuit& ex ) {
                return 0;
            }


            delete yyroot;
            yyroot = nullptr;
        }

        yy_delete_buffer( lex_buffer );
    }

    return 0;
}
