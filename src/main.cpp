#include "mempeek_parser.h"

#include "parser.h"
#include "lexer.h"

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

static void parse( const char* str, bool is_file )
{
	yyscan_t scanner;
    yylex_init( &scanner );

    YY_BUFFER_STATE lex_buffer;
    FILE* file;
    if( is_file ) {
        file = fopen( str, "r" );
    	lex_buffer = yy_create_buffer( file, YY_BUF_SIZE, scanner );
    }
    else lex_buffer = yy_scan_string( str, scanner );
    yy_switch_to_buffer( lex_buffer, scanner );

    ASTNode* yyroot = nullptr;

    ASTNode::clear_terminate();
    signal( SIGABRT, signal_handler );
    signal( SIGINT, signal_handler );
    signal( SIGTERM, signal_handler );

    try {
        yyparse( scanner, &yyroot );

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

    if( yyroot ) delete yyroot;

    yy_delete_buffer( lex_buffer, scanner );
    yylex_destroy( scanner );

    if( is_file  ) fclose( file );
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

                parse( cmd.c_str(), false );
            }
            else parse( argv[i], true );
        }

        if( is_interactive ) {
            for(;;) {
                string line = console.get_line();
                parse( line.c_str(), false );
            }
        }
    }
    catch( ASTExceptionQuit ) {
        // nothing to do
    }

    return 0;
}
