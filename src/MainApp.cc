
#include "MainApp.hh"

#include <fstream>
#include <iostream>
#include <signal.h>
#include <string>

#include <wx/wxprec.h>
#ifndef WX_PRECOMP
    #include <wx/wx.h>
#endif
#include <wx/app.h>
#include <wx/menu.h>
#include <wx/process.h>
#include <wx/webview.h>

#include "config.hh"
#include "util.hh"
#include "MainFrame.hh"

wxIMPLEMENT_APP(MainApp);

static void signal_handler(int /* signum */)
{
    exit(wxGetApp().OnExit());
}

bool MainApp::OnInit()
{
    waiting_to_quit = false;

    // Load blank notebook so we can do "New"
    blank_notebook = read_all_file(config::blank_notebook_filename);

    // Initialize image handlers so we can load toolbar bitmaps
    wxInitAllImageHandlers();

    // On Mac, allow no frames
#if defined(__APPLE__)
    SetExitOnFrameDelete(false);
    wxMenuBar *menubar = new wxMenuBar();
    wxMenu *menu = new wxMenu();
    menu->Append(wxID_ABOUT, "About");
    menu->Append(wxID_NEW, "New\tCtrl-N");
    menu->Append(wxID_OPEN, "Open\tCtrl-O");
    menu->AppendSeparator();
    menu->Append(wxID_EXIT, "Quit\tCtrl-Q");
    menubar->Append(menu, "File");

    wxMenuBar::MacSetCommonMenuBar(menubar);

    Bind(wxEVT_COMMAND_MENU_SELECTED, &MainFrame::OnNew, wxID_NEW);
    Bind(wxEVT_COMMAND_MENU_SELECTED, &MainFrame::OnOpen, wxID_OPEN);
    Bind(wxEVT_COMMAND_MENU_SELECTED, &MainApp::OnAbout, wxID_ABOUT);
    Bind(wxEVT_COMMAND_MENU_SELECTED, &MainApp::OnQuit, this, wxID_EXIT);

#endif

    // See if we can load most recently used file
    std::string filename(recently_used.Get());

    if (filename.empty()) {
        // If no recently used, create fresh new file
        MainFrame::CreateNew(true);
    } else {
        // Open most recently used
        MainFrame::Spawn(url_from_filename(filename), filename, true);
    }

    wxString server_script;
    server = nullptr;
    if (!wxGetEnv(config::server_script_env, &server_script)) {
        server_script = config::server_script_default;
    }

    Bind(wxEVT_END_PROCESS, &MainApp::OnSubprocessTerminate, this, wxID_ANY);

    server = new wxProcess(frame);
    wxExecute(server_script,
        wxEXEC_ASYNC | wxEXEC_HIDE_CONSOLE | wxEXEC_MAKE_GROUP_LEADER,
        server);
    // Set handler to kill process if we die
    signal(SIGINT, signal_handler);

    return true;
}

int MainApp::OnExit()
{
    if (server) {
        server->Kill(static_cast<int>(server->GetPid()), wxSIGTERM, wxKILL_CHILDREN);
    }
    return(0);
}

void MainApp::OnSubprocessTerminate(wxProcessEvent &/* event */)
{
    std::cout << "SUBPROCESS TERMINATED" << std::endl;
}

void MainApp::OnAbout(wxCommandEvent &/* event */)
{
    std::stringstream ss;
    ss << config::version_full << "\n\n";
    ss << "Copyright (c) 2015 Nathan Whitehead\n\n";
    ss << wxGetLibraryVersionInfo().ToString() << "\n";
    ss << "Icons are from: https://icons8.com/" << std::endl; 
    wxMessageBox(ss.str(), "About", wxOK | wxICON_INFORMATION);
}

void MainApp::OnQuit(wxCommandEvent &/* event */)
{
    // Close all frames asynchronously
    // As they close, they remove themselves from list
    // and notice waiting_to_quit
    waiting_to_quit = true;
    if (frames.size() > 0) {
        for (auto it : frames) {
            if (it) {
                it->Close();
            }
        }
    } else {
        ExitMainLoop();
    }
}