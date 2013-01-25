/*
 * main.cc
 * Copyright (C) Emilien Kia 2012 <emilien.kia@free.fr>
 *
	wxTerminal is free software: you can redistribute it and/or modify it
	under the terms of the GNU General Public License as published by the
	Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.
	
	wxTerminal is distributed in the hope that it will be useful, but
	WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
	See the GNU General Public License for more details.
	
	You should have received a copy of the GNU General Public License along
	with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif
#include <wx/wx.h>

#include <wx/process.h>

#include "terminal-ctrl.hpp"

class MyApp : public wxApp
{
public:
    virtual bool OnInit();

    void execProcess();

    wxProcess* process;
    wxTerminalCtrl* term;
};

IMPLEMENT_APP(MyApp)

bool MyApp::OnInit()
{
    wxFrame *frame = new wxFrame((wxFrame *)NULL, -1, wxT("wxTerminal"),
                               wxPoint(50, 50), wxSize(450, 340));
    wxSizer* gsz = new wxBoxSizer(wxVERTICAL);
    term = new wxTerminalCtrl(frame, wxID_ANY);
    gsz->Add(term, 1, wxEXPAND);

    frame->SetSizer(gsz);
    frame->Show(TRUE);

    execProcess();
    
    return TRUE;
}

void MyApp::execProcess()
{
    process = new wxProcess;
    process->Redirect();
    
    long res = wxExecute("bash --login -i", wxEXEC_ASYNC/*|wxEXEC_HIDE_CONSOLE*/, process);
    term->SetOutputStream(process->GetOutputStream());
    term->SetInputStream(process->GetInputStream());
    term->SetErrorStream(process->GetErrorStream());
}


