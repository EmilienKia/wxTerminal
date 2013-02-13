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
#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif
#include <wx/wx.h>

#include <wx/dcbuffer.h>
#include <wx/caret.h>
#include <wx/event.h>

#include <cstring>
using namespace std;

#include "terminal-ctrl.hpp"

//
//
// wxTerminalCharacter
//
//

wxTerminalCharacter wxTerminalCharacter::DefaultCharacter = { 0, 0, wxTCS_Invisible, 0 };

//
//
// wxConsoleContent
//
//

void wxConsoleContent::setChar(wxPoint pos, wxChar c, unsigned char fore, unsigned char back, unsigned char style)
{
	wxTerminalCharacter ch;
	ch.c     = c;
	ch.fore  = fore;
	ch.back  = back;
	ch.style = style;
	setChar(pos, ch);
}


void wxConsoleContent::setChar(wxPoint pos, wxTerminalCharacter c)
{
	ensureHasChar(pos.y, pos.x);
	at(pos.y)[pos.x] = c;
}

void wxConsoleContent::addNewLine()
{
	push_back(wxTerminalLine());
}

void wxConsoleContent::ensureHasLine(size_t l)
{
	if(size()<=l)
		resize(l+1);
}

void wxConsoleContent::ensureHasChar(size_t l, size_t c)
{
	ensureHasLine(l);
	
	wxTerminalLine& line = at(l);

	if(line.size()<=c)
		line.resize(c+1, wxTerminalCharacter::DefaultCharacter);
}


//
//
// wxTerminalCtrl
//
//


wxString wxTerminalCtrlNameStr(wxT("wxTerminalCtrl"));

wxBEGIN_EVENT_TABLE(wxTerminalCtrl, wxWindow)
	EVT_PAINT(wxTerminalCtrl::OnPaint)
	EVT_SIZE(wxTerminalCtrl::OnSize)
	EVT_SCROLLWIN(wxTerminalCtrl::OnScroll)
	EVT_CHAR(wxTerminalCtrl::OnChar)
	EVT_TIMER(wxID_ANY, wxTerminalCtrl::OnTimer)
wxEND_EVENT_TABLE()

wxTerminalCtrl::wxTerminalCtrl(wxWindow *parent, wxWindowID id, const wxPoint &pos,
    const wxSize &size, long style, const wxString &name):
wxWindow(parent, id, pos, size, style|wxVSCROLL/*|wxHSCROLL*/, name),
m_inputStream(NULL),
m_outputStream(NULL),
m_historicContent(NULL),
m_staticContent(NULL),
m_currentContent(NULL)
{
	CommonInit();
}

wxTerminalCtrl::wxTerminalCtrl():
m_inputStream(NULL),
m_outputStream(NULL),
m_historicContent(NULL),
m_staticContent(NULL),
m_currentContent(NULL)
{
}

wxTerminalCtrl::~wxTerminalCtrl()
{
}

bool wxTerminalCtrl::Create(wxWindow *parent, wxWindowID id, const wxPoint &pos,
    const wxSize &size, long style, const wxString &name)
{
	if(!wxWindow::Create(parent, id, pos, size, style|wxVSCROLL/*|wxHSCROLL*/, name))
		return false;
	CommonInit();
	return true;
}

void wxTerminalCtrl::CommonInit()
{
	m_historicContent = new wxConsoleContent;
	m_staticContent = new wxConsoleContent;

	// Set historic content as current (default behavior)
	m_currentContent = m_historicContent;
	m_isHistoric = true;

	m_colours[0]  = wxColour(0, 0, 0); // Normal black
	m_colours[1]  = wxColour(255, 85, 85); // Bright red
	m_colours[2] = wxColour(85, 255, 85); // Bright green
	m_colours[3] = wxColour(255, 255, 85); // Bright yellow
	m_colours[4] = wxColour(85, 85, 255); // Bright blue
	m_colours[5] = wxColour(255, 85, 255); // Bright magenta
	m_colours[6] = wxColour(85, 255, 255); // Bright cyan
	m_colours[7] = wxColour(255, 255, 255); // Bright grey (white)
/*	m_colours[0]  = wxColour(0, 0, 0); // Normal black
	m_colours[1]  = wxColour(170, 0, 0); // Normal red
	m_colours[2]  = wxColour(0, 170, 0); // Normal green
	m_colours[3]  = wxColour(170, 85, 0); // Normal yellow (brown)
	m_colours[4]  = wxColour(0, 0, 170); // Normal blue
	m_colours[5]  = wxColour(170, 0, 170); // Normal magenta
	m_colours[6]  = wxColour(0, 170, 170); // Normal cyan
	m_colours[7]  = wxColour(170, 170, 170); // Normal gray
	m_colours[8]  = wxColour(85, 85, 85); // Bright black (dark grey)
	m_colours[9]  = wxColour(255, 85, 85); // Bright red
	m_colours[10] = wxColour(85, 255, 85); // Bright green
	m_colours[11] = wxColour(255, 255, 85); // Bright yellow
	m_colours[12] = wxColour(85, 85, 255); // Bright blue
	m_colours[13] = wxColour(255, 85, 255); // Bright magenta
	m_colours[14] = wxColour(85, 255, 255); // Bright cyan
	m_colours[15] = wxColour(255, 255, 255); // Bright grey (white)
*/

	m_consoleSize = wxSize(80, 25);
	m_consolePos  = wxPoint(0, 0);

	GenerateFonts(wxFont(10, wxFONTFAMILY_TELETYPE));
	SetBackgroundStyle(wxBG_STYLE_PAINT);
	SetBackgroundColour(m_colours[0]);

	m_lastBackColor = 0;
	m_lastForeColor = 7;
	m_lastStyle     = wxTCS_Normal;

	m_caret = new wxCaret(this, GetCharSize());
	m_caret->Show();
	SetCaretPosition(0, 0);

	UpdateScrollBars();
	
	m_timer = new wxTimer(this);
	m_timer->Start(10);
}

void wxTerminalCtrl::GenerateFonts(const wxFont& font)
{
	m_defaultFont   = font;
	m_boldFont      = font.Bold();
	m_underlineFont = font.Underlined();
	m_boldUnderlineFont = m_boldFont.Underlined();
}

wxSize wxTerminalCtrl::GetCharSize(wxChar c)const
{
	wxSize sz;
	GetTextExtent(c, &sz.x, &sz.y, NULL, NULL, &m_defaultFont);
	return sz;
}

int wxTerminalCtrl::ConsoleToHistoric(int row)const
{
	return row + m_consolePos.y;
}

int wxTerminalCtrl::HistoricToConsole(int row)const
{
	return row - m_consolePos.y;
}

wxPoint wxTerminalCtrl::GetCaretPosInBuffer()const
{
	return ConsoleToHistoric(GetCaretPosition());
}

void wxTerminalCtrl::SetCaretPosition(wxPoint pos)
{
	// Move caret position
	if(pos.x==-1)
		pos.x = m_caretPos.x;
	if(pos.y==-1)
		pos.y = m_caretPos.y;
	m_caretPos = pos;
 

	// Move caret pseudo-widget in consequence
//printf("SetCaretPosition %d,%d\n", m_caretPos.x, m_caretPos.y);
	wxSize sz = GetCharSize();
	m_caret->Move(sz.x*pos.x, sz.y*pos.y);
}

void wxTerminalCtrl::MoveCaret(int x, int y)
{
	wxPoint pos = m_caretPos + wxSize(x, y);

	// Scroll vertically to make caret horizontally visible
	int scroll = 0;
	while(pos.x >= m_consoleSize.x)
	{
		++scroll;
		++pos.y;
		pos.x -= m_consoleSize.x;
	}
	while(pos.x < 0)
	{
		--scroll;
		--pos.y;
		pos.x += m_consoleSize.x;
	}

	SetCaretPosition(pos);
}

void wxTerminalCtrl::SetChar(wxChar c)
{
	m_currentContent->setChar(ConsoleToHistoric(m_caretPos), c, m_lastForeColor, m_lastBackColor, m_lastStyle);
}


void wxTerminalCtrl::OnPaint(wxPaintEvent& event)
{
	wxCaretSuspend caretSuspend(this);
		
	wxSize charSz = GetCharSize();
	wxSize clientSz = GetClientSize();
	wxSize clchSz = GetClientSizeInChars();

	wxAutoBufferedPaintDC dc(this);
	dc.SetBrush(wxBrush(*wxBLACK));
	dc.SetPen(wxNullPen);
	dc.DrawRectangle(0, 0, clientSz.x, clientSz.y);
	
	wxConsoleContent* content = m_currentContent;
	if(content)
	{
		for(size_t n=0, l=m_consolePos.y; n<clchSz.y && l<content->size(); n++, l++)
		{
			const wxTerminalLine& line = content->at(l);
			for(size_t i=0; i<line.size(); i++)
			{
				const wxTerminalCharacter &ch = line[i];

				// Not shown so skip
				if(ch.style & wxTCS_Invisible)
					continue;

				// Choose font
				if(ch.style & wxTCS_Bold)
				{
					if(ch.style & wxTCS_Underlined)
						dc.SetFont(m_boldUnderlineFont);
					else
						dc.SetFont(m_boldFont);
				}
				else
				{
					if(ch.style & wxTCS_Underlined)
						dc.SetFont(m_underlineFont);
					else
						dc.SetFont(m_defaultFont);
				}

				// Choose colors
				if(ch.style & wxTCS_Inverse)
				{
					dc.SetBrush(wxBrush(m_colours[ch.fore]));
					dc.SetTextBackground(m_colours[ch.fore]);
					dc.SetTextForeground(m_colours[ch.back]);
				}
				else
				{
					dc.SetBrush(wxBrush(m_colours[ch.back]));
					dc.SetTextBackground(m_colours[ch.back]);
					dc.SetTextForeground(m_colours[ch.fore]);
				}

				// Draw
				dc.DrawRectangle(i*charSz.x, n*charSz.y, charSz.x, charSz.y);
				dc.DrawText(ch.c, i*charSz.x, n*charSz.y);	
			}
		}
	}
}

void wxTerminalCtrl::OnScroll(wxScrollWinEvent& event)
{
	// Save aboslute historic caret position (before console scroll)
	wxPoint carpos = ConsoleToHistoric(m_caretPos);
	
	if(event.GetOrientation() == wxVERTICAL)
	{
		m_consolePos.y = event.GetPosition();
	}

	// Apply caret position (after scrolling)
	SetCaretPosition(HistoricToConsole(carpos));
	
	Refresh();
	event.Skip();
}

void wxTerminalCtrl::OnSize(wxSizeEvent& event)
{
	wxSize sz = GetClientSize();
	wxSize ch = GetCharSize();
	m_consoleSize = wxSize(sz.x/ch.x, sz.y/ch.y);
	
	UpdateScrollBars();
}

void wxTerminalCtrl::UpdateScrollBars()
{
	wxPoint pos = ConsoleToHistoric(m_caretPos);

	SetScrollbar(wxVERTICAL, GetScrollPos(wxVERTICAL), m_consoleSize.y, m_currentContent->size());
	
	SetCaretPosition(HistoricToConsole(pos));
}

void wxTerminalCtrl::OnChar(wxKeyEvent& event)
{
	int c = event.GetUnicodeKey();
	if( c != WXK_NONE)
	{
		// Process special keys
		if(c==WXK_RETURN)
			c = wxT('\n');

		if(m_outputStream!=NULL)
		{
			if(c<32)
			{
				if(c!=0x08 && c!=0x0A && c!=0x0D) // Backspace or Line feed or Cariage return
					printf("Enter C0 code : %02x\n", c);
			}
			m_outputStream->PutC(c);
		}
		
	}
	else
	{
		int key = event.GetKeyCode();
		switch(key)
		{
			case WXK_UP:
			{
				static unsigned char cuu[] = {0x1B, 0x5B, 0x41};// CUrsor Up
				m_outputStream->Write(cuu, 3);
				break;
			}
			case WXK_DOWN:
			{
				static unsigned char cud[] = {0x1B, 0x5B, 0x42};// CUrsor Down
				m_outputStream->Write(cud, 3);
				break;
			}
			case WXK_LEFT:
			{
				static unsigned char cub[] = {0x1B, 0x5B, 0x44};// CUrsor Backward
				m_outputStream->Write(cub, 3);
				break;
			}
			case WXK_RIGHT:
			{
				static unsigned char cuf[] = {0x1B, 0x5B, 0x43};// CUrsor Foreward
				m_outputStream->Write(cuf, 3);
				break;
			}			
			case WXK_PAGEUP:
			{
				static unsigned char su[] = {0x1B, 0x5B, 0x53};// Scroll Up
				m_outputStream->Write(su, 3);
				break;
			}
			case WXK_PAGEDOWN:
			{
				static unsigned char sd[] = {0x1B, 0x5B, 0x54};// Scroll Down
				m_outputStream->Write(sd, 3);
				break;
			}
			default:
			{
				std::cout << "Key : " << key << std::endl;
				break;
			}
		}
	}
}


//
// Treatment of inputs and outputs 
//

void wxTerminalCtrl::OnTimer(wxTimerEvent& event)
{
	if(m_inputStream)
	{
		if(m_inputStream->CanRead())
		{
			unsigned char buffer[256];
			while(true)
			{
				m_inputStream->Read(buffer, 256);
				size_t sz = m_inputStream->LastRead();
				Append(buffer, sz);
				if(sz<256)
					break;
			}

			UpdateScrollBars();
			Refresh();
		}
	}
	if(m_errorStream)
	{
		if(m_errorStream->CanRead())
		{
			unsigned char buffer[256];
			while(true)
			{
				m_errorStream->Read(buffer, 256);
				size_t sz = m_errorStream->LastRead();
				Append(buffer, sz);
				if(sz<256)
					break;
			}

			UpdateScrollBars();
			Refresh();
		}
	}
}

void wxTerminalCtrl::Append(const unsigned char* buff, size_t sz)
{
	if(buff && sz>0)
	{
		for(size_t n=0; n<sz; n++)
		{
			Process(buff[n]);
		}
	}
}


//
// Definition of TerminalParser interface abstract functions:
//


void wxTerminalCtrl::onPrintableChar(unsigned char c)
{
	SetChar(c);
	MoveCaret(1);
}

void wxTerminalCtrl::onS7C1T() // 7-bit controls
{
	std::cout << "onS7C1T" << std::endl;
}

void wxTerminalCtrl::onS8C1T() // 8-bit controls
{
	std::cout << "onS8C1T" << std::endl;
}

void wxTerminalCtrl::onANSIconf1() // Set ANSI conformance level 1  (vt100, 7-bit controls).
{
	std::cout << "onANSIconf1" << std::endl;
}

void wxTerminalCtrl::onANSIconf2() // Set ANSI conformance level 2  (vt200).
{
	std::cout << "onANSIconf2" << std::endl;
}

void wxTerminalCtrl::onANSIconf3() // Set ANSI conformance level 3  (vt300).
{
	std::cout << "onANSIconf3" << std::endl;
}

void wxTerminalCtrl::onDECDHLth() // DEC double-height line, top half
{
	std::cout << "onDECDHLth" << std::endl;
}

void wxTerminalCtrl::onDECDHLbh() // DEC double-height line, bottom half
{
	std::cout << "onDECDHLbh" << std::endl;
}

void wxTerminalCtrl::onDECSWL() // DEC single-width line
{
	std::cout << "onDECSWL" << std::endl;
}

void wxTerminalCtrl::onDECDWL() // DEC double-width line
{
	std::cout << "onDECDWL" << std::endl;
}

void wxTerminalCtrl::onDECALN() // DEC Screen Alignment Test
{
	std::cout << "onDECALN" << std::endl;
}

void wxTerminalCtrl::onISO8859_1() // Select default character set. That is ISO 8859-1 (ISO 2022).
{
	std::cout << "onISO8859_1" << std::endl;
}

void wxTerminalCtrl::onUTF_8() // Select UTF-8 character set (ISO 2022).
{
	std::cout << "onUTF_8" << std::endl;
}

void wxTerminalCtrl::onDesignateG0(unsigned char charset) // Designate G0 Character Set (ISO 2022) 
{
	std::cout << "onDesignateG0" << std::endl;
}

void wxTerminalCtrl::onDesignateG1(unsigned char charset) // Designate G1 Character Set (ISO 2022) 
{
	std::cout << "onDesignateG1" << std::endl;
}

void wxTerminalCtrl::onDesignateG2(unsigned char charset) // Designate G2 Character Set (ISO 2022) 
{
	std::cout << "onDesignateG2" << std::endl;
}

void wxTerminalCtrl::onDesignateG3(unsigned char charset) // Designate G3 Character Set (ISO 2022)
{
	std::cout << "onDesignateG3" << std::endl;
}

void wxTerminalCtrl::onDECBI() // Back Index, VT420 and up.
{
	std::cout << "onDECBI" << std::endl;
}

void wxTerminalCtrl::onDECSC() // Save cursor
{
	std::cout << "onDECSC" << std::endl;
}

void wxTerminalCtrl::onDECRC() // Restore cursor
{
	std::cout << "onDECRC" << std::endl;
}

void wxTerminalCtrl::onDECFI() // Forward Index, VT420 and up.
{
	std::cout << "onDECFI" << std::endl;
}

void wxTerminalCtrl::onDECKPAM() // Application Keypad
{
	std::cout << "onDECKPAM" << std::endl;
}

void wxTerminalCtrl::onDECKPNM() // Normal Keypad
{
	std::cout << "onDECKPNM" << std::endl;
}

void wxTerminalCtrl::onRIS() // Full Reset
{
	std::cout << "onRIS" << std::endl;
}

void wxTerminalCtrl::onLS2() // Invoke the G2 Character Set
{
	std::cout << "onLS2" << std::endl;
}

void wxTerminalCtrl::onLS3() // Invoke the G3 Character Set
{
	std::cout << "onLS3" << std::endl;
}

void wxTerminalCtrl::onLS1R() // Invoke the G1 Character Set as GR (). Has no visible effect in xterm.
{
	std::cout << "onLS1R" << std::endl;
}

void wxTerminalCtrl::onLS2R() // Invoke the G1 Character Set as GR (). Has no visible effect in xterm.
{
	std::cout << "onLS2R" << std::endl;
}

void wxTerminalCtrl::onLS3R() // Invoke the G1 Character Set as GR (). Has no visible effect in xterm.
{
	std::cout << "onLS3R" << std::endl;
}


// C0:
//-----	
void wxTerminalCtrl::onNUL()  // 0x00
{
	std::cout << "onNUL" << std::endl;
}

void wxTerminalCtrl::onSOH()  // 0x01
{
	std::cout << "onSOH" << std::endl;
}

void wxTerminalCtrl::onSTX()  // 0x02
{
	std::cout << "onSTX" << std::endl;
}

void wxTerminalCtrl::onETX()  // 0x03
{
	std::cout << "onETX" << std::endl;
}

void wxTerminalCtrl::onEOT()  // 0x04
{
	std::cout << "onEOT" << std::endl;
}

void wxTerminalCtrl::onENQ()  // 0x05
{
	std::cout << "onSonENQ" << std::endl;
}

void wxTerminalCtrl::onACK()  // 0x06
{
	std::cout << "onACK" << std::endl;
}

void wxTerminalCtrl::onBEL()  // 0x07 -- Mostly done
{
	wxBell();
	printf("Bell !!!\n");
}

void wxTerminalCtrl::onBS()   // 0x08 -- Done
{
	MoveCaret(-1);
}

void wxTerminalCtrl::onHT()   // 0x09
{
	std::cout << "onHT" << std::endl;
}

void wxTerminalCtrl::onLF()   // 0x0A - LINE FEED -- Done
{
	SetChar(0x0A);
	SetCaretPosition(0, GetCaretPosition().y+1);
}

void wxTerminalCtrl::onVT()   // 0x0B
{
	std::cout << "onVT" << std::endl;
}

void wxTerminalCtrl::onFF()   // 0x0C
{
	std::cout << "onFF" << std::endl;
}

void wxTerminalCtrl::onCR()   // 0x0D - CARIAGE RETURN -- Done
{
	SetChar(0x0D);
	SetCaretPosition(0, GetCaretPosition().y+1);
}

void wxTerminalCtrl::onSO()   // 0x0E
{
	std::cout << "onSO" << std::endl;
}

void wxTerminalCtrl::onSI()   // 0x0F
{
	std::cout << "onSI" << std::endl;
}

void wxTerminalCtrl::onDLE()  // 0x10
{
	std::cout << "onDLE" << std::endl;
}

void wxTerminalCtrl::onDC1()  // 0x11
{
	std::cout << "onDC1" << std::endl;
}

void wxTerminalCtrl::onDC2()  // 0x12
{
	std::cout << "onDC2" << std::endl;
}

void wxTerminalCtrl::onDC3()  // 0x13
{
	std::cout << "onDC3" << std::endl;
}

void wxTerminalCtrl::onDC4()  // 0x14
{
	std::cout << "onDC4" << std::endl;
}

void wxTerminalCtrl::onNAK()  // 0x15
{
	std::cout << "onNAK" << std::endl;
}

void wxTerminalCtrl::onSYN()  // 0x16
{
	std::cout << "onSYN" << std::endl;
}

void wxTerminalCtrl::onETB()  // 0x17
{
	std::cout << "onETB" << std::endl;
}

void wxTerminalCtrl::onEM()   // 0x19
{
	std::cout << "onEM" << std::endl;
}

void wxTerminalCtrl::onFS()   // 0x1C
{
	std::cout << "onFS" << std::endl;
}

void wxTerminalCtrl::onGS()   // 0x1D
{
	std::cout << "onGS" << std::endl;
}

void wxTerminalCtrl::onRS()   // 0x1E
{
	std::cout << "nRS" << std::endl;
}

void wxTerminalCtrl::onUS()   // 0x1F
{
	std::cout << "onUS" << std::endl;
}


// C1:
//-----	
void wxTerminalCtrl::onPAD()  // 0x80
{
	std::cout << "onPAD" << std::endl;
}

void wxTerminalCtrl::onHOP()  // 0x81
{
	std::cout << "onHOP" << std::endl;
}

void wxTerminalCtrl::onBPH()  // 0x82
{
	std::cout << "onBPH" << std::endl;
}

void wxTerminalCtrl::onNBH()  // 0x83
{
	std::cout << "onNBH" << std::endl;
}

void wxTerminalCtrl::onIND()  // 0x84
{
	std::cout << "onIND" << std::endl;
}

void wxTerminalCtrl::onNEL()  // 0x85
{
	std::cout << "onNEL" << std::endl;
}

void wxTerminalCtrl::onSSA()  // 0x86
{
	std::cout << "onSSA" << std::endl;
}

void wxTerminalCtrl::onESA()  // 0x87
{
	std::cout << "onESA" << std::endl;
}

void wxTerminalCtrl::onHTS()  // 0x88
{
	std::cout << "onHTS" << std::endl;
}

void wxTerminalCtrl::onHTJ()  // 0x89
{
	std::cout << "onHTJ" << std::endl;
}

void wxTerminalCtrl::onVTS()  // 0x8A
{
	std::cout << "onVTS" << std::endl;
}

void wxTerminalCtrl::onPLD()  // 0x8B
{
	std::cout << "onPLD" << std::endl;
}

void wxTerminalCtrl::onPLU()  // 0x8C
{
	std::cout << "onPLU" << std::endl;
}

void wxTerminalCtrl::onRI()   // 0x8D
{
	std::cout << "onRI" << std::endl;
}

void wxTerminalCtrl::onSS2()  // 0x8E
{
	std::cout << "onSS2" << std::endl;
}

void wxTerminalCtrl::onSS3()  // 0x8F
{
	std::cout << "onSS3" << std::endl;
}

void wxTerminalCtrl::onPU1()  // 0x91
{
	std::cout << "onPU1" << std::endl;
}

void wxTerminalCtrl::onPU2()  // 0x92
{
	std::cout << "onPU2" << std::endl;
}

void wxTerminalCtrl::onSTS()  // 0x93
{
	std::cout << "onSTS" << std::endl;
}

void wxTerminalCtrl::onCCH()  // 0x94
{
	std::cout << "onCCH" << std::endl;
}

void wxTerminalCtrl::onMW()   // 0x95
{
	std::cout << "onMW" << std::endl;
}

void wxTerminalCtrl::onSPA()  // 0x96
{
	std::cout << "onSPA" << std::endl;
}

void wxTerminalCtrl::onEPA()  // 0x97
{
	std::cout << "onEPA" << std::endl;
}

void wxTerminalCtrl::onSGCI() // 0x99
{
	std::cout << "onSGCI" << std::endl;
}

void wxTerminalCtrl::onSCI()  // 0x9A
{
	std::cout << "onSCI" << std::endl;
}


// CSI:
//-----	
void wxTerminalCtrl::onICH(unsigned short nb) // Insert P s (Blank) Character(s) (default = 1)
{
	std::cout << "onICH" << std::endl;
}

void wxTerminalCtrl::onCUU(unsigned short nb) // Cursor Up P s Times (default = 1)
{
	std::cout << "onCUU" << std::endl;
}

void wxTerminalCtrl::onCUD(unsigned short nb) // Cursor Down P s Times (default = 1)
{
	std::cout << "onCUD" << std::endl;
}

void wxTerminalCtrl::onCUF(unsigned short nb) // Cursor Forward P s Times (default = 1)
{
	std::cout << "onCUF" << std::endl;
}

void wxTerminalCtrl::onCUB(unsigned short nb) // Cursor Backward P s Times (default = 1)
{
	std::cout << "onCUB" << std::endl;
}

void wxTerminalCtrl::onCNL(unsigned short nb) // Cursor Next Line P s Times (default = 1)
{
	std::cout << "onCNL" << std::endl;
}

void wxTerminalCtrl::onCPL(unsigned short nb) // Cursor Preceding Line P s Times (default = 1)
{
	std::cout << "onCPL" << std::endl;
}

void wxTerminalCtrl::onCHA(unsigned short nb) // Moves the cursor to column n. -- Done
{
	SetCaretPosition(nb - 1, GetCaretPosition().y);
}

void wxTerminalCtrl::onCUP(unsigned short row, unsigned short col) // Moves the cursor to row n, column m -- Done
{
	SetCaretPosition(col - 1, row - 1);
}

void wxTerminalCtrl::onCHT(unsigned short nb) // Cursor Forward Tabulation P s tab stops (default = 1)
{
	std::cout << "onCHT" << std::endl;
}


void wxTerminalCtrl::onED(unsigned short opt) // Clears part of the screen. -- Mostly done
{
	wxPoint pos = GetCaretPosInBuffer();
	if(opt==0) // Erase Below
	{
std::cout << "Erase below" << std::endl;
		m_currentContent->resize(pos.y+1);
	}
	else if(opt==1) // Erase Above
	{
std::cout << "Erase above" << std::endl;
		int top = ConsoleToHistoric(0);
		for(; top<pos.y-1 ; ++top)
		{
			(*m_currentContent)[top].clear();
		}
	}
	else if(opt==2) // Erase All
	{
std::cout << "Erase All" << std::endl;
		for(int n=0; n<m_consoleSize.y; ++n)
		{
			m_currentContent->addNewLine();
		}
		UpdateScrollBars();
		ScrollPages(1);
		// TODO
	}
	else if(opt==3) // Erase Saved Lines (xterm)
	{
std::cout << "Erase saved lines (xterm)" << std::endl;
		 // TODO
	}	
}

void wxTerminalCtrl::onDECSED(unsigned short opt)  // Erase in Display. 0 → Selective Erase Below (default). 1 → Selective Erase Above. 2 → Selective Erase All. 3 → Selective Erase Saved Lines (xterm).
{
	std::cout << "onDECSED" << std::endl;
}

void wxTerminalCtrl::onEL(unsigned short opt) // Erases part of the line. -- Mostly done
{
	wxPoint pos = GetCaretPosInBuffer();
	wxTerminalLine& line = m_currentContent->at(pos.y);
	if(opt==0) // Clear caret and after
		line.erase(line.begin()+pos.x, line.end());
	else if(opt==1) // Clear caret and before
		{} // TODO
	else// Clear caret line
		line.clear();
}

void wxTerminalCtrl::onDECSEL(unsigned short nb)  // Erase in Line. 0 → Selective Erase to Right (default). 1 → Selective Erase to Left. 2 → Selective Erase All.
{
	std::cout << "onDECSEL" << std::endl;
}

void wxTerminalCtrl::onIL(unsigned short nb)  // Insert Ps Line(s) (default = 1)
{
	std::cout << "onIL" << std::endl;
}

void wxTerminalCtrl::onDL(unsigned short nb)  // Delete Ps Line(s) (default = 1)
{
	std::cout << "onDL" << std::endl;
}

void wxTerminalCtrl::onDCH(unsigned short nb) // Delete Ps Character(s) (default = 1)
{
	std::cout << "onDCH" << std::endl;
}

void wxTerminalCtrl::onSU(unsigned short nb)  // Scroll up Ps lines (default = 1)
{
	std::cout << "onSU" << std::endl;
}

void wxTerminalCtrl::onSD(unsigned short nb)  // Scroll down Ps lines (default = 1)
{
	std::cout << "onSD" << std::endl;
}

void wxTerminalCtrl::onECH(unsigned short nb)  // Erase Ps Character(s) (default = 1)
{
	std::cout << "onECH" << std::endl;
}

void wxTerminalCtrl::onCBT(unsigned short nb)  // Cursor Backward Tabulation Ps tab stops (default = 1)
{
	std::cout << "onCBT" << std::endl;
}

void wxTerminalCtrl::onHPA(const std::vector<unsigned short> nbs)  // Character Position Absolute [column] (default = [row,1]) (HPA).
{
	std::cout << "onHPA" << std::endl;
}

void wxTerminalCtrl::onHPR(const std::vector<unsigned short> nbs)  // Character Position Relative [columns] (default = [row,col+1]) (HPR).
{
	std::cout << "onHPR" << std::endl;
}

void wxTerminalCtrl::onVPA(const std::vector<unsigned short> nbs)  // Line Position Absolute [row] (default = [1,column])
{
	std::cout << "onVPA" << std::endl;
}

void wxTerminalCtrl::onVPR(const std::vector<unsigned short> nbs)  // Line Position Relative [rows] (default = [row+1,column]) (VPR)
{
	std::cout << "onSonVPR" << std::endl;
}

void wxTerminalCtrl::onHVP(unsigned short row, unsigned short col) // Horizontal and Vertical Position [row;column] (default = [1,1])
{
	std::cout << "onHVP" << std::endl;
}

void wxTerminalCtrl::onTBC(unsigned short nb)  // Tab Clear
{
	std::cout << "onTBC" << std::endl;
}

void wxTerminalCtrl::onSM(const std::vector<unsigned short> nbs)  // Set Mode
{
	std::cout << "onSM" << std::endl;
}

void wxTerminalCtrl::onDECSET(const std::vector<unsigned short> nbs)  // DEC Private Mode Set
{
	std::cout << "onDECSET" << std::endl;
}

void wxTerminalCtrl::onMC(const std::vector<unsigned short> nbs)  // Media Copy
{
	std::cout << "onMC" << std::endl;
}

void wxTerminalCtrl::onDECMC(const std::vector<unsigned short> nbs)  // DEC specific Media Copy
{
	std::cout << "onDECMC" << std::endl;
}

void wxTerminalCtrl::onRM(const std::vector<unsigned short> nbs)  // Reset Mode
{
	std::cout << "onRM" << std::endl;
}

void wxTerminalCtrl::onDECRST(const std::vector<unsigned short> nbs)  // DEC Private Mode Reset
{
	std::cout << "onDECRST" << std::endl;
}

void wxTerminalCtrl::onDSR(unsigned short nb)  // Device Status Report
{
	std::cout << "onDSR" << std::endl;
}

void wxTerminalCtrl::onSGR(const std::vector<unsigned short> nbs) // Select Graphic Renditions -- In progress
{
	for(size_t n=0; n<nbs.size(); ++n)
	{
		unsigned short sgr = nbs[n]; 
		switch(sgr)
		{
		case 0: // Default
			m_lastForeColor = 7;
			m_lastBackColor = 0;
			m_lastStyle = wxTCS_Normal;
			break;
		case 1: // Bold
			m_lastStyle |= wxTCS_Bold;
			break;
		case 4: // Underline
			m_lastStyle |= wxTCS_Underlined;
			break;
		case 5: // Blink
			m_lastStyle |= wxTCS_Blink;
			break;
		case 7: // Inverse
			m_lastStyle |= wxTCS_Inverse;
			break;
		case 8: // Invisible (hidden)
			m_lastStyle |= wxTCS_Invisible;
			break;
		case 22: // Normal (neither bold nor faint)
			m_lastStyle &= ~wxTCS_Bold;
			break;
		case 24: // Not underlined
			m_lastStyle &= ~wxTCS_Underlined;
			break;
		case 25: // Steady (not blinking)
			m_lastStyle &= ~wxTCS_Blink;
			break;
		case 27: // Positive (not inverse)
			m_lastStyle &= ~wxTCS_Inverse;
			break;
		case 28: // Visible (not hidden)
				m_lastStyle &= ~wxTCS_Invisible;
			break;
		case 30: // Foreground black
		case 31: // Foreground red
		case 32: // Foreground green
		case 33: // Foreground yellow
		case 34: // Foreground blue
		case 35: // Foreground purple
		case 36: // Foreground cyan
		case 37: // Foreground white
			m_lastForeColor = sgr - 30; 
			break;
		case 39: // Foreground default
			m_lastForeColor = 7;
			break;
		case 40: // Background black
		case 41: // Background red
		case 42: // Background green
		case 43: // Background yellow
		case 44: // Background blue
		case 45: // Background purple
		case 46: // Background cyan
		case 47: // Background white
			m_lastBackColor = sgr - 40; 
			break;
		case 49: // ForegBackground default
			m_lastBackColor = 0;
			break;
		default:
			printf("ApplySGR %d\n", sgr);
			break;
		}		
	}
}

void wxTerminalCtrl::onDECDSR(unsigned short nb)  // DEC-specific Device Status Report
{
	std::cout << "onDECDSR" << std::endl;
}

void wxTerminalCtrl::onDECSTR()  // Soft terminal reset
{
	std::cout << "onDECSTR" << std::endl;
}

void wxTerminalCtrl::onDECSCL(unsigned short nb1, unsigned short nb2) // Set conformance level
{
	std::cout << "onDECSCL" << std::endl;
}

void wxTerminalCtrl::onDECRQM(unsigned short nb)  // Request DEC private mode
{
	std::cout << "onDECRQM" << std::endl;
}

void wxTerminalCtrl::onDECLL(unsigned short nb)  // Load LEDs
{
	std::cout << "onDECLL" << std::endl;
}

void wxTerminalCtrl::onDECSCUSR(unsigned short nb)  // Set cursor style (DECSCUSR, VT520).
{
	std::cout << "onDECSCUSR" << std::endl;
}

void wxTerminalCtrl::onDECSCA(unsigned short nb)  // Select character protection attribute (DECSCA).
{
	std::cout << "onDECSCA" << std::endl;
}

void wxTerminalCtrl::onDECSTBM(unsigned short top, unsigned short bottom) // Set Scrolling Region [top;bottom] (default = full size of window)
{
	std::cout << "onDECSTBM" << std::endl;
}

void wxTerminalCtrl::onRDECPMV(const std::vector<unsigned short> nbs)  // Restore DEC Private Mode Values. The value of P s previously saved is restored. P s values are the same as for DECSET.
{
	std::cout << "onRDECPMV" << std::endl;
}

void wxTerminalCtrl::onSDECPMV(const std::vector<unsigned short> nbs)  // Save DEC Private Mode Values. P s values are the same as for DECSET.
{
	std::cout << "onSDECPMV" << std::endl;
}

void wxTerminalCtrl::onDECCARA(const std::vector<unsigned short> nbs)  // Change Attributes in Rectangular Area (DECCARA), VT400 and up.
{
	std::cout << "onDECCARA" << std::endl;
}

void wxTerminalCtrl::onDECSLRM(unsigned short left, unsigned short right) // Set left and right margins (DECSLRM), available only when DECLRMM is enabled (VT420 and up).
{
	std::cout << "onDECSLRM" << std::endl;
}

void wxTerminalCtrl::onANSISC()  // Save cursor (ANSI.SYS), available only when DECLRMM is disabled.
{
	std::cout << "onANSISC" << std::endl;
}

void wxTerminalCtrl::onANSIRC()  // Restore cursor (ANSI.SYS).
{
	std::cout << "onANSIRC" << std::endl;
}

void wxTerminalCtrl::onWindowManip(unsigned short nb1, unsigned short nb2, unsigned short nb3) // Set conformance level
{
	std::cout << "onWindowManip" << std::endl;
}

void wxTerminalCtrl::onDECRARA(const std::vector<unsigned short> nbs)  // Reverse Attributes in Rectangular Area (DECRARA), VT400 and up.
{
	std::cout << "onDECRARA" << std::endl;
}

void wxTerminalCtrl::onDECSWBV(unsigned short nb)  // Set warning-bell volume (DECSWBV, VT520).
{
	std::cout << "onDECSWBV" << std::endl;
}

void wxTerminalCtrl::onDECSMBV(unsigned short nb)  // Set margin-bell volume (DECSMBV, VT520).
{
	std::cout << "onDECSMBV" << std::endl;
}

void wxTerminalCtrl::onDECCRA(const std::vector<unsigned short> nbs)  // Copy Rectangular Area (DECCRA, VT400 and up).
{
	std::cout << "onDECCRA" << std::endl;
}

void wxTerminalCtrl::onDECEFR(unsigned short top, unsigned short left, unsigned short bottom, unsigned short right) // Enable Filter Rectangle (DECEFR), VT420 and up.
{
	std::cout << "onDECEFR" << std::endl;
}

void wxTerminalCtrl::onDECREQTPARM(unsigned short nb)  // Request Terminal Parameters
{
	std::cout << "onDECREQTPARM" << std::endl;
}

void wxTerminalCtrl::onDECSACE(unsigned short nb)  // Select Attribute Change Extent
{
	std::cout << "onDECSACE" << std::endl;
}

void wxTerminalCtrl::onDECFRA(unsigned short chr, unsigned short top, unsigned short left, unsigned short bottom, unsigned short right) // Fill Rectangular Area (DECFRA), VT420 and up.
{
	std::cout << "onDECFRA" << std::endl;
}

void wxTerminalCtrl::onDECRQCRA(unsigned short id, unsigned short page, unsigned short top, unsigned short left, unsigned short bottom, unsigned short right) // Request Checksum of Rectangular Area (DECRQCRA), VT420 and up.
{
	std::cout << "onDECRQCRA" << std::endl;
}

void wxTerminalCtrl::onDECELR(unsigned short nb1, unsigned short nb2) // Enable Locator Reporting (DECELR).
{
	std::cout << "onDECELR" << std::endl;
}

void wxTerminalCtrl::onDECERA(unsigned short top, unsigned short left, unsigned short bottom, unsigned short right) // Erase Rectangular Area (DECERA), VT400 and up.
{
	std::cout << "onDECERA" << std::endl;
}

void wxTerminalCtrl::onDECSLE(const std::vector<unsigned short> nbs)  // Select Locator Events (DECSLE).
{
	std::cout << "onDECSLE" << std::endl;
}

void wxTerminalCtrl::onDECSERA(unsigned short top, unsigned short left, unsigned short bottom, unsigned short right) // Selective Erase Rectangular Area (), VT400 and up.
{
	std::cout << "onDECSERA" << std::endl;
}

void wxTerminalCtrl::onDECRQLP(unsigned short nb)  // Request Locator Position (DECRQLP).
{
	std::cout << "onDECRQLP" << std::endl;
}

void wxTerminalCtrl::onDECIC(unsigned short nb)  // Insert P s Column(s) (default = 1) (DECIC), VT420 and up.
{
	std::cout << "onDECIC" << std::endl;
}

void wxTerminalCtrl::onDECDC(unsigned short nb)  // InsDelete P s Column(s) (default = 1) (DECIC), VT420 and up.
{
	std::cout << "onDECDC" << std::endl;
}

void wxTerminalCtrl::onOSC(unsigned short command, const std::vector<unsigned char>& params) // Receive an OSC (Operating System Command) command. -- In progress
{
	switch(command)
	{
		case 0: // Change Icon Name and Window Title to Pt.
		case 2: // Change Window Title to Pt.
		{
			wxString str((const char*)&params.front(), params.size());
			printf("Change title: \"%s\"\n", (const char*)str.c_str());
			break;
		}
		default:
			// TODO Add others
			printf("Unknown OSC 0x%04X\n", (int)command);
			break;
	}
}


