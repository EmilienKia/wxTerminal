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

#include <cstring>

#include "terminal-ctrl.hpp"

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
		line.resize(c+1);
}


//
//
// wxTerminalContent
//
//

wxTerminalContent::wxTerminalContent(size_t maxLineSize, size_t maxLineCount):
m_maxLineSize(0),
m_maxLineCount(0)
{
	setMaxLineCount(maxLineCount);
	setMaxLineSize(maxLineSize);
	addNewLine();
}

wxTerminalContent::~wxTerminalContent()
{
}

void wxTerminalContent::setMaxLineSize(size_t maxLineSize)
{
	m_maxLineSize = maxLineSize;
	// TODO Change current (back) line size.
}

void wxTerminalContent::setMaxLineCount(size_t maxLineCount)
{
	m_maxLineCount = maxLineCount;
	removeExtraLines();
}





void wxTerminalContent::removeExtraLines()
{
	if(m_maxLineCount>0 && size()>m_maxLineCount)
	{
		erase(begin(), begin()+(size()-m_maxLineCount));
	}
}

void wxTerminalContent::addNewLine()
{
	static wxTerminalLine line;
	push_back(line);
	removeExtraLines();
}

void wxTerminalContent::setChar(wxPoint pos, wxChar c, char fore, char back, unsigned char style)
{
	static wxTerminalLine line;
	wxTerminalCharacter ch;
	ch.c = c;
	ch.fore  = fore;
	ch.back  = back;
	ch.style = style;
	setChar(pos, ch);

}

void wxTerminalContent::setChar(wxPoint pos, wxTerminalCharacter c)
{
	if(pos.y<=size())
		resize(pos.y+1);

	wxTerminalLine& line = at(pos.y);

	if(pos.x<=line.size())
		line.resize(pos.x+1);

	line[pos.x] = c;
}

void wxTerminalContent::append(wxChar c, char fore, char back, unsigned char style)
{
	wxTerminalCharacter ch;
	ch.c = c;
	ch.fore  = fore;
	ch.back  = back;
	ch.style = style;
	append(ch);
}

void wxTerminalContent::append(const wxTerminalCharacter& c)
{
	if(!isLineInfinite() && front().size()>=getMaxLineSize())
		addNewLine();
	back().push_back(c);
}



//
//
// wxTerminalCtrl
//
//


wxString wxTerminalCtrlNameStr(wxT("wxTerminalCtrl"));

wxBEGIN_EVENT_TABLE(wxTerminalCtrl, wxScrolledCanvas)
	EVT_PAINT(wxTerminalCtrl::OnPaint)
	EVT_SIZE(wxTerminalCtrl::OnSize)
	EVT_SCROLLWIN(wxTerminalCtrl::OnScroll)
	EVT_CHAR(wxTerminalCtrl::OnChar)
	EVT_TIMER(wxID_ANY, wxTerminalCtrl::OnTimer)
wxEND_EVENT_TABLE()

wxTerminalCtrl::wxTerminalCtrl(wxWindow *parent, wxWindowID id, const wxPoint &pos,
    const wxSize &size, long style, const wxString &name):
wxScrolledCanvas(parent, id, pos, size, style, name),
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
	if(!wxScrolledCanvas::Create(parent, id, pos, size, style|wxVSCROLL|wxHSCROLL, name))
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

	m_clSizeChar = wxSize(80, 25);

	GenerateFonts(wxFont(10, wxFONTFAMILY_TELETYPE));
	SetBackgroundStyle(wxBG_STYLE_PAINT);
	SetBackgroundColour(m_colours[0]);

	m_lastBackColor = 0;
	m_lastForeColor = 7;
	m_lastStyle     = wxTCS_Normal;

	
	EnableScrolling(false, false);
	UpdateScrollBars();
	
	m_timer = new wxTimer(this);
	m_timer->Start(10);

	m_caret = new wxCaret(this, GetCharSize());
	m_caret->Show();
	SetCaretPosition(0, 0);
}

void wxTerminalCtrl::GenerateFonts(const wxFont& font)
{
	m_defaultFont   = font;
	m_boldFont      = font.Bold();
	m_underlineFont = font.Underlined();
	m_boldUnderlineFont = m_boldFont.Underlined();
}

void wxTerminalCtrl::SetCaretPosition(wxPoint pos)
{
	if(pos.x==-1)
		pos.x = m_caretPos.x;
	if(pos.y==-1)
		pos.y = m_caretPos.y;
	m_caretPos = pos;
	wxSize sz = GetCharSize();
//printf("SetCaretPosition %d,%d\n", m_caretPos.x, m_caretPos.y);
	m_caret->Move(sz.x*pos.x, sz.y*pos.y);
}

void wxTerminalCtrl::MoveCaret(int x, int y)
{
//printf("MoveCaret %d,%d - %d,%d\n", m_caretPos.x, m_caretPos.y, x, y);
	m_caretPos.x += x;
	m_caretPos.y += y;

	if(m_caretPos.x<0)
	{
		while(m_caretPos.x<0)
		{
			m_caretPos.y -= 1;
			m_caretPos.x += m_currentContent->at(m_caretPos.y + GetScrollPos(wxVERTICAL)).size()-1;
		}
	}
	// TODO else {}

	if(m_caretPos.y<0)
		m_caretPos.y = 0;
	// TODO else {}

	SetCaretPosition(-1, -1);
}

wxPoint wxTerminalCtrl::GetCaretPosInBuffer()const
{
	wxPoint pos = GetCaretPosition();
	pos.y += GetScrollPos(wxVERTICAL);
	return pos;
}

wxSize wxTerminalCtrl::GetCharSize(wxChar c)const
{
	wxSize sz;
	GetTextExtent(c, &sz.x, &sz.y, NULL, NULL, &m_defaultFont);
	return sz;
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
	
	wxPoint scroll(0, GetScrollPos(wxVERTICAL));
	
	wxConsoleContent* content = m_currentContent;
	if(content)
	{
		for(size_t n=0, l=GetScrollPos(wxVERTICAL); n<clchSz.y && l<content->size(); n++, l++)
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
	Refresh();
	event.Skip();
}

void wxTerminalCtrl::OnSize(wxSizeEvent& event)
{
	wxSize sz = GetClientSize();
	wxSize ch = GetCharSize();
	m_clSizeChar = wxSize(sz.x/ch.x, sz.y/ch.y);
	
	UpdateScrollBars();
}

void wxTerminalCtrl::UpdateScrollBars()
{
	wxConsoleContent* content = m_currentContent;
	wxSize charSz = GetCharSize();
	wxSize globalSize(0, content?content->size():0);
	SetScrollRate(charSz.x, charSz.y);
	SetVirtualSize(0, globalSize.y*charSz.y);
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

void wxTerminalCtrl::SetChar(wxChar c)
{
	wxConsoleContent* content = m_currentContent;
	if(!content)
		return;

	wxPoint pos = GetCaretPosition();
	pos.y += GetScrollPos(wxVERTICAL);
	
	content->setChar(pos, c, m_lastForeColor, m_lastBackColor, m_lastStyle);
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

void wxTerminalCtrl::onBEL()
{
	wxBell();
	printf("Bell !!!\n");
}

void wxTerminalCtrl::onBS()
{
	MoveCaret(-1);
}

void wxTerminalCtrl::onLF()   // 0x0A - LINE FEED
{
	SetChar(0x0A);
	SetCaretPosition(0, GetCaretPosition().y+1);
}

void wxTerminalCtrl::onCR()   // 0x0D - CARIAGE RETURN
{
	SetChar(0x0D);
	SetCaretPosition(0, GetCaretPosition().y+1);
}

void wxTerminalCtrl::onCHA(unsigned short nb) // Moves the cursor to column n.
{
	SetCaretPosition(nb - 1, GetCaretPosition().y);
}

void wxTerminalCtrl::onCUP(unsigned short row, unsigned short col) // Moves the cursor to row n, column m
{
	SetCaretPosition(col - 1, row - 1);
}

void wxTerminalCtrl::onED(unsigned short opt) // Clears part of the screen.
{
	wxPoint pos = GetCaretPosInBuffer();
	if(opt==0) // Erase Below
		{} // TODO
	else if(opt==1) // Erase Above
		{} // TODO
	else if(opt==2) // Erase All
		{} // TODO
	else if(opt==3) // Erase Saved Lines (xterm)
		{} // TODO	
}

void wxTerminalCtrl::onEL(unsigned short opt) // Erases part of the line.
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

void wxTerminalCtrl::onSGR(const std::vector<unsigned short> nbs) // Select Graphic Renditions
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

void wxTerminalCtrl::onOSC(unsigned short command, const std::vector<unsigned char>& params)
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


