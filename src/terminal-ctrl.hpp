/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * wxTerminal
 * Copyright (C) Emilien Kia 2012 <emilien.kia@free.fr>
 * 
 * wxTerminal is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * wxTerminal is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _TERMINAL_CTRL_HPP_
#define _TERMINAL_CTRL_HPP_

#include <vector>
#include <list>
#include <set>

#include "terminal-parser.hpp"

extern wxString wxTerminalCtrlNameStr;

struct wxTerminalCharacter;
class wxTerminalContent;
class wxTerminalCtrl;


enum wxTerminalCharacterStyle
{
	wxTCS_Normal     = 0,
	
	wxTCS_Bold       = 1,
	wxTCS_Underlined = 2,
	wxTCS_Blink      = 4,
	wxTCS_Inverse    = 8,
	wxTCS_Invisible  = 16,

	//wxTCS_Selected   = 128 // wxTerminal specific
};


struct wxTerminalCharacter
{
	unsigned char fore;
	unsigned char back;
	unsigned char style;
	wxChar c;
};


typedef std::vector<wxTerminalCharacter> wxTerminalLine;

/**
 * Represent the content of a text console.
 * It is the base class for consoles with or without historic.
 * It is a vector of terminal lines. 
 */
class wxConsoleContent: public std::vector<wxTerminalLine>
{
public:
	/**
	 * Set a char at the specified position.
	 */
	void setChar(wxPoint pos, wxTerminalCharacter c);
	/**
	 * Set a char at the specified position.
	 */
	virtual void setChar(wxPoint pos, wxChar c, unsigned char fore, unsigned char back, unsigned char style);

	/**
	 * Add an empty new line
	 */
	virtual void addNewLine();
	
	/**
	 * Ensure that the specified line is available in history, create it if needed.
	 */
	virtual void ensureHasLine(size_t l);

	/**
	 * Ensure that the specified character is available in history, create it if needed.
	 */
	virtual void ensureHasChar(size_t l, size_t c);
	
protected:

};





class wxTerminalContent: public std::vector<wxTerminalLine>
{
public:
	wxTerminalContent(size_t maxLineSize=0, size_t maxLineCount=0);
	~wxTerminalContent();

	void setMaxLineSize(size_t maxLinesize);
	void setMaxLineCount(size_t maxLineCount);

	size_t getLineCount()const{return size();}
	size_t getMaxLineSize()const{return m_maxLineSize;}
	size_t getMaxLineCount()const{return m_maxLineCount;}
	
	size_t isLineInfinite()const{return m_maxLineSize==0;}

	void setChar(wxPoint pos, wxChar c, char fore, char back, unsigned char style);
	void setChar(wxPoint pos, wxTerminalCharacter c);
	
	void append(wxChar c, char fore, char back, unsigned char style);
	void append(const wxTerminalCharacter& c);

	void addNewLine();

protected:	
	void removeExtraLines();
	
private:
	size_t m_maxLineSize;
	size_t m_maxLineCount;

	bool m_deleteOnNoListener;
};



class wxTerminalCtrl: public wxScrolledCanvas, protected TerminalParser
{
	wxDECLARE_EVENT_TABLE();
public:
    wxTerminalCtrl(wxWindow *parent, wxWindowID id, const wxPoint &pos=wxDefaultPosition,
        const wxSize &size=wxDefaultSize, long style=0, const wxString &name=wxTerminalCtrlNameStr);
    wxTerminalCtrl();
	virtual ~wxTerminalCtrl();
    virtual bool Create(wxWindow *parent, wxWindowID id, const wxPoint &pos=wxDefaultPosition,
        const wxSize &size=wxDefaultSize, long style=0, const wxString &name=wxTerminalCtrlNameStr);

	wxSize GetClientSizeInChars()const{return m_clSizeChar;}

	// Output stream where send user input.
	wxOutputStream* GetOutputStream()const{return m_outputStream;}
	void SetOutputStream(wxOutputStream* out){m_outputStream = out;}

	// Input stream from where get shown data.
	void SetInputStream(wxInputStream* in){m_inputStream = in;}
	wxInputStream* GetInputStream()const{return m_inputStream;}

	// Error stream from where get shown data.
	void SetErrorStream(wxInputStream* err){m_errorStream = err;}
	wxInputStream* GetErrorStream()const{return m_errorStream;}

	
protected:
	void CommonInit();
	wxSize GetCharSize(wxChar c = wxT('0'))const;

	void Append(const unsigned char* buff, size_t sz);

	void UpdateScrollBars();

	void SetCaretPosition(int x, int y){SetCaretPosition(wxPoint(x, y));}
	void SetCaretPosition(wxPoint pos);
	void MoveCaret(int x=0, int y=0);
	const wxPoint& GetCaretPosition()const{return m_caretPos;}
	wxPoint GetCaretPosInBuffer()const;

	wxSize m_clSizeChar;

	void GenerateFonts(const wxFont& font);


	/**
	 * Declaration of TerminalParser interface abstract functions
	 * \{ */
	/*overriden*/ void onPrintableChar(unsigned char c);
	/*overriden*/ void onBEL();  // 0x07
	/*overriden*/ void onBS();   // 0x08
	/*overriden*/ void onLF();   // 0x0A
	/*overriden*/ void onCR();   // 0x0D
	/*overriden*/ void onCHA(unsigned short nb);
	/*overriden*/ void onCUP(unsigned short row, unsigned short col);
	/*overriden*/ void onED(unsigned short opt);
	/*overriden*/ void onEL(unsigned short opt);
	/*overriden*/ void onSGR(const std::vector<unsigned short> nbs);
	/*overriden*/ void onOSC(unsigned short command, const std::vector<unsigned char>& params);
	/** \} */
	
private:
	wxConsoleContent *m_historicContent;
	wxConsoleContent *m_staticContent;
	wxConsoleContent *m_currentContent;
	bool m_isHistoric;
	
	
	
	void SetChar(wxChar c);
	
	void OnPaint(wxPaintEvent& event);
	void OnSize(wxSizeEvent& event);
	void OnScroll(wxScrollWinEvent& event);
	void OnChar(wxKeyEvent& event);
	void OnTimer(wxTimerEvent& event);

	wxFont m_defaultFont, m_boldFont, m_underlineFont, m_boldUnderlineFont;

	wxTimer* m_timer;
	wxCaret* m_caret;
	wxPoint  m_caretPos;

	wxColour m_colours[8];
	unsigned char  m_lastBackColor, m_lastForeColor, m_lastStyle;
	
	wxOutputStream* m_outputStream;
	wxInputStream*  m_inputStream;
	wxInputStream*  m_errorStream;
};

#endif // _TERMINAL_CTRL_HPP_
