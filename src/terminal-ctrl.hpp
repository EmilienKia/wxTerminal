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

struct wxTerminalCharacterAttributes
{
	unsigned char fore;
	unsigned char back;
	unsigned char style; // From wxTerminalCharacterStyle
};

struct wxTerminalCharacter
{
	wxUniChar c;
	wxTerminalCharacterAttributes attr;

	static wxTerminalCharacter DefaultCharacter;
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
	virtual void setChar(wxPoint pos, wxUniChar c, const wxTerminalCharacterAttributes& attr);

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

	/** Translate Console coordinates to Historic coordinates.*/
	int ConsoleToHistoric(int row)const;
	wxPoint ConsoleToHistoric(wxPoint pt)const{return wxPoint(pt.x, ConsoleToHistoric(pt.y));}
	
	/** Translate Historic coordinates to Console coordinates.*/
	int HistoricToConsole(int row)const;
	wxPoint HistoricToConsole(wxPoint pt)const{return wxPoint(pt.x, HistoricToConsole(pt.y));}
	
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
	
	wxSize   m_consoleSize; // Size of console in chars
	wxPoint  m_consolePos;  // Position of consol in historic.
	
	wxCaret* m_caret;       // Caret pseudo-widget instance.
	wxPoint  m_caretPos;    // Position of caret (console cursor) in chars

	wxFont m_defaultFont, m_boldFont, m_underlineFont, m_boldUnderlineFont;
	wxColour m_colours[8];
	wxTerminalCharacterAttributes m_currentAttributes;

	wxTerminalCharacterSet m_charset; // Current input character set
	wxTerminalCharacterDecoder m_mbdecoder; // Multibyte decoder (for UTF-x) 
	
	const wxTerminalCharacterMap* m_Gx[4]; // G0...G3 character maps
	unsigned short m_GL, m_GR; // Respectively 7-bit and 8-bit visible character set (values in 0...3).
	
	wxTimer* m_timer;    // Timer for i/o treatments.
	wxOutputStream* m_outputStream;
	wxInputStream*  m_inputStream;
	wxInputStream*  m_errorStream;
};

#endif // _TERMINAL_CTRL_HPP_
