/*  Copyright (c) 2015-2018, Martin Hammel
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

#ifndef __console_h__
#define __console_h__

#include <string>
#include <vector>
#include <functional>

#ifdef USE_EDITLINE
#include <histedit.h>
#else
typedef void EditLine;
#endif


//////////////////////////////////////////////////////////////////////////////
// class Console
//////////////////////////////////////////////////////////////////////////////

class Console {
public:
	Console( std::string name, size_t histsize = 100 );
	Console( std::string name, std::string histfile, size_t histsize = 100 );
	~Console();

	void set_prompt( std::string prompt );
	void set_completion( std::function<unsigned char( EditLine*, int )> completion );
	void set_clientdata( void* data );

	std::string get_line();

	typedef std::vector< std::string > tokens_t;
	tokens_t get_tokens();

private:
    static char* s_Prompt;

#ifdef USE_EDITLINE

	std::string m_Histfile;

	History* m_History;
	EditLine* m_Editline;

    static std::function<unsigned char( EditLine*, int )> s_Completion;

    static char* get_prompt( EditLine* );
    static unsigned char completion_callback( EditLine* el, int ch );

#endif
};


//////////////////////////////////////////////////////////////////////////////
// class Console inline functions
//////////////////////////////////////////////////////////////////////////////

inline Console::Console( std::string name, size_t histsize )
 : Console( name, "", histsize )
{}


#endif // __console_h__
