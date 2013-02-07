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
class wxConsoleContent;
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

	/** Set a character at the specified position (console coordinates). */
	void SetChar(wxChar c);

	
	/** Recompute scroll bar states (size and pos) from console size and historic position and size.*/ 
	void UpdateScrollBars();

	/** Move the caret to the specified position. */
	void SetCaretPosition(int x, int y){SetCaretPosition(wxPoint(x, y));}
	/**
	 * Move the caret (console textual cursor) to the specified position.
     * \param pos Caret absolute position in console coordinates (visible, in chars).
	 */
	void SetCaretPosition(wxPoint pos);
	/** Move the caret of specified offset (in chars). */
	void MoveCaret(int x=0, int y=0);

	/** Retrieve the caret (console textual cursor) position. */
	const wxPoint& GetCaretPosition()const{return m_caretPos;}
	/** Retrieve the caret (console textual cursor) position in historic coordinates. */
	wxPoint GetCaretPosInBuffer()const;

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

	void GenerateFonts(const wxFont& font);
	
	void OnPaint(wxPaintEvent& event);
	void OnSize(wxSizeEvent& event);
	void OnScroll(wxScrollWinEvent& event);
	void OnChar(wxKeyEvent& event);
	void OnTimer(wxTimerEvent& event);
	
	wxSize m_clSizeChar; // Size of console in chars
	wxCaret* m_caret;    // Caret pseudo-widget instance.
	wxPoint  m_caretPos; // Position of caret (console cursor) in chars

	wxFont m_defaultFont, m_boldFont, m_underlineFont, m_boldUnderlineFont;
	wxColour m_colours[8];
	unsigned char  m_lastBackColor, m_lastForeColor, m_lastStyle;
	
	wxTimer* m_timer;    // Timer for i/o treatments.
	wxOutputStream* m_outputStream;
	wxInputStream*  m_inputStream;
	wxInputStream*  m_errorStream;
};

#endif // _TERMINAL_CTRL_HPP_
