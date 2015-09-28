#ifndef __console_h__
#define __console_h__

#include <string>
#include <vector>
#include <functional>

#include <histedit.h>


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

	std::string get_line();

	typedef std::vector< std::string > tokens_t;
	tokens_t get_tokens();

private:
	void init( std::string name, std::string histfile, size_t histsize );

	static char* get_prompt( EditLine* );
	static unsigned char completion_callback( EditLine* el, int ch );

	std::string m_Histfile;

	static char* s_Prompt;
	static std::function<unsigned char( EditLine*, int )> s_Completion;

	History* m_History;
	EditLine* m_Editline;
};


//////////////////////////////////////////////////////////////////////////////
// class Console inline functions
//////////////////////////////////////////////////////////////////////////////

inline Console::Console( std::string name, size_t histsize )
{
	init( name, "", histsize );
}


inline Console::Console( std::string name, std::string histfile, size_t histsize )
{
	init( name, histfile, histsize );
}


#endif // __console_h__
