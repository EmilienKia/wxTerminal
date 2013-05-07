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

/**
 * Character presentational style.
 */
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

/**
 * Character presentational attributes.
 */
struct wxTerminalCharacterAttributes
{
	unsigned char fore;
	unsigned char back;
	unsigned char style; // From wxTerminalCharacterStyle
};

/**
 * Represent a terminal character:
 * an unicode character with its presentational attributes.
 */
struct wxTerminalCharacter
{
	wxUniChar c;
	wxTerminalCharacterAttributes attr;

	static wxTerminalCharacter DefaultCharacter;
};

/**
 * A line of characters.
 */
typedef std::vector<wxTerminalCharacter> wxTerminalLine;

/**
 * Represent the content of a terminal.
 * It is a vector of terminal lines withoutknowledge of scrolling.
 * It just verify that the slots are available.
 * It doesnt do any character validation.
 */
class wxTerminalContent: public std::vector<wxTerminalLine>
{
public:
	/**
	 * Set a char at the specified position.
	 */
	void setChar(wxPoint pos, wxTerminalCharacter c);

	/**
	 * Insert a char at the specified position.
	 */
	void insertChar(wxPoint pos, wxTerminalCharacter c);

	/**
	 * Add an empty new line
	 */
	void addNewLine();

	/**
	 * Ensure that the specified line is available in history, create it if needed.
	 */
	void ensureHasLine(size_t l);

	/**
	 * Ensure that the specified character is available in history, create it if needed.
	 */
	void ensureHasChar(size_t l, size_t c);
};


/**
 * Represent a screen of a terminal.
 * Has the notion of cursor position and scrolling.
 */
class wxTerminalScreen
{
public:
	wxTerminalScreen();

	/** Clear the screen (and buffer). */
	void clear();

	/** Retrieve a line, from its screen position.*/
	wxTerminalLine& getLine(int line){ return _content[line+_originPosition.y]; }
	wxTerminalLine& operator[](int line){ return _content[line+_originPosition.y]; }
	const wxTerminalLine& getLine(int line)const{ return _content[line+_originPosition.y]; }
	const wxTerminalLine& operator[](int line)const{ return _content[line+_originPosition.y]; }

	/** Retrieve a line, from its absolute position.*/
	wxTerminalLine& getLineAbsolute(int line){ return _content[line]; }
	const wxTerminalLine& getLineAbsolute(int line)const{ return _content[line]; }

	/** Retrieve a char, from its screen position.*/
	wxTerminalCharacter& getChar(int line, int col){ return getLine(line)[col+_originPosition.x]; }
	/** Retrieve a char, from its screen position.*/
	const wxTerminalCharacter& getChar(int line, int col)const{ return getLine(line)[col+_originPosition.x]; }

	/** Retrieve the line of the caret.*/
	wxTerminalLine& getCurrentLine(){ return _content[_caretPosition.y]; }
	const wxTerminalLine& getCurrentLine()const{ return _content[_caretPosition.y]; }

	/** Retrieve the number of rows in content buffer.*/
	size_t getHistoryRowCount()const{return _content.size();}

	/** Retrieve the number of rows in screen (after origin in history).*/
	size_t getScreenRowCount()const{return _content.size() >= _originPosition.y ? _content.size() - _originPosition.y : 0;}

	/** Retrieve the caret (textual cursor) position in relative coordinates. */
	wxPoint getCaretPosition()const{return _caretPosition - _originPosition;}
	/** Retrieve the caret (textual cursor) position in absolute coordinates. */
	wxPoint getCaretAbsolutePosition()const{return _caretPosition;}
	/** Set the caret (textual cursor) position (in relative coordinates). */
	void setCaretPosition(wxPoint pos);
	/** Set the caret (textual cursor) position (in absolute coordinates). */
	void setCaretAbsolutePosition(wxPoint pos);
	
	/** Set a char at specified position, overriding existing if any. */
	void setChar(wxPoint pos, wxTerminalCharacter ch);
	/** Set a char at specified position, overriding existing if any. */
	void setChar(wxPoint pos, wxUniChar c, const wxTerminalCharacterAttributes& attr);
	/** Set a char at specified absolute position, overriding existing if any. */
	void setCharAbsolute(wxPoint pos, wxTerminalCharacter ch);
	/** Set a char at specified absolute position, overriding existing if any. */
	void setCharAbsolute(wxPoint pos, wxUniChar c, const wxTerminalCharacterAttributes& attr);

	/** Insert a char just before the specified position. */
	void insertChar(wxPoint pos, wxTerminalCharacter ch);
	/** Insert a char just before the specified position. */
	void insertChar(wxPoint pos, wxUniChar c, const wxTerminalCharacterAttributes& attr);
	/** Insert a char just before the specified absolute position. */
	void insertCharAbsolute(wxPoint pos, wxTerminalCharacter ch);
	/** Insert a char just before the specified absolute position. */
	void insertCharAbsolute(wxPoint pos, wxUniChar c, const wxTerminalCharacterAttributes& attr);

	/** Insert a char at caret position and move caret by one.*/
	void insertChar(wxUniChar c, const wxTerminalCharacterAttributes& attr);
	/** Overwrite a char at caret position and move caret by one.*/
	void overwriteChar(wxUniChar c, const wxTerminalCharacterAttributes& attr);

	/** Insert lines at specified position. */
	void insertLines(int pos, unsigned int count = 1);
	/** Insert lines at specified absolute position. */
	void insertLinesAbsolute(int pos, unsigned int count = 1);
	/** Insert lines at caret position. */
	void insertLinesAtCarret(unsigned int count = 1);
	/** Delete lines at specified position. */
	void deleteLines(int pos, unsigned int count = 1);
	/** Delete lines at specified absolute position. */
	void deleteLinesAbsolute(int pos, unsigned int count = 1);
	/** Delete lines at caret position. */
	void deleteLinesAtCarret(unsigned int count = 1);

	/** Retrieve origin coordinates.*/
	wxPoint getOrigin()const{return _originPosition;}
	/** Move origin in history. */
	void moveOrigin(int lines);
	/** Set origin in history. */
	void setOrigin(int lines);

	/** Retrieve the screen shown size (in chars). */
	wxSize getScreenSize()const{return _size;}
	/** Modify the screen size (in chars). */
	void setScreenSize(wxSize sz){_size = sz;}

	/** Move caret by specified cols and lines.*/
	void moveCaret(int lines, int cols);
	/** Move caret to specified col in current line. */
	void setCaretColumn(int col);
	
protected:
	/** Content of terminal screen, with potential history.*/
	wxTerminalContent _content;

	/** Position of origin (firstshown char of screen, top-left), in historic position. */  
	wxPoint _originPosition;

	/** Caret position, absolute coordinates (in buffer). */
	wxPoint _caretPosition;

	/** Screen shwon size (in chars). */
	wxSize _size;
	
};




enum wxTerminalCharacterSet
{
	wxTCSET_ISO_8859_1,
	wxTCSET_UTF_8
};


class wxTerminalCharacterDecoder
{
public:
	wxTerminalCharacterDecoder();

	void setCharacterSet(wxTerminalCharacterSet set);
	wxTerminalCharacterSet getCharacterSet()const{return _set;}

	bool add(unsigned char c, wxUniChar& ch);
		
protected:
	wxTerminalCharacterSet _set;
	unsigned long _buffer;

	enum wxTerminalCharacterDecoderState
	{
		wxTCDSTATE_DEFAULT,
		wxTCDSTATE_UTF8_2BYTES,
		wxTCDSTATE_UTF8_3BYTES_1,
		wxTCDSTATE_UTF8_3BYTES_2,
		wxTCDSTATE_UTF8_4BYTES_1,
		wxTCDSTATE_UTF8_4BYTES_2,
		wxTCDSTATE_UTF8_4BYTES_3		
	} _state;
};



class wxTerminalCharacterMap
{
public:
	struct wxTerminalCharacterMappping {
		unsigned char c;
		wxUniChar     u;
	};
	
	
	/** Simple character mapping.*/
	wxUniChar get(unsigned char c)const{return _chars[c];}

	static wxTerminalCharacterMap graphic, british, us, dutch, finnish, french, french_canadian, german, italian, norwegian,  spanish, swedish, swiss;
	static const wxTerminalCharacterMap* getMap(char id);

protected:
	wxTerminalCharacterMap();
	wxTerminalCharacterMap(const wxTerminalCharacterMap& map);
	wxTerminalCharacterMap(const wxTerminalCharacterMappping* mapping);

	void clear();
	void map(const wxTerminalCharacterMappping* mapping);
	
	wxUniChar _chars[256];
};


enum wxTerminalOptionFlags
{
	wxTOF_WRAPAROUND = 0,
	wxTOF_REVERSE_WRAPAROUND,
	wxTOF_ORIGINMODE,
	wxTOF_AUTO_CARRIAGE_RETURN,
	wxTOF_CURSOR_VISIBLE,
	wxTOF_CURSOR_BLINK,
	wxTOF_INSERT_MODE,
	wxTOF_REVERSE_VIDEO
};


struct wxTerminalState
{
	/** Cursor position. */
	wxPoint cursorPos;

	/** Character attributes. */
	wxTerminalCharacterAttributes textAttributes;

	/** G0...G3 character maps. */
	const wxTerminalCharacterMap* Gx[4];

	/** GL/GR character map index. Respectively 7-bit and 8-bit visible character set (values in 0...3). */
	unsigned short GL, GR;

	wxTerminalState();
	wxTerminalState(const wxTerminalState& state);
};


class wxTerminalCtrl: public wxWindow, protected TerminalParser
{
	wxDECLARE_EVENT_TABLE();
public:
    wxTerminalCtrl(wxWindow *parent, wxWindowID id, const wxPoint &pos=wxDefaultPosition,
        const wxSize &size=wxDefaultSize, long style=0, const wxString &name=wxTerminalCtrlNameStr);
    wxTerminalCtrl();
	virtual ~wxTerminalCtrl();
    virtual bool Create(wxWindow *parent, wxWindowID id, const wxPoint &pos=wxDefaultPosition,
        const wxSize &size=wxDefaultSize, long style=0, const wxString &name=wxTerminalCtrlNameStr);

	wxSize GetClientSizeInChars()const{return m_consoleSize;}

	void wrapAround(bool val);
	bool wrapAround()const {return getOption(wxTOF_WRAPAROUND);}

	void reverseWrapAround(bool val);
	bool reverseWrapAround()const {return getOption(wxTOF_REVERSE_WRAPAROUND);}

	void originMode(bool val);
	bool originMode()const {return getOption(wxTOF_ORIGINMODE);}

	void autoCarriageReturn(bool val);
	bool autoCarriageReturn()const {return getOption(wxTOF_AUTO_CARRIAGE_RETURN);}

	void cursorVisible(bool val);
	bool cursorVisible()const {return getOption(wxTOF_CURSOR_VISIBLE);}

	void cursorBlink(bool val);
	bool cursorBlink()const {return getOption(wxTOF_CURSOR_BLINK);}

	void insertMode(bool val);
	bool insertMode()const {return getOption(wxTOF_INSERT_MODE);}

	void reverseVideo(bool val);
	bool reverseVideo()const {return getOption(wxTOF_REVERSE_VIDEO);}

	bool getOption(wxTerminalOptionFlags opt)const{return (m_options & (1 << opt)) != 0;}

	/** Process new line ('\n' character). */
	void newLine();
	/** Process line feed (0x0A): move down caret (keep col unchanged). */
	void lineFeed();
	/** Process form feed (0x0C): if autoCarriageReturn : new line, else line feed. */
	void formFeed();
	/** Process carriage return (0x0d). */
	void carriageReturn();


	/** Move the cursor up a specified number of rows.*/
	void cursorUp(int count=1);
	/** Move the cursor down a specified number of rows.*/
	void cursorDown(int count=1);
	/** Move the cursor left a specified number of columns.*/
	void cursorLeft(int count=1);
	/** Move the cursor right a specified number of columns.*/
	void cursorRight(int count=1);
	/** Move the cursor to the specified column. */
	void setCursorColumn(int col);
	/** Move the cursor to the specified position. */
	void setCursorPosition(int row, int col);

	/** Replace all characters to the left of the current cursor.*/
	void eraseLeft();
	/** Erase all characters to the left of the current cursor.*/
	void eraseRight();
	/** Erase the current line.*/
	void eraseLine();
	/** Erase all characters from the start of the screen to the current cursor position, regardless of scroll region.*/
	void eraseAbove();
	/** Erase all characters from the current cursor position to the end of the screen, regardless of scroll region.*/
	void eraseBelow();
	/** Erase the screen.*/
	void eraseScreen();
	/** Insert lines at cursor position.*/
	void insertLines(unsigned int count = 1);
	/** Delete lines at cursor position.*/
	void deleteLines(unsigned int count = 1);


	/** Move the cursor forward to the next tab stop, or to the last column if no more tab stops are set. */
	void forwardTabStops();
	/** Move the cursor backward to the previous tab stop, or to the first column if no previous tab stops are set. */
	void backwardTabStops();
	/** Set a tab stop at the given column. */
	void setTabStop(int col);
	/** Set a tab stop at the cursor position. */
	void setTabStop();
	/** Clear the tab stop at the given column. */
	void clearTabStop(int col);
	/** Clear the tab stop at the cursor position. */
	void clearTabStop();
	/** Clear all tab stops. */
	void clearAllTabStops();
	/** Set up the default tab stops, starting from a given column. */
	void setDefaultTabStops(int col = 0);
	

	/** Set one of the ANSI defined terminal mode bits.
	 * @param state @true to set (SM) and @false to reset (RM)*/
	void setANSIMode(unsigned int mode, bool state);

	/** Set one of the DEC private mode.
	 * @param state @true to set (DECSET) and @false to reset (DECRST)*/
	void setDECMode(unsigned int mode, bool state);


	/** Save terminal state. */
	void saveState();
	/** Restore terminal state from saved state. */
	void restoreState();
	/** Set alternate mode.
	 * @param alternate @true to use alternate screen and @false to use normal screen. */
	void setAlternateMode(bool alternate);
	
	/** Test if shown screen is primary. */
	bool isPrimaryScreen()const {return m_currentScreen==m_primaryScreen;}
	
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
	void SetChar(wxUniChar c);
	
	/** Recompute scroll bar states (size and pos) from console size and historic position and size.*/ 
	void UpdateScrollBars();

	/** Update caret widget position. */
	void UpdateCaret();
	
	/**
	 * Declaration of TerminalParser interface abstract functions
	 * \{ */
	/*overriden*/ void onPrintableChar(unsigned char c);

	// ESC:
	//------
	/*overriden*/ void onS7C1T(); // 7-bit controls
	/*overriden*/ void onS8C1T(); // 8-bit controls
	/*overriden*/ void onANSIconf1(); // Set ANSI conformance level 1  (vt100, 7-bit controls).
	/*overriden*/ void onANSIconf2(); // Set ANSI conformance level 2  (vt200).
	/*overriden*/ void onANSIconf3(); // Set ANSI conformance level 3  (vt300).
	/*overriden*/ void onDECDHLth(); // DEC double-height line, top half
	/*overriden*/ void onDECDHLbh(); // DEC double-height line, bottom half
	/*overriden*/ void onDECSWL(); // DEC single-width line
	/*overriden*/ void onDECDWL(); // DEC double-width line
	/*overriden*/ void onDECALN(); // DEC Screen Alignment Test
	/*overriden*/ void onISO8859_1(); // Select default character set. That is ISO 8859-1 (ISO 2022).
	/*overriden*/ void onUTF_8(); // Select UTF-8 character set (ISO 2022).
	/*overriden*/ void onSCS(unsigned char id, unsigned char charset); // Character Set Selection (SCS). Designate G(id) (G0...G3) Character Set (ISO 2022)
	/*overriden*/ void onDECBI(); // Back Index, VT420 and up.
	/*overriden*/ void onDECSC(); // Save cursor
	/*overriden*/ void onDECRC(); // Restore cursor
	/*overriden*/ void onDECFI(); // Forward Index, VT420 and up.
	/*overriden*/ void onDECKPAM(); // Application Keypad
	/*overriden*/ void onDECKPNM(); // Normal Keypad
	/*overriden*/ void onRIS(); // Full Reset
	/*overriden*/ void onLS2(); // Invoke the G2 Character Set as GL.
	/*overriden*/ void onLS3(); // Invoke the G3 Character Set as GL.
	/*overriden*/ void onLS1R(); // Invoke the G1 Character Set as GR (). Has no visible effect in xterm.
	/*overriden*/ void onLS2R(); // Invoke the G1 Character Set as GR (). Has no visible effect in xterm.
	/*overriden*/ void onLS3R(); // Invoke the G1 Character Set as GR (). Has no visible effect in xterm.

	// C0:
	//-----	
	/*overriden*/ void onNUL();  // 0x00
	/*overriden*/ void onSOH();  // 0x01
	/*overriden*/ void onSTX();  // 0x02
	/*overriden*/ void onETX();  // 0x03
	/*overriden*/ void onEOT();  // 0x04
	/*overriden*/ void onENQ();  // 0x05
	/*overriden*/ void onACK();  // 0x06
	/*overriden*/ void onBEL();  // 0x07
	/*overriden*/ void onBS();   // 0x08	
	/*overriden*/ void onHT();   // 0x09
	/*overriden*/ void onLF();   // 0x0A	
	/*overriden*/ void onVT();   // 0x0B
	/*overriden*/ void onFF();   // 0x0C
	/*overriden*/ void onCR();   // 0x0D	
	/*overriden*/ void onSO();   // 0x0E
	/*overriden*/ void onSI();   // 0x0F
	/*overriden*/ void onDLE();  // 0x10
	/*overriden*/ void onDC1();  // 0x11
	/*overriden*/ void onDC2();  // 0x12
	/*overriden*/ void onDC3();  // 0x13
	/*overriden*/ void onDC4();  // 0x14
	/*overriden*/ void onNAK();  // 0x15
	/*overriden*/ void onSYN();  // 0x16
	/*overriden*/ void onETB();  // 0x17
	/*overriden*/ void onEM();   // 0x19
	/*overriden*/ void onFS();   // 0x1C
	/*overriden*/ void onGS();   // 0x1D
	/*overriden*/ void onRS();   // 0x1E
	/*overriden*/ void onUS();   // 0x1F

	// C1:
	//-----	
	/*overriden*/ void onPAD();  // 0x80
	/*overriden*/ void onHOP();  // 0x81
	/*overriden*/ void onBPH();  // 0x82
	/*overriden*/ void onNBH();  // 0x83
	/*overriden*/ void onIND();  // 0x84
	/*overriden*/ void onNEL();  // 0x85
	/*overriden*/ void onSSA();  // 0x86
	/*overriden*/ void onESA();  // 0x87
	/*overriden*/ void onHTS();  // 0x88
	/*overriden*/ void onHTJ();  // 0x89
	/*overriden*/ void onVTS();  // 0x8A
	/*overriden*/ void onPLD();  // 0x8B
	/*overriden*/ void onPLU();  // 0x8C
	/*overriden*/ void onRI();   // 0x8D
	/*overriden*/ void onSS2();  // 0x8E
	/*overriden*/ void onSS3();  // 0x8F
	/*overriden*/ void onPU1();  // 0x91
	/*overriden*/ void onPU2();  // 0x92
	/*overriden*/ void onSTS();  // 0x93
	/*overriden*/ void onCCH();  // 0x94
	/*overriden*/ void onMW();   // 0x95
	/*overriden*/ void onSPA();  // 0x96
	/*overriden*/ void onEPA();  // 0x97
	/*overriden*/ void onSGCI(); // 0x99
	/*overriden*/ void onSCI();  // 0x9A

	// CSI:
	//-----	
	/*overriden*/ void onICH(unsigned short nb=1); // Insert P s (Blank) Character(s) (default = 1)
	/*overriden*/ void onCUU(unsigned short nb=1); // Cursor Up P s Times (default = 1)
	/*overriden*/ void onCUD(unsigned short nb=1); // Cursor Down P s Times (default = 1)
	/*overriden*/ void onCUF(unsigned short nb=1); // Cursor Forward P s Times (default = 1)
	/*overriden*/ void onCUB(unsigned short nb=1); // Cursor Backward P s Times (default = 1)
	/*overriden*/ void onCNL(unsigned short nb=1); // Cursor Next Line P s Times (default = 1)
	/*overriden*/ void onCPL(unsigned short nb=1); // Cursor Preceding Line P s Times (default = 1)
	/*overriden*/ void onCHA(unsigned short nb=1); // Cursor Character Absolute [column] (default = [row,1]) // Moves the cursor to column n.
	/*overriden*/ void onCUP(unsigned short row=1, unsigned short col=1); // Cursor Position [row;column] (default = [1,1])
	/*overriden*/ void onCHT(unsigned short nb=1); // Cursor Forward Tabulation P s tab stops (default = 1)
	/*overriden*/ void onED(unsigned short opt);  // Erase in Display. 0 → Erase Below (default). 1 → Erase Above. 2 → Erase All. 3 → Erase Saved Lines (xterm).	
	/*overriden*/ void onDECSED(unsigned short opt=1);  // Erase in Display. 0 → Selective Erase Below (default). 1 → Selective Erase Above. 2 → Selective Erase All. 3 → Selective Erase Saved Lines (xterm).
	/*overriden*/ void onEL(unsigned short opt); // Erase in Line. 0 → Erase to Right (default). 1 → Erase to Left. 2 → Erase All.	
	/*overriden*/ void onDECSEL(unsigned short nb=0);  // Erase in Line. 0 → Selective Erase to Right (default). 1 → Selective Erase to Left. 2 → Selective Erase All.
	/*overriden*/ void onIL(unsigned short nb=1);  // Insert Ps Line(s) (default = 1)
	/*overriden*/ void onDL(unsigned short nb=1);  // Delete Ps Line(s) (default = 1)
	/*overriden*/ void onDCH(unsigned short nb=1); // Delete Ps Character(s) (default = 1)
	/*overriden*/ void onSU(unsigned short nb=1);  // Scroll up Ps lines (default = 1)
	/*overriden*/ void onSD(unsigned short nb=1);  // Scroll down Ps lines (default = 1)
	/*overriden*/ void onECH(unsigned short nb=1);  // Erase Ps Character(s) (default = 1)
	/*overriden*/ void onCBT(unsigned short nb=1);  // Cursor Backward Tabulation Ps tab stops (default = 1)
	/*overriden*/ void onHPA(const std::vector<unsigned short> nbs);  // Character Position Absolute [column] (default = [row,1]) (HPA).
	/*overriden*/ void onHPR(const std::vector<unsigned short> nbs);  // Character Position Relative [columns] (default = [row,col+1]) (HPR).
	/*overriden*/ void onVPA(const std::vector<unsigned short> nbs);  // Line Position Absolute [row] (default = [1,column])
	/*overriden*/ void onVPR(const std::vector<unsigned short> nbs);  // Line Position Relative [rows] (default = [row+1,column]) (VPR)
	/*overriden*/ void onHVP(unsigned short row=1, unsigned short col=1); // Horizontal and Vertical Position [row;column] (default = [1,1])
	/*overriden*/ void onTBC(unsigned short nb=0);  // Tab Clear
	/*overriden*/ void onSM(const std::vector<unsigned short> nbs);  // Set Mode
	/*overriden*/ void onDECSET(const std::vector<unsigned short> nbs);  // DEC Private Mode Set
	/*overriden*/ void onMC(const std::vector<unsigned short> nbs);  // Media Copy
	/*overriden*/ void onDECMC(const std::vector<unsigned short> nbs);  // DEC specific Media Copy
	/*overriden*/ void onRM(const std::vector<unsigned short> nbs);  // Reset Mode
	/*overriden*/ void onDECRST(const std::vector<unsigned short> nbs);  // DEC Private Mode Reset
	/*overriden*/ void onSGR(const std::vector<unsigned short> nbs);  // Character Attributes
	/*overriden*/ void onDSR(unsigned short nb);  // Device Status Report
	/*overriden*/ void onDECDSR(unsigned short nb);  // DEC-specific Device Status Report
	/*overriden*/ void onDECSTR();  // Soft terminal reset
	/*overriden*/ void onDECSCL(unsigned short nb1, unsigned short nb2); // Set conformance level
	/*overriden*/ void onDECRQM(unsigned short nb);  // Request DEC private mode
	/*overriden*/ void onDECLL(unsigned short nb=0);  // Load LEDs
	/*overriden*/ void onDECSCUSR(unsigned short nb);  // Set cursor style (DECSCUSR, VT520).
	/*overriden*/ void onDECSCA(unsigned short nb=0);  // Select character protection attribute (DECSCA).
	/*overriden*/ void onDECSTBM(unsigned short top=0, unsigned short bottom=0); // Set Scrolling Region [top;bottom] (default = full size of window)
 	/*overriden*/ void onRDECPMV(const std::vector<unsigned short> nbs);  // Restore DEC Private Mode Values. The value of P s previously saved is restored. P s values are the same as for DECSET.
	/*overriden*/ void onSDECPMV(const std::vector<unsigned short> nbs);  // Save DEC Private Mode Values. P s values are the same as for DECSET.
	/*overriden*/ void onDECCARA(const std::vector<unsigned short> nbs);  // Change Attributes in Rectangular Area (DECCARA), VT400 and up.
	/*overriden*/ void onDECSLRM(unsigned short left, unsigned short right); // Set left and right margins (DECSLRM), available only when DECLRMM is enabled (VT420 and up).
	/*overriden*/ void onANSISC();  // Save cursor (ANSI.SYS), available only when DECLRMM is disabled.
	/*overriden*/ void onANSIRC();  // Restore cursor (ANSI.SYS).
	/*overriden*/ void onWindowManip(unsigned short nb1, unsigned short nb2=0, unsigned short nb3=0); // Set conformance level
	/*overriden*/ void onDECRARA(const std::vector<unsigned short> nbs);  // Reverse Attributes in Rectangular Area (DECRARA), VT400 and up.
	/*overriden*/ void onDECSWBV(unsigned short nb);  // Set warning-bell volume (DECSWBV, VT520).
	/*overriden*/ void onDECSMBV(unsigned short nb);  // Set margin-bell volume (DECSMBV, VT520).
	/*overriden*/ void onDECCRA(const std::vector<unsigned short> nbs);  // Copy Rectangular Area (DECCRA, VT400 and up).
	/*overriden*/ void onDECEFR(unsigned short top, unsigned short left, unsigned short bottom, unsigned short right); // Enable Filter Rectangle (DECEFR), VT420 and up.
	/*overriden*/ void onDECREQTPARM(unsigned short nb);  // Request Terminal Parameters
	/*overriden*/ void onDECSACE(unsigned short nb);  // Select Attribute Change Extent
	/*overriden*/ void onDECFRA(unsigned short chr, unsigned short top, unsigned short left, unsigned short bottom, unsigned short right); // Fill Rectangular Area (DECFRA), VT420 and up.
	/*overriden*/ void onDECRQCRA(unsigned short id, unsigned short page, unsigned short top, unsigned short left, unsigned short bottom, unsigned short right); // Request Checksum of Rectangular Area (DECRQCRA), VT420 and up.
	/*overriden*/ void onDECELR(unsigned short nb1, unsigned short nb2); // Enable Locator Reporting (DECELR).
	/*overriden*/ void onDECERA(unsigned short top, unsigned short left, unsigned short bottom, unsigned short right); // Erase Rectangular Area (DECERA), VT400 and up.
	/*overriden*/ void onDECSLE(const std::vector<unsigned short> nbs);  // Select Locator Events (DECSLE).
	/*overriden*/ void onDECSERA(unsigned short top, unsigned short left, unsigned short bottom, unsigned short right); // Selective Erase Rectangular Area (), VT400 and up.
 	/*overriden*/ void onDECRQLP(unsigned short nb);  // Request Locator Position (DECRQLP).
	/*overriden*/ void onDECIC(unsigned short nb=1);  // Insert P s Column(s) (default = 1) (DECIC), VT420 and up.
	/*overriden*/ void onDECDC(unsigned short nb=1);  // InsDelete P s Column(s) (default = 1) (DECIC), VT420 and up.

	// OSC:
	//------
	/*overriden*/ void onOSC(unsigned short command, const std::vector<unsigned char>& params); // Receive an OSC (Operating System Command) command.
	
	/** \} */
	
private:
	wxTerminalScreen* m_primaryScreen;
	wxTerminalScreen* m_alternateScreen;
	wxTerminalScreen* m_currentScreen;

	void GenerateFonts(const wxFont& font);
	
	void OnPaint(wxPaintEvent& event);
	void OnSize(wxSizeEvent& event);
	void OnScroll(wxScrollWinEvent& event);
	void OnChar(wxKeyEvent& event);
	void OnTimer(wxTimerEvent& event);
	
	wxSize   m_consoleSize; // Size of console in chars
	
	wxCaret* m_caret;       // Caret pseudo-widget instance.

	wxFont m_defaultFont, m_boldFont, m_underlineFont, m_boldUnderlineFont;
	wxColour m_colours[8];

	wxTerminalCharacterSet m_charset; // Current input character set
	wxTerminalCharacterDecoder m_mbdecoder; // Multibyte decoder (for UTF-x) 

	/** Current and saved terminal state.
	 * Note: cursor position in current state is not used, refer to currentScreen cursor position instead.
	 */
	wxTerminalState m_currentState, m_savedState;

	unsigned int m_options; // Flags from wxTerminalOptionFlags

	unsigned int m_tabWidth; // Size of tab in chars
	std::set<unsigned int> m_tabstops; // Set of tabstops
	
	wxTimer* m_timer;    // Timer for i/o treatments.
	wxOutputStream* m_outputStream;
	wxInputStream*  m_inputStream;
	wxInputStream*  m_errorStream;
};

#endif // _TERMINAL_CTRL_HPP_
