#include "mempeek_ast.h"
#include "mempeek_exceptions.h"
#include "console.h"

#if defined( YYDEBUG ) && YYDEBUG != 0
#include "mempeek_parser.h"
#include "parser.h"
#endif

#include <iostream>

#include <string.h>
#include <signal.h>

using namespace std;


static void signal_handler( int )
{
    ASTNode::set_terminate();
}

static void parse( const char* str, bool is_file )
{
    ASTNode::clear_terminate();
    signal( SIGABRT, signal_handler );
    signal( SIGINT, signal_handler );
    signal( SIGTERM, signal_handler );

    try {
        ASTNode* yyroot = ASTNode::parse( str, is_file );

#ifdef ASTDEBUG
		cout << "executing ASTNode[" << yyroot << "]" << endl;
#endif
		yyroot->execute();
		delete yyroot;
    }
    catch( ASTExceptionBreak& ) {
        // nothing to do
    }
    catch( ASTExceptionTerminate& ) {
        cout << endl << "terminated execution" << endl;
    }
    catch( const ASTCompileException& ex ) {
        cerr << ex.get_location() << "compile error: " << ex.what() << endl;
    }
    catch( const ASTRuntimeException& ex ) {
        cerr << ex.get_location() << "runtime error: " << ex.what() << endl;
    }
    catch( ... ) {
        signal( SIGABRT, SIG_DFL );
        signal( SIGINT, SIG_DFL );
        signal( SIGTERM, SIG_DFL );

        throw;
    }

    signal( SIGABRT, SIG_DFL );
    signal( SIGINT, SIG_DFL );
    signal( SIGTERM, SIG_DFL );
}

int main( int argc, char** argv )
{
#if defined( YYDEBUG ) && YYDEBUG != 0
    yydebug = 1;
#endif

    Console console( "mempeek", "~/.mempeek_history" );

    MMap::enable_signal_handler();

    try {
        bool is_interactive = false;
        bool has_commands = false;

        for( int i = 1; i < argc; i++ )
        {
            if( strcmp( argv[i], "-i" ) == 0 ) is_interactive = true;
            else if( strcmp( argv[i], "-I" ) == 0 ) {
                if( ++i >= argc ) {
                    cerr << "missing include path" << endl;
                    break;
                }

                ASTNode::add_include_path( argv[i] );
            }
            else if( strcmp( argv[i], "-c" ) == 0 ) {
                if( ++i >= argc ) {
                    cerr << "missing command" << endl;
                    break;
                }
                // TODO: parser should treat EOF as end of statement
                string cmd = string( argv[i] ) + '\n';

                parse( cmd.c_str(), false );
                has_commands = true;
            }
            else {
                parse( argv[i], true );
                has_commands = true;
            }
        }

        if( is_interactive || !has_commands ) {
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
