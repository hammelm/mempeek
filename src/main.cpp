#include "lexer.h"
#include "parser.h"

#include "mempeek_ast.h"
#include "console.h"

#include <iostream>

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>

using namespace std;


static void signal_handler( int )
{
    ASTNode::set_terminate();
}

static void parse()
{
    ASTNode::clear_terminate();
    signal( SIGABRT, signal_handler );
    signal( SIGINT, signal_handler );
    signal( SIGTERM, signal_handler );

    try {
        yyparse();

        if( yyroot ) {
#ifdef ASTDEBUG
            cout << "executing ASTNode[" << yyroot << "]" << endl;
#endif
            yyroot->execute();
        }
    }
    catch( ASTExceptionBreak& ) {
        // nothing to do
    }
    catch( ASTExceptionTerminate& ) {
        cout << endl << "terminated execution" << endl;
    }
    catch( const ASTCompileException& ex ) {
        cerr << "compile error: " << ex.what() << endl;
    }
    catch( const ASTRuntimeException& ex ) {
        cerr << "runtime error: " << ex.what() << endl;
    }

    signal( SIGABRT, SIG_DFL );
    signal( SIGINT, SIG_DFL );
    signal( SIGTERM, SIG_DFL );

    if( yyroot ) {
        delete yyroot;
        yyroot = nullptr;
    }
}

int main( int argc, char** argv )
{
#if defined( YYDEBUG ) && YYDEBUG != 0
    yydebug = 1;
#endif

    Console console( "mempeek", "~/.mempeek_history" );

    MMap::enable_signal_handler();

    try {
        bool is_interactive = true;
        for( int i = 1; i < argc; i++ )
        {
            if( string(argv[i]) == "-c" ) {
                is_interactive = false;

                if( ++i >= argc ) {
                    cout << "missing command" << endl;
                    break;
                }

                // TODO: parser should treat EOF as end of statement
                string cmd = string( argv[i] ) + '\n';
                YY_BUFFER_STATE lex_buffer = yy_scan_string( cmd.c_str() );
                yy_switch_to_buffer( lex_buffer );
                parse();
                yy_delete_buffer( lex_buffer );
            }
            else {
                FILE* file = fopen( argv[i], "r" );

                YY_BUFFER_STATE lex_buffer = yy_create_buffer( file, YY_BUF_SIZE );
                yy_switch_to_buffer( lex_buffer );
                parse();
                yy_delete_buffer( lex_buffer );

                fclose( file );
            }
        }

        if( is_interactive ) {
            for(;;) {
                string line = console.get_line();

                YY_BUFFER_STATE lex_buffer = yy_scan_string( line.c_str() );
                yy_switch_to_buffer( lex_buffer );
                parse();
                yy_delete_buffer( lex_buffer );
            }
        }
    }
    catch( ASTExceptionQuit ) {
        // nothing to do
    }

    return 0;
}
