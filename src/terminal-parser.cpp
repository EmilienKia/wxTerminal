/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*-  */
/*
 * wxTerminal
 * Copyright (C) 2013 Émilien KIA <emilien.kia@gmail.com>
 * 
wxTerminal is free software: you can redistribute it and/or modify it
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

#include "terminal-parser.hpp"

#include <iostream>


//
//
// TerminalParser
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

TerminalParser::TerminalParser():
m_state(WXTP_STATE_GROUND)
{
}

void TerminalParser::Process(unsigned char c)
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

void TerminalParser::Transition(WXTP_STATE state)
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


void TerminalParser::executeC0ControlCode(unsigned char c)
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

void TerminalParser::onCAN()
{
	Transition(WXTP_STATE_GROUND);
}

void TerminalParser::onSUB()
{
	Transition(WXTP_STATE_GROUND);
}

void TerminalParser::onESC()
{
	Transition(WXTP_STATE_ESCAPE);
}

void TerminalParser::executeC1ControlCode(unsigned char c)
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

void TerminalParser::onDCS()
{
	Transition(WXTP_STATE_DCS_ENTRY);
}

void TerminalParser::onSOS()
{
	Transition(WXTP_STATE_SOS_PM_APC_STRING);
}

void TerminalParser::onCSI()
{
	Transition(WXTP_STATE_CSI_ENTRY);
}

void TerminalParser::onST()
{
	Transition(WXTP_STATE_GROUND);
}

void TerminalParser::onOSC()
{
	Transition(WXTP_STATE_OSC_STRING);
}

void TerminalParser::onPM()
{
	Transition(WXTP_STATE_SOS_PM_APC_STRING);
}

void TerminalParser::onAPC()
{
	Transition(WXTP_STATE_SOS_PM_APC_STRING);
}

void TerminalParser::Clear()
{
	m_collected.clear();
	m_params.clear();
}

void TerminalParser::Collect(unsigned char c)
{
	m_collected.push_back(c);
}

void TerminalParser::Param(unsigned char c)
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




void TerminalParser::DcsHook()
{
	/* TODO DCS is not implemented yet. */
}

void TerminalParser::DcsUnhook()
{
	/* TODO DCS is not implemented yet. */
}

void TerminalParser::DcsPut(unsigned char c)
{
	/* TODO DCS is not implemented yet. */
}

void TerminalParser::onOSC(unsigned short command, const std::vector<unsigned char>& params)
{
	
}

void TerminalParser::onCSI(unsigned char command, const std::vector<unsigned short>& params, const std::vector<unsigned char>& collect)
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

void TerminalParser::onESC(unsigned char command)
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

void TerminalParser::onESC(unsigned char command, unsigned char param)
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
	    onSCS(0, param);
	    break;
	case ')': // Designate G1 charset
	case '-':
	    onSCS(1, param);
	    break;
	case '*': // Designate G2 charset
	case '.':
	    onSCS(2, param);
	    break;
	case '+': // Designate G3 charset
	case '/':
	    onSCS(3, param);
	    break;
	default:
		// Invalid, TODO should I call onESC(...) with one char ? 
		break;
	}
}

