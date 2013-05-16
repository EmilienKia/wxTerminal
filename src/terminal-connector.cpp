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
#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif
#include <wx/wx.h>

#include <wx/event.h>
#include <wx/process.h>

#include "terminal-connector.hpp"

#include "terminal-ctrl.hpp"

//
//
// wxTerminalConnector
//
//

wxTerminalConnector::wxTerminalConnector(wxTerminalCtrl* ctrl)
{
	setTerminalCtrl(ctrl);
}

void wxTerminalConnector::setTerminalCtrl(wxTerminalCtrl* ctrl)
{
	if(_ctrl)
		_ctrl->setConnector(NULL);
	
	_ctrl = ctrl;

	if(ctrl)
		_ctrl->setConnector(this);
}

wxTerminalCtrl* wxTerminalConnector::getTerminalCtrl()
{
	return _ctrl;
}

void wxTerminalConnector::send(const char* msg, ...)
{
	va_list ap;
	va_start(ap, msg);
	vsend(msg, ap);
}

//
//
// wxExecuteTerminalConnector
//
//

wxExecuteTerminalConnector::wxExecuteTerminalConnector(wxTerminalCtrl* ctrl):
wxTerminalConnector(ctrl),
_process(NULL)
{
}

void wxExecuteTerminalConnector::execute(const wxString& cmd)
{
    _process = new wxProcess;
    _process->Redirect();
    
    long res = wxExecute(cmd, wxEXEC_ASYNC/*|wxEXEC_HIDE_CONSOLE*/, _process);

	_outputStream = _process->GetOutputStream();
	_inputStream  = _process->GetInputStream();
	_errorStream  = _process->GetErrorStream();
}

void wxExecuteTerminalConnector::vsend(const char* msg, va_list ap)
{
	if(_outputStream)
	{
		char buffer[1024];
		int sz = vsnprintf(buffer, 1024, msg, ap);
		_outputStream->Write(buffer, sz<0 ? 1024 : sz);
	}
}

void wxExecuteTerminalConnector::send(char c)
{
	if(_outputStream)
	{
		_outputStream->Write(&c, 1);
	}
}

void wxExecuteTerminalConnector::processEntries()
{
	if(_ctrl)
	{
		if(_inputStream)
		{
			if(_inputStream->CanRead())
			{
				unsigned char buffer[256];
				while(true)
				{
					_inputStream->Read(buffer, 256);
					size_t sz = _inputStream->LastRead();
					_ctrl->append(buffer, sz);
					if(sz<256)
						break;
				}
			}
		}
		if(_errorStream)
		{
			if(_errorStream->CanRead())
			{
				unsigned char buffer[256];
				while(true)
				{
					_errorStream->Read(buffer, 256);
					size_t sz = _errorStream->LastRead();
					_ctrl->append(buffer, sz);
					if(sz<256)
						break;
				}
			}
		}
	}
}


//
//
// wxTimerTerminalConnector
//
//

wxBEGIN_EVENT_TABLE(wxTimerTerminalConnector, wxEvtHandler)
	EVT_TIMER(wxID_ANY, wxTimerTerminalConnector::onTimer)
wxEND_EVENT_TABLE()

wxTimerTerminalConnector::wxTimerTerminalConnector(wxTerminalCtrl* ctrl):
wxExecuteTerminalConnector(ctrl)
{
}

void wxTimerTerminalConnector::execute(const wxString& cmd)
{
    wxExecuteTerminalConnector::execute(cmd);

	_timer = new wxTimer(this);
	_timer->Start(10);
}

void wxTimerTerminalConnector::onTimer(wxTimerEvent& event)
{
	processEntries();
}



