/*  Copyright (c) 2015, Martin Hammel
    All rights reserved.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright notice, this
      list of conditions and the following disclaimer.

    * Redistributions in binary form must reproduce the above copyright notice,
      this list of conditions and the following disclaimer in the documentation
      and/or other materials provided with the distribution.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
    AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
    DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
    FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
    DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
    SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
    CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
    OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
    OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "mempeek_ast.h"
#include "mempeek_exceptions.h"
#include "console.h"
#include "teestream.h"
#include "version.h"

#if defined( YYDEBUG ) && YYDEBUG != 0
#include "mempeek_parser.h"
#include "parser.h"
#endif

#include <iostream>
#include <fstream>

#include <string.h>
#include <signal.h>

using namespace std;


static void signal_handler( int )
{
    Environment::set_terminate();
}

static void parse( Environment* env, const char* str, bool is_file )
{
    Environment::clear_terminate();
    signal( SIGABRT, signal_handler );
    signal( SIGINT, signal_handler );
    signal( SIGTERM, signal_handler );

    try {
        ASTNode::ptr yyroot = env->parse( str, is_file );

#ifdef ASTDEBUG
		cerr << "executing ASTNode[" << yyroot << "]" << endl;
#endif
		yyroot->execute();
    }
    catch( ASTExceptionExit& ) {
        // nothing to do
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

static void print_usage( const char* name )
{
    cout << "Usage: " << name << " [options] [script] ...\n"
            "\n"
            "Options:\n"
            "    -i          Enter interactive mode when all scripts and commands are completed\n"
            "    -I <path>   Add <path> to the search path of the \"import\" command\n"
            "    -c <stmt>   Execute the mempeek command <stmt>\n"
            "    -l <file>   Write output and interactive input to <file>\n"
            "    -ll <file>  Append output and interactive input to <file>\n"
            "    -v          Print version\n"
            "    -h          Print usage\n"
         << flush;
}

static unsigned char completion( EditLine* el, int ch )
{
    // init
    const LineInfo* li = el_line( el );
    string line( li->buffer, li->cursor );

    // retrieve environment
    Environment* env;
    el_get( el, EL_CLIENTDATA, &env );

    // find current token
    size_t pos = line.rfind( ' ' );
    if( pos == string::npos ) pos = 0;
    else pos++;

    size_t bpos = line.rfind( '[' );
    if( bpos != string::npos && ++bpos > pos ) pos = bpos;

    // get all matching variables
    string prefix = line.substr( pos, string::npos );
    set< string > completion = env->get_autocompletion( prefix );

    if( completion.size() == 0 ) return CC_NORM;

    // find common prefix of all matches
    string first = *completion.begin();
    string last = *completion.rbegin();
    string common = "";

    for( size_t i = prefix.length(); i < first.length() && i < last.length(); i++ ) {
        if( first[i] != last[i] ) break;
        common += first[i];
    }

    // append completion to command line
    bool completion_error = false;
    if( completion.size() == 1 ) common += ' ';
    if( common.length() > 0 ) {
        if( el_insertstr( el, common.c_str() ) != 0 ) completion_error = true;
    }

    // show all matches
    if( completion.size() > 1 ) {
        int cnt = 0;
        for( auto var: completion ) {
            cout << (++cnt > 1 ? ' ' : '\n') << var;
        }
        cout << endl;
    }

    if( completion_error ) return CC_ERROR;
    if( completion.size() == 1 ) return CC_REFRESH;
    else return CC_REDISPLAY;
}

int main( int argc, char** argv )
{
#if defined( YYDEBUG ) && YYDEBUG != 0
    yydebug = 1;
#endif

    MMap::enable_signal_handler();

    ofstream* logfile = nullptr;
    basic_teebuf< char >* cout_buf = nullptr;
    basic_teebuf< char >* cerr_buf = nullptr;

    Environment env;

    try {
        bool is_interactive = false;
        bool has_commands = false;

        for( int i = 1; i < argc; i++ )
        {
            if( strcmp( argv[i], "-v" ) == 0 ) {
                print_release_info();
                throw ASTExceptionQuit();
            }
            else if( strcmp( argv[i], "-h" ) == 0 ) {
                print_usage( argv[0] );
                throw ASTExceptionQuit();
            }
            else if( strcmp( argv[i], "-i" ) == 0 ) is_interactive = true;
            else if( strcmp( argv[i], "-I" ) == 0 ) {
                if( ++i >= argc ) {
                    cerr << "missing include path" << endl;
                    throw ASTExceptionQuit();
                }

                env.add_include_path( argv[i] );
            }
            else if( strcmp( argv[i], "-c" ) == 0 ) {
                if( ++i >= argc ) {
                    cerr << "missing command" << endl;
                    throw ASTExceptionQuit();
                }
                // TODO: parser should treat EOF as end of statement
                string cmd = string( argv[i] ) + '\n';

                parse( &env, cmd.c_str(), false );
                has_commands = true;
            }
            else if( strcmp( argv[i], "-l" ) == 0 || strcmp( argv[i], "-ll" ) == 0 ) {
                if( logfile ) {
                    cerr << "duplicate logfile option" << endl;
                    throw ASTExceptionQuit();
                }
                if( ++i >= argc ) {
                    cerr << "missing file name" << endl;
                    throw ASTExceptionQuit();
                }

                if( strcmp( argv[ i - 1 ], "-l" ) == 0 ) logfile = new ofstream( argv[i] );
                else logfile = new ofstream( argv[i], ios::app );

                cout_buf = new basic_teebuf<char>;
                cout_buf->attach( cout.rdbuf() );
                cout_buf->attach( logfile->rdbuf() );
                cout.rdbuf( cout_buf );

                cerr_buf = new basic_teebuf<char>;
                cerr_buf->attach( cerr.rdbuf() );
                cerr_buf->attach( logfile->rdbuf() );
                cerr.rdbuf( cerr_buf );
            }
            else {
                parse( &env, argv[i], true );
                has_commands = true;
            }
        }

        if( is_interactive || !has_commands ) {
            Console console( "mempeek", "~/.mempeek_history" );
            console.set_clientdata( &env );
            console.set_completion( completion );
            if( !has_commands ) print_release_info();
            for(;;) {
                string line = console.get_line();
                if( logfile ) *logfile << "> " << line;
                parse( &env, line.c_str(), false );
            }
        }
    }
    catch( ASTExceptionQuit& ) {
        // nothing to do
    }

    if( logfile ) {
        cout_buf->detach( logfile->rdbuf() );
        cerr_buf->detach( logfile->rdbuf() );
        delete logfile;
        // teebuffers are not deleted because cout/cerr still use them
    }

    return 0;
}
