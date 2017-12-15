// Page.h

#ifndef _PAGE_h
#define _PAGE_h

#if defined(ARDUINO) && ARDUINO >= 100
	#include "arduino.h"
#else
	#include "WProgram.h"
#endif

class PageClass /*: protected String*/{
	protected:
		String _style;
		String _script;
		String _body;
		String _page;
		String _onload;

	public:
		PageClass();
		PageClass(String);
		~PageClass();
		void appendStyle(String);	
		void appendScript(String);
		void appendBody(String);
		
		String go();
};


#endif







