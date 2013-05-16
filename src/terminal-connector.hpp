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

#ifndef _TERMINAL_CONNECTOR_HPP_
#define _TERMINAL_CONNECTOR_HPP_

class wxTerminalCtrl;

/**
 * Base class for terminal connector.
 */
class wxTerminalConnector
{
	friend class wxTerminalCtrl;
public:

	/** Attach a terminal to the terminal connector. */
	void setTerminalCtrl(wxTerminalCtrl* ctrl);

	/** Retrieve the attached terminal. */
	wxTerminalCtrl* getTerminalCtrl();
	
protected:
	wxTerminalConnector(wxTerminalCtrl* ctrl = NULL);

	/** Send some chars to shell. Same format as printf. */
	void send(const char* msg, ...);

	/** Send some chars to shell. Same format as printf. */
	virtual void vsend(const char* msg, va_list ap){}

	/** Send a char to shell. */
	virtual void send(char c){}

	/** Terminal attached to the connector. */
	wxTerminalCtrl* _ctrl;
	 
};


/**
 * Base pure wxWidgets terminal connector.
 * 
 * Abstract terminal connector using wxExecute method to launch shell process.
 */
class wxExecuteTerminalConnector : public wxTerminalConnector
{
public:

	virtual void execute(const wxString& cmd = "bash --login -i");
	
protected:
	wxExecuteTerminalConnector(wxTerminalCtrl* ctrl = NULL);

	/*overriden*/ void vsend(const char* msg, va_list ap);
	/*overriden*/ void send(char c);
	
	void processEntries();

	wxProcess*      _process;
	wxOutputStream* _outputStream;
	wxInputStream*  _inputStream;
	wxInputStream*  _errorStream;	
};


/**
 * Pure wxWidgets terminal connector using timer to receive data from shell.
 */
class wxTimerTerminalConnector : public wxExecuteTerminalConnector, public wxEvtHandler
{
	wxDECLARE_EVENT_TABLE();
public:
	wxTimerTerminalConnector(wxTerminalCtrl* ctrl = NULL);

	/*overriden*/ void execute(const wxString& cmd = "bash --login -i");
	
protected:

	void onTimer(wxTimerEvent& event);
	
	wxTimer* _timer;    // Timer for i/o treatments.
};



#endif // _TERMINAL_CONNECTOR_HPP_

