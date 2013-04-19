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

#ifndef _TERMINAL_PARSER_HPP_
#define _TERMINAL_PARSER_HPP_


#include <vector>
#include <list>
#include <set>


class TerminalParser
{
public:

	void Process(unsigned char c);
	
protected:
	TerminalParser();


	/**
	 * Receive a printable char, to print.
	 */
	virtual void onPrintableChar(unsigned char c){}

	virtual void onSP(){}
	virtual void onDEL(){}	

	/** Escape sequence control handlers  (other than C0 and C1)
	 * EKI Should I support HP extensions ?
	 * \{ */
	/**
	 * Receive an one-char ESC (escaped) command.
	 * \param command Command character (first char after ESC command).
	 */
	virtual void onESC(unsigned char command);
	/**
	 * Receive an two-char ESC (escaped) command.
	 * \param command Command character (first char after ESC command).
	 * \param param Parameter character (second char after ESC command).
	 */
	virtual void onESC(unsigned char command, unsigned char param);	
	virtual void onS7C1T(){} // 7-bit controls
	virtual void onS8C1T(){} // 8-bit controls
	virtual void onANSIconf1(){} // Set ANSI conformance level 1  (vt100, 7-bit controls).
	virtual void onANSIconf2(){} // Set ANSI conformance level 2  (vt200).
	virtual void onANSIconf3(){} // Set ANSI conformance level 3  (vt300).
	virtual void onDECDHLth(){} // DEC double-height line, top half
	virtual void onDECDHLbh(){} // DEC double-height line, bottom half
	virtual void onDECSWL(){} // DEC single-width line
	virtual void onDECDWL(){} // DEC double-width line
	virtual void onDECALN(){} // DEC Screen Alignment Test
	virtual void onISO8859_1(){} // Select default character set. That is ISO 8859-1 (ISO 2022).
	virtual void onUTF_8(){} // Select UTF-8 character set (ISO 2022).
	virtual void onSCS(unsigned char id, unsigned char charset){} // Character Set Selection (SCS). Designate G(id) (G0...G3) Character Set (ISO 2022) 
	virtual void onDECBI(){} // Back Index, VT420 and up.
	virtual void onDECSC(){} // Save cursor
	virtual void onDECRC(){} // Restore cursor
	virtual void onDECFI(){} // Forward Index, VT420 and up.
	virtual void onDECKPAM(){} // Application Keypad
	virtual void onDECKPNM(){} // Normal Keypad
	virtual void onRIS(){} // Full Reset
	virtual void onLS2(){} // Invoke the G2 Character Set as GL.
	virtual void onLS3(){} // Invoke the G3 Character Set as GL.
	virtual void onLS1R(){} // Invoke the G1 Character Set as GR (). Has no visible effect in xterm.
	virtual void onLS2R(){} // Invoke the G1 Character Set as GR (). Has no visible effect in xterm.
	virtual void onLS3R(){} // Invoke the G1 Character Set as GR (). Has no visible effect in xterm.
	/** \} */

	
	/** C0 Control code handlers
	 * \{ */
	/**
	 * Execute C0 control code (0x00-0x1F)
	 * http://en.wikipedia.org/wiki/C0_and_C1_control_codes
	 */
	virtual void executeC0ControlCode(unsigned char c);
	virtual void onNUL(){}  // 0x00
	virtual void onSOH(){}  // 0x01
	virtual void onSTX(){}  // 0x02
	virtual void onETX(){}  // 0x03
	virtual void onEOT(){}  // 0x04
	virtual void onENQ(){}  // 0x05
	virtual void onACK(){}  // 0x06
	virtual void onBEL(){}  // 0x07
	virtual void onBS(){}   // 0x08
	virtual void onHT(){}   // 0x09
	virtual void onLF(){}   // 0x0A
	virtual void onVT(){}   // 0x0B
	virtual void onFF(){}   // 0x0C
	virtual void onCR(){}   // 0x0D
	virtual void onSO(){}   // 0x0E
	virtual void onSI(){}   // 0x0F
	virtual void onDLE(){}  // 0x10
	virtual void onDC1(){}  // 0x11
	virtual void onDC2(){}  // 0x12
	virtual void onDC3(){}  // 0x13
	virtual void onDC4(){}  // 0x14
	virtual void onNAK(){}  // 0x15
	virtual void onSYN(){}  // 0x16
	virtual void onETB(){}  // 0x17
	virtual void onCAN();   // 0x18
	virtual void onEM(){}   // 0x19
	virtual void onSUB();   // 0x1A
	virtual void onESC();   // 0x1B
	virtual void onFS(){}   // 0x1C
	virtual void onGS(){}   // 0x1D
	virtual void onRS(){}   // 0x1E
	virtual void onUS(){}   // 0x1F
	/** \} */

	
	/** C1 Control code handlers
	 * \{ */
	/**
	 * Execute C1 control code (0x80-0x9F)
	 * http://en.wikipedia.org/wiki/C0_and_C1_control_codes
	 */
	virtual void executeC1ControlCode(unsigned char c);
	virtual void onPAD(){}  // 0x80
	virtual void onHOP(){}  // 0x81
	virtual void onBPH(){}  // 0x82
	virtual void onNBH(){}  // 0x83
	virtual void onIND(){}  // 0x84
	virtual void onNEL(){}  // 0x85
	virtual void onSSA(){}  // 0x86
	virtual void onESA(){}  // 0x87
	virtual void onHTS(){}  // 0x88
	virtual void onHTJ(){}  // 0x89
	virtual void onVTS(){}  // 0x8A
	virtual void onPLD(){}  // 0x8B
	virtual void onPLU(){}  // 0x8C
	virtual void onRI(){}   // 0x8D
	virtual void onSS2(){}  // 0x8E
	virtual void onSS3(){}  // 0x8F
	virtual void onDCS();   // 0x90
	virtual void onPU1(){}  // 0x91
	virtual void onPU2(){}  // 0x92
	virtual void onSTS(){}  // 0x93
	virtual void onCCH(){}  // 0x94
	virtual void onMW(){}   // 0x95
	virtual void onSPA(){}  // 0x96
	virtual void onEPA(){}  // 0x97
	virtual void onSOS();   // 0x98
	virtual void onSGCI(){} // 0x99
	virtual void onSCI(){}  // 0x9A
	virtual void onCSI();   // 0x9B
	virtual void onST();    // 0x9C
	virtual void onOSC();   // 0x9D
	virtual void onPM();    // 0x9E
	virtual void onAPC();   // 0x9F
	/** \} */
	

	/** CSI Control handlers
	 * \{ */
	/**
	 * Receive a CSI (Control Sequence Introducer) command.
	 * \param command Command character (last character of CSI sequence).
	 * \param params  Optionnal parameter sequence.
     */
	virtual void onCSI(unsigned char command, const std::vector<unsigned short>& params, const std::vector<unsigned char>& collect);
	virtual void onICH(unsigned short nb=1){} // Insert P s (Blank) Character(s) (default = 1)
	virtual void onCUU(unsigned short nb=1){} // Cursor Up P s Times (default = 1)
	virtual void onCUD(unsigned short nb=1){} // Cursor Down P s Times (default = 1)
	virtual void onCUF(unsigned short nb=1){} // Cursor Forward P s Times (default = 1)
	virtual void onCUB(unsigned short nb=1){} // Cursor Backward P s Times (default = 1)
	virtual void onCNL(unsigned short nb=1){} // Cursor Next Line P s Times (default = 1)
	virtual void onCPL(unsigned short nb=1){} // Cursor Preceding Line P s Times (default = 1)
	virtual void onCHA(unsigned short nb=1){} // Cursor Character Absolute [column] (default = [row,1]) // Moves the cursor to column n.
	virtual void onCUP(unsigned short row=1, unsigned short col=1){} // Cursor Position [row;column] (default = [1,1])
	virtual void onCHT(unsigned short nb=1){} // Cursor Forward Tabulation P s tab stops (default = 1)
	virtual void onED(unsigned short opt=1){}  // Erase in Display. 0 → Erase Below (default). 1 → Erase Above. 2 → Erase All. 3 → Erase Saved Lines (xterm).
	virtual void onDECSED(unsigned short opt=1){}  // Erase in Display. 0 → Selective Erase Below (default). 1 → Selective Erase Above. 2 → Selective Erase All. 3 → Selective Erase Saved Lines (xterm).
	virtual void onEL(unsigned short opt=0){}      // Erase in Line. 0 → Erase to Right (default). 1 → Erase to Left. 2 → Erase All.
	virtual void onDECSEL(unsigned short nb=0){}  // Erase in Line. 0 → Selective Erase to Right (default). 1 → Selective Erase to Left. 2 → Selective Erase All.
	virtual void onIL(unsigned short nb=1){}  // Insert Ps Line(s) (default = 1)
	virtual void onDL(unsigned short nb=1){}  // Delete Ps Line(s) (default = 1)
	virtual void onDCH(unsigned short nb=1){} // Delete Ps Character(s) (default = 1)
	virtual void onSU(unsigned short nb=1){}  // Scroll up Ps lines (default = 1)
	virtual void onSD(unsigned short nb=1){}  // Scroll down Ps lines (default = 1)
	virtual void onECH(unsigned short nb=1){}  // Erase Ps Character(s) (default = 1)
	virtual void onCBT(unsigned short nb=1){}  // Cursor Backward Tabulation Ps tab stops (default = 1)
	virtual void onHPA(const std::vector<unsigned short> nbs){}  // Character Position Absolute [column] (default = [row,1]) (HPA).
	virtual void onHPR(const std::vector<unsigned short> nbs){}  // Character Position Relative [columns] (default = [row,col+1]) (HPR).
	virtual void onVPA(const std::vector<unsigned short> nbs){}  // Line Position Absolute [row] (default = [1,column])
	virtual void onVPR(const std::vector<unsigned short> nbs){}  // Line Position Relative [rows] (default = [row+1,column]) (VPR)
	virtual void onHVP(unsigned short row=1, unsigned short col=1){} // Horizontal and Vertical Position [row;column] (default = [1,1])
	virtual void onTBC(unsigned short nb=0){}  // Tab Clear
	virtual void onSM(const std::vector<unsigned short> nbs){}  // Set Mode
	virtual void onDECSET(const std::vector<unsigned short> nbs){}  // DEC Private Mode Set
	virtual void onMC(const std::vector<unsigned short> nbs){}  // Media Copy
	virtual void onDECMC(const std::vector<unsigned short> nbs){}  // DEC specific Media Copy
	virtual void onRM(const std::vector<unsigned short> nbs){}  // Reset Mode
	virtual void onDECRST(const std::vector<unsigned short> nbs){}  // DEC Private Mode Reset
	virtual void onSGR(const std::vector<unsigned short> nbs){}  // Character Attributes
	virtual void onDSR(unsigned short nb){}  // Device Status Report
	virtual void onDECDSR(unsigned short nb){}  // DEC-specific Device Status Report
	virtual void onDECSTR(){}  // Soft terminal reset
	virtual void onDECSCL(unsigned short nb1, unsigned short nb2){} // Set conformance level
	virtual void onDECRQM(unsigned short nb){}  // Request DEC private mode
	virtual void onDECLL(unsigned short nb=0){}  // Load LEDs
	virtual void onDECSCUSR(unsigned short nb){}  // Set cursor style (DECSCUSR, VT520).
	virtual void onDECSCA(unsigned short nb=0){}  // Select character protection attribute (DECSCA).
	virtual void onDECSTBM(unsigned short top=0, unsigned short bottom=0){} // Set Scrolling Region [top;bottom] (default = full size of window)
 	virtual void onRDECPMV(const std::vector<unsigned short> nbs){}  // Restore DEC Private Mode Values. The value of P s previously saved is restored. P s values are the same as for DECSET.
	virtual void onSDECPMV(const std::vector<unsigned short> nbs){}  // Save DEC Private Mode Values. P s values are the same as for DECSET.
	virtual void onDECCARA(const std::vector<unsigned short> nbs){}  // Change Attributes in Rectangular Area (DECCARA), VT400 and up.
	virtual void onDECSLRM(unsigned short left, unsigned short right){} // Set left and right margins (DECSLRM), available only when DECLRMM is enabled (VT420 and up).
	virtual void onANSISC(){}  // Save cursor (ANSI.SYS), available only when DECLRMM is disabled.
	virtual void onANSIRC(){}  // Restore cursor (ANSI.SYS).
	virtual void onWindowManip(unsigned short nb1, unsigned short nb2=0, unsigned short nb3=0){} // Set conformance level
	virtual void onDECRARA(const std::vector<unsigned short> nbs){}  // Reverse Attributes in Rectangular Area (DECRARA), VT400 and up.
	virtual void onDECSWBV(unsigned short nb){}  // Set warning-bell volume (DECSWBV, VT520).
	virtual void onDECSMBV(unsigned short nb){}  // Set margin-bell volume (DECSMBV, VT520).
	virtual void onDECCRA(const std::vector<unsigned short> nbs){}  // Copy Rectangular Area (DECCRA, VT400 and up).
	virtual void onDECEFR(unsigned short top, unsigned short left, unsigned short bottom, unsigned short right){} // Enable Filter Rectangle (DECEFR), VT420 and up.
	virtual void onDECREQTPARM(unsigned short nb){}  // Request Terminal Parameters
	virtual void onDECSACE(unsigned short nb){}  // Select Attribute Change Extent
	virtual void onDECFRA(unsigned short chr, unsigned short top, unsigned short left, unsigned short bottom, unsigned short right){} // Fill Rectangular Area (DECFRA), VT420 and up.
	virtual void onDECRQCRA(unsigned short id, unsigned short page, unsigned short top, unsigned short left, unsigned short bottom, unsigned short right){} // Request Checksum of Rectangular Area (DECRQCRA), VT420 and up.
	virtual void onDECELR(unsigned short nb1, unsigned short nb2){} // Enable Locator Reporting (DECELR).
	virtual void onDECERA(unsigned short top, unsigned short left, unsigned short bottom, unsigned short right){} // Erase Rectangular Area (DECERA), VT400 and up.
	virtual void onDECSLE(const std::vector<unsigned short> nbs){}  // Select Locator Events (DECSLE).
	virtual void onDECSERA(unsigned short top, unsigned short left, unsigned short bottom, unsigned short right){} // Selective Erase Rectangular Area (), VT400 and up.
 	virtual void onDECRQLP(unsigned short nb){}  // Request Locator Position (DECRQLP).
	virtual void onDECIC(unsigned short nb=1){}  // Insert P s Column(s) (default = 1) (DECIC), VT420 and up.
	virtual void onDECDC(unsigned short nb=1){}  // InsDelete P s Column(s) (default = 1) (DECIC), VT420 and up.
	/** \} */

	
	/** OSC Control handlers
	 * \{ */
	/**
	 * Receive an OSC (Operating System Command) command.
	 * \param command Command (First decoded integer).
	 * \param params Optionnal parameter sequence.
     */
	virtual void onOSC(unsigned short command, const std::vector<unsigned char>& params);
	/** \} */

private:
	enum WXTP_STATE
	{
		WXTP_STATE_GROUND,
		WXTP_STATE_ESCAPE,
		WXTP_STATE_ESCAPE_INTERMEDIATE,
		WXTP_STATE_CSI_ENTRY,
		WXTP_STATE_CSI_PARAM,
		WXTP_STATE_CSI_INTERMEDIATE,
		WXTP_STATE_CSI_IGNORE,
		WXTP_STATE_DCS_ENTRY,
		WXTP_STATE_DCS_PARAM,
		WXTP_STATE_DCS_INTERMEDIATE,
		WXTP_STATE_DCS_PASSTHROUGH,
		WXTP_STATE_DCS_IGNORE,
		WXTP_STATE_OSC_ENTRY,
		WXTP_STATE_OSC_STRING,
		WXTP_STATE_SOS_PM_APC_STRING
	}m_state;

	/**
	 * Change to a new state
	 */
	void Transition(WXTP_STATE state);

	/**
	 * Clear
	 */
	void Clear();

	/**
	 * Collect the current character.
	 */
	void Collect(unsigned char c);

	/**
	 * PARAM
	 */
	void Param(unsigned char c);
	

	/**
	 * DCS relative functions.
	 * \{
	 */
	
	/** Hook */
	void DcsHook();
	
	/** Unhook */
	void DcsUnhook();

	/** PUT */
	void DcsPut(unsigned char c);

	/** \} *
		
	/** Vector for storing collected data. \see Collect(unsigned char) */
	std::vector<unsigned char> m_collected;

	/** Vector for storing collected parameters. \see Param(unsigned char) */
	std::vector<unsigned short> m_params;

	/** First escaping character, if any. */
	unsigned char m_escapedFirstChar;
};


#endif // _TERMINAL_PARSER_HPP_

