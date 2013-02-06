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
// wxTerminalParser
//
// This is the base state machine analyser from http://vt100.net/emu/dec_ansi_parser
//
// Notes:
//  - sci_dispatch calls have been replaced by direct calls to "onCSI(c, m_params, m_collected);"
//  - a difference is done between esc_dispatch with one and more (two) characters
//    - for one-char, calling esc_dispatch directly
//    - for two-char, collecting in m_escapedFirstChar (instead of m_collected) and call two param version of esc_dispatch. 
//    - esc_dispatch calls have been replaced by direct calls to onESC(...)
//  - OSC is parsed differently from osc's start/put/end, the first number is parsed and the rest is buffered as is.
//
// Other useful pointers:
//  - http://vt100.net/emu/
//  - http://invisible-island.net/xterm/ctlseqs/ctlseqs.html
//  - http://rtfm.etla.org/xterm/ctlseq.html
//  - http://www.inwap.com/pdp10/ansicode.txt
//  - http://man.he.net/man4/console_codes
//
// TODO Add mouse support (CSI Ps ; Ps ; Ps ; Ps ; Ps T)
// TODO Add feature reseting (CSI > Ps ; Ps T)
// TODO Add Primary and Secondary Device Attribute  (CSI Ps c) (CSI > Ps c)
// TODO Add Set one or more features of the title modes (CSI > Ps ; Ps t)
// TODO Add support fo real SGR decoding instead of delegating to upper layers
// TODO Add support fo real OSC decoding instead of delegating to upper layers
//
//

wxTerminalParser::wxTerminalParser():
m_state(WXTP_STATE_GROUND)
{
}

void wxTerminalParser::Process(unsigned char c)
{
	// Parse "Anywhere" entries
	// {
	if( c==0x18 || c==0x1A || c==0x1B )
	{
		// Execute (transition is done in default handler when needed).
		executeC0ControlCode(c);
		return;
	}
	else if( c>=0x80 && c<=0x9F )
	{
		// Execute (transition is done in default handler when needed).
		executeC1ControlCode(c);
		return;
	}

	// Real state dependant processing
	switch(m_state)
	{
		case WXTP_STATE_GROUND:
		{
			if(c <= 0x1F) // except 18, 1A, 1B, but already processed
			{
				// execute
				executeC0ControlCode(c);
			}
			else if(/* c >= 0x20 && */ c <= 0x7F) // ASCII printable character
			{
				// print
				onPrintableChar(c);
			}
			//  c>= 0x80 && c <= 0x9F : Globally processed
			else // Extended character / GR Area ( c >= 0xA0 && c <= FF ) => GL  
			{
				// print
				onPrintableChar(c);				
			}
			break;
		}
		case WXTP_STATE_ESCAPE:
		{
			if(c <= 0x1F) // except 18, 1A, 1b, but already processed
			{
				// execute
				executeC0ControlCode(c);
			}
			else if(c <= 0x2F)
			{
				// collect // Special char collect
				m_escapedFirstChar = c;
				// goto WXTP_STATE_ESCAPE_INTERMEDIATE
				Transition(WXTP_STATE_ESCAPE_INTERMEDIATE);
			}
			else if(c == 0x50)
			{
				// goto WXTP_STATE_DCS_ENTRY
				Transition(WXTP_STATE_DCS_ENTRY);
			}
			else if(c == 0x58 || c == 0x5E || c == 0x5F)
			{
				// goto WXTP_STATE_SOS_PM_APC_STRING
				Transition(WXTP_STATE_SOS_PM_APC_STRING);
			}
			else if(c == 0x5B)
			{
				// goto WXTP_STATE_CSI_ENTRY
				Transition(WXTP_STATE_CSI_ENTRY);
			}
			else if(c == 0x5D)
			{
				// goto WXTP_STATE_OSC_ENRTY
				Transition(WXTP_STATE_OSC_ENTRY);
			}
			else if(c == 0x7F)
			{
				// Ignore
			}
			else
			{
				// esc_dispatch // Direct call to onESC(cmd)
				onESC(c);
				// EKI What about GR Area ???
				// goto WXTP_STATE_GROUND
				Transition(WXTP_STATE_GROUND);
			}
			break;
		}
		case WXTP_STATE_ESCAPE_INTERMEDIATE:
		{
			if(c <= 0x1F) // except 18, 1A, 1b, but already processed
			{
				// execute
				executeC0ControlCode(c);
			}
			else if(c <= 0x2F)
			{
				// collect
				// EKI: escaped sequence cannot have more than two characters ??
				Collect(c);
			}
			else if(c == 0x7F)
			{
				// Ignore
			}
			else
			{
				// esc_dispatch // Direct call to onESC(cmd, param)
				onESC(m_escapedFirstChar, c);
				// EKI What about GR Area ???
				// goto WXTP_STATE_GROUND
				Transition(WXTP_STATE_GROUND);
			}
			break;
		}
		case WXTP_STATE_CSI_ENTRY:
		{
			if(c <= 0x1F) // except 18, 1A, 1b, but already processed
			{
				// execute
				executeC0ControlCode(c);
			}
			else if(c <= 0x2F)
			{
				// collect
				Collect(c);
				// goto WXTP_STATE_CSI_INTERMEDIATE
				Transition(WXTP_STATE_CSI_INTERMEDIATE);
			}
			else if(c == 0x3A)
			{
				// goto WXTP_STATE_CSI_IGNORE
				Transition(WXTP_STATE_CSI_IGNORE);
			}
			else if(c <= 0x3B)
			{
				// param
				Param(c);
				// goto WXTP_STATE_CSI_PARAM
				Transition(WXTP_STATE_CSI_PARAM);
			}
			else if(c <= 0x3F)
			{
				// collect
				Collect(c);
				// goto WXTP_STATE_CSI_PARAM
				Transition(WXTP_STATE_CSI_PARAM);
			}
			else if(c == 0x7F)
			{
				// Ignore
			}
			else
			{
				// csi_dispatch // Direct call to onCSI(cmd, params, m_collected)
				onCSI(c, m_params, m_collected);
				// EKI What about GR Area ???
				// goto WXTP_STATE_GROUND
				Transition(WXTP_STATE_GROUND);
			}
			break;
		}
		case WXTP_STATE_CSI_PARAM:
		{
			if(c <= 0x1F) // except 18, 1A, 1b, but already processed
			{
				// execute
				executeC0ControlCode(c);
			}
			else if(c <= 0x2F)
			{
				// collect
				Collect(c);
				// goto WXTP_STATE_CSI_INTERMEDIATE
				Transition(WXTP_STATE_CSI_INTERMEDIATE);
			}
			else if(c <= 0x39 || c == 0x3B)
			{
				// param
				Param(c);
			}
			else if(c <= 0x3F)
			{
				// goto WXTP_STATE_CSI_IGNORE
				Transition(WXTP_STATE_CSI_IGNORE);
			}
			else if(c == 0x7F)
			{
				// Ignore
			}
			else
			{
				// csi_dispatch
				onCSI(c, m_params, m_collected);
				// EKI What about GR Area ???
				// goto WXTP_STATE_GROUND
				Transition(WXTP_STATE_GROUND);
			}
			break;
		}
		case WXTP_STATE_CSI_INTERMEDIATE:
		{
			if(c <= 0x1F) // except 18, 1A, 1b, but already processed
			{
				// execute
				executeC0ControlCode(c);
			}
			else if(c <= 0x2F)
			{
				// collect
				Collect(c);
			}
			else if(c <= 0x3F)
			{
				// goto WXTP_STATE_CSI_IGNORE
				Transition(WXTP_STATE_CSI_IGNORE);
			}
			else if(c == 0x7F)
			{
				// Ignore
			}
			else
			{
				// csi_dispatch
				onCSI(c, m_params, m_collected);
				// EKI What about GR Area ???
				// goto WXTP_STATE_GROUND
				Transition(WXTP_STATE_GROUND);
			}
			break;
		}
		case WXTP_STATE_CSI_IGNORE:
		{
			if(c <= 0x1F) // except 18, 1A, 1b, but already processed
			{
				// execute
				executeC0ControlCode(c);
			}
			else if(c <= 0x3F || c == 0x7F)
			{
				// ignore
			}
			else
			{// EKI What about GR Area ???
				// goto WXTP_STATE_GROUND
				Transition(WXTP_STATE_GROUND);
			}			
			break;
		}
		case WXTP_STATE_DCS_ENTRY:
		{
			if(c <= 0x1F) // except 18, 1A, 1b, but already processed
			{
				// ignore
			}
			else if(c <= 0x2F)
			{
				// collect
				Collect(c);
				// goto WXTP_STATE_DCS_INTERMEDIATE
				Transition(WXTP_STATE_DCS_INTERMEDIATE);
			}
			else if(c == 0x3A)
			{
				// goto WXTP_STATE_DCS_IGNORE
				Transition(WXTP_STATE_DCS_IGNORE);
			}
			else if(c <= 0x3B)
			{
				// param
				Param(c);
				// goto WXTP_STATE_DCS_PARAM
				Transition(WXTP_STATE_DCS_PARAM);
			}
			else if(c <= 0x3F)
			{
				// collect
				Collect(c);
				// goto WXTP_STATE_DCS_PARAM
				Transition(WXTP_STATE_DCS_PARAM);
			}
			else if(c == 0x7F)
			{
				// Ignore
			}
			else
			{// EKI What about GR Area ???
				// goto WXTP_STATE_DCS_PASSTHROUGH
				Transition(WXTP_STATE_DCS_PASSTHROUGH);
			}
			break;
		}
		case WXTP_STATE_DCS_PARAM:
		{
			if(c <= 0x1F) // except 18, 1A, 1b, but already processed
			{
				// ignore
			}
			else if(c <= 0x2F)
			{
				// collect
				Collect(c);
				// goto WXTP_STATE_DCS_INTERMEDIATE
				Transition(WXTP_STATE_DCS_INTERMEDIATE);
			}
			else if(c <= 0x39 || c == 0x3B)
			{
				// param
				Param(c);
			}
			else if(c <= 0x3F)
			{
				// goto WXTP_STATE_DCS_IGNORE
				Transition(WXTP_STATE_DCS_IGNORE);
			}
			else if(c == 0x7F)
			{
				// Ignore
			}
			else
			{// EKI What about GR Area ???
				// goto WXTP_STATE_DCS_PASSTHROUGH
				Transition(WXTP_STATE_DCS_PASSTHROUGH);
			}
			break;
		}
		case WXTP_STATE_DCS_INTERMEDIATE:
		{
			if(c <= 0x1F) // except 18, 1A, 1b, but already processed
			{
				// ignore
			}
			else if(c <= 0x2F)
			{
				// collect
				Collect(c);
			}
			else if(c <= 0x3F)
			{
				// goto WXTP_STATE_DCS_IGNORE
				Transition(WXTP_STATE_DCS_IGNORE);
			}
			else if(c == 0x7F)
			{
				// Ignore
			}
			else
			{// EKI What about GR Area ???
				// goto WXTP_STATE_DCS_PASSTHROUGH
				Transition(WXTP_STATE_DCS_PASSTHROUGH);
			}
			break;
		}
		case WXTP_STATE_DCS_PASSTHROUGH:
		{
			if(c == 0x7F)
			{
				// Ignore
			}
			else if(c == 0x9C)
			{
				// goto WXTP_STATE_GROUND
				Transition(WXTP_STATE_GROUND);
			}
			else
			{
				// put
				DcsPut(c);
			}
			// EKI What about GR Area ???
			break;
		}
		case WXTP_STATE_DCS_IGNORE:
		{
			if(c == 0x9C)
			{
				// goto WXTP_STATE_GROUND
				Transition(WXTP_STATE_GROUND);
			}
			else
			{
				// ignore
			}
			// EKI What about GR Area ???
			break;
		}
		case WXTP_STATE_OSC_ENTRY:
		{
			if( c >= '0' && c <= '9' ) // 0x30 - 0x39
			{
				Param(c);
			}
			else if( c == 0x9C || c == 0x07 ) // ST || BEL
			{
				// goto WXTP_STATE_GROUND
				Transition(WXTP_STATE_GROUND);
			}
			else if( c == ';' ) // 0x3B
			{
				// goto WXTP_STATE_OSC_STRING
				Transition(WXTP_STATE_OSC_STRING);
			}
			else
			{
				// collect
				Collect(c);
				// goto WXTP_STATE_OSC_STRING
				Transition(WXTP_STATE_OSC_STRING);
			}
			break;
		}
		case WXTP_STATE_OSC_STRING:
		{
			if(c <= 0x1F) // except 18, 1A, 1b, but already processed
			{
				// ignore
			}
			else if(c == 0x9C || c == 0x07 ) // ST || BEL
			{
				// goto WXTP_STATE_GROUND
				Transition(WXTP_STATE_GROUND);
			}
			else //if(c <= 0x7F)
			{
				// ocs_put
				Collect(c);
			}
			// EKI What about GR Area ???
			break;
		}
		case WXTP_STATE_SOS_PM_APC_STRING:
		{
			if(c == 0x9C)
			{
				// goto WXTP_STATE_GROUND
				Transition(WXTP_STATE_GROUND);
			}
			else
			{
				// ignore
			}
			// EKI What about GR Area ???
			break;
		}
		default:
			/* Must not occurs ! */
			break;
	}
}

void wxTerminalParser::Transition(WXTP_STATE state)
{
	// Exit old state
	switch(m_state)
	{
		case WXTP_STATE_DCS_PASSTHROUGH:
			DcsUnhook();
			break;
		case WXTP_STATE_OSC_ENTRY:
		case WXTP_STATE_OSC_STRING:
			if(m_params.size()>0)
				onOSC(m_params[0], m_collected);
			break;
		case WXTP_STATE_GROUND:
		case WXTP_STATE_ESCAPE:
		case WXTP_STATE_ESCAPE_INTERMEDIATE:
		case WXTP_STATE_CSI_ENTRY:
		case WXTP_STATE_CSI_PARAM:
		case WXTP_STATE_CSI_INTERMEDIATE:
		case WXTP_STATE_CSI_IGNORE:
		case WXTP_STATE_DCS_ENTRY:
		case WXTP_STATE_DCS_PARAM:
		case WXTP_STATE_DCS_INTERMEDIATE:
		case WXTP_STATE_DCS_IGNORE:
		case WXTP_STATE_SOS_PM_APC_STRING:
		default:
			break; /* Nothing specified. */
	}

	m_state = state;
	
	// Enter new state
	switch(m_state)
	{
		case WXTP_STATE_ESCAPE:
		case WXTP_STATE_CSI_ENTRY:
		case WXTP_STATE_DCS_ENTRY:
		case WXTP_STATE_OSC_ENTRY:
			Clear();
			break;
		case WXTP_STATE_DCS_PASSTHROUGH:
			DcsHook();
			break;
		case WXTP_STATE_OSC_STRING:
		case WXTP_STATE_GROUND:
		case WXTP_STATE_ESCAPE_INTERMEDIATE:
		case WXTP_STATE_CSI_PARAM:
		case WXTP_STATE_CSI_INTERMEDIATE:
		case WXTP_STATE_CSI_IGNORE:
		case WXTP_STATE_DCS_PARAM:
		case WXTP_STATE_DCS_INTERMEDIATE:
		case WXTP_STATE_DCS_IGNORE:
		case WXTP_STATE_SOS_PM_APC_STRING:
		default:
			break; /* Nothing specified. */
	}
}


void wxTerminalParser::executeC0ControlCode(unsigned char c)
{
	switch(c)
	{
		case 0x00: onNUL();	break;
		case 0x01: onSOH();	break;
		case 0x02: onSTX(); break;
		case 0x03: onETX(); break;
		case 0x04: onEOT(); break;
		case 0x05: onENQ(); break;
		case 0x06: onACK(); break;
		case 0x07: onBEL(); break;
		case 0x08: onBS(); break;
		case 0x09: onHT(); break;
		case 0x0A: onLF(); break;
		case 0x0B: onVT(); break;
		case 0x0C: onFF(); break;
		case 0x0D: onCR(); break;
		case 0x0E: onSO(); break;
		case 0x0F: onSI(); break;
		case 0x10: onDLE(); break;
		case 0x11: onDC1(); break;
		case 0x12: onDC2(); break;
		case 0x13: onDC3(); break;
		case 0x14: onDC4(); break;
		case 0x15: onNAK(); break;
		case 0x16: onSYN(); break;
		case 0x17: onETB(); break;
		case 0x18: onCAN(); break;
		case 0x19: onEM(); break;
		case 0x1A: onSUB(); break;
		case 0x1B: onESC(); break;
		case 0x1C: onFS(); break;
		case 0x1D: onGS(); break;
		case 0x1E: onRS(); break;
		case 0x1F: onUS(); break;
		default:
			// Must not occur (out of range)
			break;
	}
}

void wxTerminalParser::onCAN()
{
	Transition(WXTP_STATE_GROUND);
}

void wxTerminalParser::onSUB()
{
	Transition(WXTP_STATE_GROUND);
}

void wxTerminalParser::onESC()
{
	Transition(WXTP_STATE_ESCAPE);
}

void wxTerminalParser::executeC1ControlCode(unsigned char c)
{
	switch(c)
	{
		case 0x80: onPAD(); break;
		case 0x81: onHOP(); break;
		case 0x82: onBPH(); break;
		case 0x83: onNBH(); break;
		case 0x84: onIND(); break;
		case 0x85: onNEL(); break;
		case 0x86: onSSA(); break;
		case 0x87: onESA(); break;
		case 0x88: onHTS(); break;
		case 0x89: onHTJ(); break;
		case 0x8A: onVTS(); break;
		case 0x8B: onPLD(); break;
		case 0x8C: onPLU(); break;
		case 0x8D: onRI(); break;
		case 0x8E: onSS2(); break;
		case 0x8F: onSS3(); break;
		case 0x90: onDCS(); break;
		case 0x91: onPU1(); break;
		case 0x92: onPU2(); break;
		case 0x93: onSTS(); break;
		case 0x94: onCCH(); break;
		case 0x95: onMW(); break;
		case 0x96: onSPA(); break;
		case 0x97: onEPA(); break;
		case 0x98: onSOS(); break;
		case 0x99: onSGCI(); break;
		case 0x9A: onSCI(); break;
		case 0x9B: onCSI(); break;
		case 0x9C: onST(); break;
		case 0x9D: onOSC(); break;
		case 0x9E: onPM(); break;
		case 0x9F: onAPC(); break;
		default:
			// Must not occur (out of range)
			break;		
	}
}

void wxTerminalParser::onDCS()
{
	Transition(WXTP_STATE_DCS_ENTRY);
}

void wxTerminalParser::onSOS()
{
	Transition(WXTP_STATE_SOS_PM_APC_STRING);
}

void wxTerminalParser::onCSI()
{
	Transition(WXTP_STATE_CSI_ENTRY);
}

void wxTerminalParser::onST()
{
	Transition(WXTP_STATE_GROUND);
}

void wxTerminalParser::onOSC()
{
	Transition(WXTP_STATE_OSC_STRING);
}

void wxTerminalParser::onPM()
{
	Transition(WXTP_STATE_SOS_PM_APC_STRING);
}

void wxTerminalParser::onAPC()
{
	Transition(WXTP_STATE_SOS_PM_APC_STRING);
}

void wxTerminalParser::Clear()
{
	m_collected.clear();
	m_params.clear();
}

void wxTerminalParser::Collect(unsigned char c)
{
	m_collected.push_back(c);
}

void wxTerminalParser::Param(unsigned char c)
{
	if( c == ';' ) // 0x3B
	{
		m_params.push_back(0);
	}
	else if( c >= '0' && c <= '9' ) // 0x30 - 0x39
	{
		if(m_params.empty())
			m_params.push_back(0);
		m_params.back() = m_params.back() * 10 + (c - '0');
	}
}




void wxTerminalParser::DcsHook()
{
	/* TODO DCS is not implemented yet. */
}

void wxTerminalParser::DcsUnhook()
{
	/* TODO DCS is not implemented yet. */
}

void wxTerminalParser::DcsPut(unsigned char c)
{
	/* TODO DCS is not implemented yet. */
}

void wxTerminalParser::onOSC(unsigned short command, const std::vector<unsigned char>& params)
{
	
}

void wxTerminalParser::onCSI(unsigned char command, const std::vector<unsigned short>& params, const std::vector<unsigned char>& collect)
{
	// TODO Add vector size test for each command and error handling if not.
	switch(command)
	{
		case '@': onICH(params.empty()?1:params[0]); break;
		case 'A': onCUU(params.empty()?1:params[0]); break;
		case 'B': onCUD(params.empty()?1:params[0]); break;
		case 'C': onCUF(params.empty()?1:params[0]); break;
		case 'D': onCUB(params.empty()?1:params[0]); break;
		case 'E': onCNL(params.empty()?1:params[0]); break;
		case 'F': onCPL(params.empty()?1:params[0]); break;
		case 'G': onCHA(params.empty()?1:params[0]); break;
		case 'H': onCUP(params.size()<1?1:params[0], params.size()<2?1:params[1]); break;
		case 'I': onCHT(params.empty()?1:params[0]); break;
		case 'J':
			if(collect.size()>0 && collect[0]=='?')
				onDECSED(params.empty()?0:params[0]);
			else
				onED(params.empty()?0:params[0]);
			break;
		case 'K':
			if(collect.size()>0 && collect[0]=='?')
				onDECSEL(params.empty()?0:params[0]);
			else
				onEL(params.empty()?0:params[0]);
			break;
		case 'L': onIL(params.empty()?1:params[0]); break;
		case 'M': onDL(params.empty()?1:params[0]); break;
		case 'P': onDCH(params.empty()?1:params[0]); break;
		case 'S': onSU(params.empty()?1:params[0]); break;
		case 'T': onSD(params.empty()?1:params[0]); break;
		case 'X': onECH(params.empty()?1:params[0]); break;
		case 'Z': onCBT(params.empty()?1:params[0]); break;
		case '`': onHPA(params); break;
		case 'a': onHPR(params); break;
		case 'd': onVPA(params); break;
		case 'e': onVPR(params); break;
		case 'f': onHVP(params.size()<1?1:params[0], params.size()<2?1:params[1]); break;
		case 'g': onTBC(params.empty()?0:params[0]); break;
		case 'h':
			if(collect.size()>0 && collect[0]=='?')
				onDECSET(params);
			else
				onSM(params);
			break;
		case 'i':
			if(collect.size()>0 && collect[0]=='?')
				onDECMC(params);
			else
				onMC(params);
			break;
		case 'l':
			if(collect.size()>0 && collect[0]=='?')
				onDECRST(params);
			else
				onRM(params);
			break;
		case 'm': onSGR(params); break;
		case 'n':
			if(collect.size()>0 && collect[0]=='?')
				onDECDSR(params[0]);
			else
				onDSR(params[0]);
			break;
		case 'p':
			if(collect.size()==1)
			{
				if(collect[0]=='!')
					onDECSTR();
				else if(collect[0]=='"')
					onDECSCL(params[0], params[1]);
			}
			else if(collect.size()==2)
			{
				if(collect[0]=='?' && collect[0]=='$')
					onDECRQM(params[0]);
			}
			break;
		case 'q':
			if(collect.size()==0)
				onDECLL(params[0]);
			else if(collect.size()==1)
			{
				if(collect[0]==' ')
					onDECSCUSR(params[0]);
				else if(collect[0]=='"')
					onDECSCA(params[0]);
			}
			break;
		case 'r':
			if(collect.size()==0)
				onDECSTBM(params[0], params[1]);
			else if(collect.size()==1)
			{
				if(collect[0]=='?')
					onRDECPMV(params);
				else if(collect[0]=='$')
					onDECCARA(params);
			}
			break;
		case 's':
			if(collect.size()==0)
			{
				if(params.size()==0)
					onANSISC();
				else if(params.size()>=2)
					onDECSLRM(params[0], params[1]);
			}
			else if(collect[0]=='?')
			{
				onSDECPMV(params);
			}
			break;
		case 't':
			if(collect.size()==0)
			{
				if(params.size()==1)
					onWindowManip(params[0]);
				else if(params.size()==2)
					onWindowManip(params[0], params[1]);
				else if(params.size()==3)
					onWindowManip(params[0], params[1], params[2]);
				else
					onDECRARA(params);
			}
			else if(collect.size()==1)
			{
				if(collect[0]==' ')
					onDECSWBV(params[0]);
			}
			break;
		case 'u':
			if(collect.size()==0)
			{
				onANSIRC();
			}
			else if(collect[0]==' ')
			{
				onDECSMBV(params[0]);
			}
			break;
		case 'v':
			if(collect.size()==1)
			{
				if(collect[0]==' ')
				{
					onDECCRA(params);
				}
			}			 
			break;
		case 'w':
			if(collect.size()==1)
			{
				if(collect[0]=='`')
				{
					onDECEFR(params[0], params[1], params[2], params[3]);
				}
			}
			break;
		case 'x':
			if(collect.size()==0)
			{
				onDECREQTPARM(params[0]);
			}
			else if(collect[0]=='*')
			{
				onDECSACE(params[0]);
			}
			else if(collect[0]=='$')
			{
				onDECFRA(params[0], params[1], params[2], params[3], params[4]);
			}
			break;
		case 'y':
			if(collect.size()==1)
			{
				if(collect[0]=='*')
				{
					onDECRQCRA(params[0], params[1], params[2], params[3], params[5], params[6]);
				}
			}
			break;
		case 'z':
			if(collect.size()==1)
			{
				if(collect[0]=='`')
				{
					onDECELR(params[0], params[1]);
				}
				else if(collect[0]=='$')
				{
					onDECERA(params[0], params[1], params[2], params[3]);
				}
			}
			break;
		case '{':
			if(collect.size()==1)
			{
				if(collect[0]=='`')
				{
					onDECSLE(params);
				}
				else if(collect[0]=='$')
				{
					onDECSERA(params[0], params[1], params[2], params[3]);
				}
			}
			break;
		case '|':
			if(collect.size()==1)
			{
				if(collect[0]=='`')
				{
					onDECRQLP(params[0]);
				}
			}
			break;
		case '}':
			if(collect.size()==1)
			{
				if(collect[0]=='`')
				{
					onDECIC(params[0]);
				}
			}
			break;			
		case '~':
			if(collect.size()==1)
			{
				if(collect[0]=='`')
				{
					onDECDC(params[0]);
				}
			}
			break;
		default:
			/* TODO CSI interpreting is not fully implemented yet. */
			std::cout << "CSI " << (char)command << std::endl; 
			break;
	}
	
}

void wxTerminalParser::onESC(unsigned char command)
{
	switch(command)
	{
		// Traditionnal commands:
		case 'D': onIND(); break;
		case 'E': onNEL(); break;
		case 'H': onHTS(); break;
		case 'M': onRI(); break;
		case 'N': onSS2(); break;
		case 'O': onSS3(); break;
		case 'P': onDCS(); break;
		case 'V': onSPA(); break;
		case 'W': onEPA(); break;
		case 'X': onSOS(); break;
		case 'Z': onSCI(); break; // TODO Validate that!!
		case '\\': onST(); break;
		case '^': onPM(); break;
		case '_': onAPC(); break;
		
		// Advanced commands:
		case '6': onDECBI(); break;
		case '7': onDECSC(); break;
		case '8': onDECRC(); break;
		case '9': onDECFI(); break;
		case '=': onDECKPAM(); break;
		case '>': onDECKPNM(); break;
		case 'c': onRIS(); break;
		case 'n': onLS2(); break;
		case 'o': onLS3(); break;
		case '|': onLS3R(); break;
		case '}': onLS2R(); break;
		case '~': onLS1R(); break;

		// Following commands should be processed elsewhere:			
		case '[': // CSI (Control Sequence Introducer) : Should not pass here !
		case ']': // OSC (Operating System Command) : Should not pass here !
			break;
		
		// HP Extensions:
		case 'F':
		case 'l':
		case 'm':
			// TODO Should I support HP extensions ???
			break;
			
		default:
			break;
	}
}

void wxTerminalParser::onESC(unsigned char command, unsigned char param)
{
	switch(command)
	{
	case ' ': // Conformance and control character set
		switch(param)
		{
			case 'F': onS7C1T(); break;
			case 'G': onS8C1T(); break;
			case 'L': onANSIconf1(); break;
			case 'M': onANSIconf2(); break;
			case 'N': onANSIconf3(); break;
			default:
				// Undefined conformance.
				break;
		}
		break;
	case '#': // DEC specific adjustements
		switch(param)
		{
			case '3': onDECDHLth(); break;
			case '4': onDECDHLbh(); break;
			case '5': onDECSWL(); break;
			case '6': onDECDWL(); break;
			case '8': onDECALN(); break;
			default:
				break;
		}
		break;
	case '%': // Character set
		switch(param)
		{
			case '@': onISO8859_1(); break;
			case 'G': onUTF_8(); break;
			default:
				break;
		}
		break;
	case '(': // Designate G0 charset
		onDesignateG0(param);
	    break;
	case ')': // Designate G1 charset
	case '-':
		onDesignateG1(param);
	    break;
	case '*': // Designate G2 charset
	case '.':
		onDesignateG2(param);
	    break;
	case '+': // Designate G3 charset
	case '/':
		onDesignateG3(param);
	    break;
	default:
		// Invalid, TODO should I call onESC(...) with one char ? 
		break;
	}
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
// Definition of wxTerminalParser interface abstract functions:
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


