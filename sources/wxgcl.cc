//
// FILE: wxgcl.cc -- wxWindows-based front-end for GCL
//
// $Id$
//

#include "wx/wx.h"
#include "wx/sashwin.h"
#include "wx/fontdlg.h"

#include "wxgcl.h"
#include "wxstatus.h"
#include "gsm.h"
#include "gnullstatus.h"
#include "gcmdline.h"
#include "gpreproc.h"
#include "gcompile.h"

char *_SourceDir = 0;
char *_ExePath = 0;

class wxCommandLine : public gclCommandLine {
protected:
  virtual char GetNextChar(void) { return '\n'; }

public:
  wxCommandLine(int p_historyDepth) : gclCommandLine(p_historyDepth) { }
  virtual ~wxCommandLine() { }

  void SetPrompt(bool) { }

  virtual bool eof(void) const { return true; }
  virtual gInput &operator>>(int &) { return *this; }
  virtual gInput &operator>>(unsigned int &) { return *this; }
  virtual gInput &operator>>(long int &) { return *this; }
  virtual gInput &operator>>(char &) { return *this; }
  virtual gInput &operator>>(double &) { return *this; }
  virtual gInput &operator>>(float &) { return *this; }
  virtual gInput &operator>>(char *) { return *this; }
  virtual int get(char &) { return 0; }
  virtual void unget(char) { }
  virtual void seekp(long int) const { }
  virtual long getpos(void) const { return 0L; }
  virtual void setpos(long) const { }
};

class wxOutputWindowStream : public gOutput {
private:
  wxTextCtrl *m_window;

public:
  wxOutputWindowStream(wxTextCtrl *p_window);
  virtual ~wxOutputWindowStream() { }

  virtual gOutput &operator<<(int x);
  virtual gOutput &operator<<(unsigned int x);
  virtual gOutput &operator<<(bool x);
  virtual gOutput &operator<<(long x);
  virtual gOutput &operator<<(char x);
  virtual gOutput &operator<<(double x);
  virtual gOutput &operator<<(float x);
  virtual gOutput &operator<<(const char *x);
  virtual gOutput &operator<<(const void *x);

  virtual int GetWidth(void) const { return 2; }
  virtual gOutput &SetWidth(int w) { return *this; }
  virtual int GetPrec(void) const { return 2; }
  virtual gOutput &SetPrec(int p) { return *this; } 
  virtual gOutput &SetExpMode(void) { return *this; }
  virtual gOutput &SetFloatMode(void) { return *this; }
  virtual char GetRepMode(void) const { return 'f'; }
};

wxOutputWindowStream::wxOutputWindowStream(wxTextCtrl *p_window)
  : m_window(p_window)
{ }

gOutput &wxOutputWindowStream::operator<<(int x)
{
  m_window->AppendText((char *) ToText(x));
  return *this;
}

gOutput &wxOutputWindowStream::operator<<(unsigned int x)
{
  m_window->AppendText((char *) ToText((int) x));
  return *this;
}

gOutput &wxOutputWindowStream::operator<<(bool x)
{
  m_window->AppendText((x) ? "True" : "False");
  return *this;
}

gOutput &wxOutputWindowStream::operator<<(long x)
{
  m_window->AppendText((char *) ToText(x));
  return *this;
}

gOutput &wxOutputWindowStream::operator<<(char x)
{
  m_window->AppendText(x);
  return *this;
}

gOutput &wxOutputWindowStream::operator<<(double x)
{
  m_window->AppendText((char *) ToText(x));
  return *this;
}

gOutput &wxOutputWindowStream::operator<<(float x)
{
  m_window->AppendText((char *) ToText(x));
  return *this;
}

gOutput &wxOutputWindowStream::operator<<(const char *x)
{
  m_window->AppendText(x);
  return *this;
}

gOutput &wxOutputWindowStream::operator<<(const void *x)
{
  char buf[10];
  sprintf(buf, "%p", x);
  m_window->AppendText(buf);
  return *this;
}

const int idCANCEL_BUTTON = 1500;

class wxCancelButton : public wxButton, public gNullStatus {
private:
  bool m_sig;

public:
  wxCancelButton(wxWindow *p_parent);
  virtual ~wxCancelButton() { }
  
  void Get(void) const;
  void Reset(void) { m_sig = false; }
  void Set(void) { m_sig = true; }
};

wxCancelButton::wxCancelButton(wxWindow *p_parent)
  : wxButton(p_parent, idCANCEL_BUTTON, "Cancel")
{ }

void wxCancelButton::Get(void) const
{
  wxYield();
  if (m_sig) {
    throw gSignalBreak();
  }
}

class wxGSM : public GSM {
private:
  gStatus &m_status;
  wxWindow *m_parent;

public:
  wxGSM(wxWindow *p_parent, gStatus &p_status,
	gInput &p_input, gOutput &p_output, gOutput &p_error)
    : GSM(p_input, p_output, p_error), m_status(p_status), m_parent(p_parent)
    { }
  virtual ~wxGSM() { }

  gStatus &GetStatusMonitor(void) { return m_status; }
  gStatus *StartAlgorithmMonitor(const gText &);
  void EndAlgorithmMonitor(gStatus *);
};

gStatus *wxGSM::StartAlgorithmMonitor(const gText &p_caption)
{
  return new wxStatus(m_parent, p_caption);
}

void wxGSM::EndAlgorithmMonitor(gStatus *p_status)
{
  delete p_status;
}


IMPLEMENT_APP(GclApp)

class GclFrame : public wxFrame {
private:
  wxTextCtrl *m_outputWindow;
  wxTextCtrl *m_inputWindow;
  wxSashWindow *m_inputSashWindow;
  wxCancelButton *m_cancelButton;
  wxOutputWindowStream *m_outputStream;
  gList<gText> m_history;

  GSM *m_environment;
  GCLCompiler *m_compiler;

  // Menu event handlers
  void OnSaveLog(wxCommandEvent &);
  void OnSaveScript(wxCommandEvent &);
  void OnPrefsInputFont(wxCommandEvent &);
  void OnPrefsOutputFont(wxCommandEvent &);
  void OnHelpAbout(wxCommandEvent &);
  void OnHelpContents(wxCommandEvent &);

  // Other event handlers
  void OnCloseWindow(wxCloseEvent &);
  void OnSize(wxSizeEvent &);
  void OnSashDrag(wxSashEvent &);
  void OnCancel(wxCommandEvent &);
  void OnTextEnter(wxCommandEvent &);

public:
  GclFrame(wxFrame *p_parent, const wxString &p_title,
	   const wxPoint &p_position, const wxSize &p_size);
  virtual ~GclFrame();

  DECLARE_EVENT_TABLE()
};

bool GclApp::OnInit(void)
{
  (void) new GclFrame(0, "Gambit Command Language",
		      wxPoint(0, 0), wxSize(400, 400));
  return true;
}

const int idINPUT_WINDOW = 1001;
const int idINPUT_SASH_WINDOW = 1002;

const int idSAVE = 2000;
const int idSAVE_LOG = 2001;
const int idSAVE_SCRIPT = 2002;
const int idPREFS_INPUTFONT = 2100;
const int idPREFS_OUTPUTFONT = 2101;

GclFrame::GclFrame(wxFrame *p_parent, const wxString &p_title,
		   const wxPoint &p_position, const wxSize &p_size)
  : wxFrame(p_parent, -1, p_title, p_position, p_size)
{
  wxMenu *fileMenu = new wxMenu;
  wxMenu *fileSaveMenu = new wxMenu;
  fileSaveMenu->Append(idSAVE_LOG, "&Log", "Save all input and output");
  fileSaveMenu->Append(idSAVE_SCRIPT, "&Script", "Save all input only");
  fileMenu->Append(idSAVE, "&Save", fileSaveMenu, "Save input and output");
  fileMenu->Append(wxID_EXIT, "&Quit", "Quit program");
  
  wxMenu *editMenu = new wxMenu;
  editMenu->Append(wxID_CUT, "Cu&t", "Cut selected text");
  editMenu->Append(wxID_COPY, "&Copy", "Copy selected text");
  editMenu->Append(wxID_PASTE, "&Paste", "Paste selected text");

  wxMenu *prefsMenu = new wxMenu;
  prefsMenu->Append(idPREFS_INPUTFONT, "&Input window font",
		    "Change the font used in the input window");
  prefsMenu->Append(idPREFS_OUTPUTFONT, "&Output window font",
		    "Change the font used in the output window");

  wxMenu *helpMenu = new wxMenu;
  helpMenu->Append(wxID_ABOUT, "&About", "About this program");
  helpMenu->Append(wxID_HELP_CONTENTS, "&Contents", "Table of contents");

  wxMenuBar *menuBar = new wxMenuBar;
  menuBar->Append(fileMenu, "&File");
  menuBar->Append(editMenu, "&Edit");
  menuBar->Append(prefsMenu, "&Prefs");
  menuBar->Append(helpMenu, "&Help");
  SetMenuBar(menuBar);

  CreateStatusBar();

  m_outputWindow = new wxTextCtrl(this, -1, "",
				  wxPoint(0, 0), wxSize(400, 300),
				  wxTE_MULTILINE | wxTE_READONLY);
  m_outputStream = new wxOutputWindowStream(m_outputWindow);
  m_inputSashWindow = new wxSashWindow(this, idINPUT_SASH_WINDOW,
				       wxPoint(0, 300), wxSize(400, 100));
#ifdef __WXMSW__
  m_inputWindow = new wxTextCtrl(m_inputSashWindow, idINPUT_WINDOW, "",
				 wxDefaultPosition, wxDefaultSize,
				 wxTE_PROCESS_ENTER);
#else
  m_inputWindow = new wxTextCtrl(m_inputSashWindow, idINPUT_WINDOW, "",
				 wxDefaultPosition, wxDefaultSize,
				 wxTE_MULTILINE | wxTE_PROCESS_ENTER);
#endif  // __WXMSW__
  m_cancelButton = new wxCancelButton(m_inputSashWindow);
  m_cancelButton->Enable(false);

  m_inputSashWindow->SetSashVisible(wxSASH_TOP, true);

  wxBoxSizer *inputSizer = new wxBoxSizer(wxHORIZONTAL);
  inputSizer->Add(m_inputWindow, 1, wxEXPAND | wxRIGHT, 5);
  inputSizer->Add(m_cancelButton, 0, wxCENTER | wxALL, 5);

  m_inputSashWindow->SetAutoLayout(true);
  m_inputSashWindow->SetSizer(inputSizer);
  inputSizer->SetSizeHints(m_inputSashWindow);
  m_inputSashWindow->Layout();

  m_inputWindow->SetFocus();
  m_inputWindow->SetValue("<< ");
  m_inputWindow->SetInsertionPointEnd();

  SetSizeHints(300, 300);
  Show(true);

  _SourceDir = new char[1024];
  strncpy(_SourceDir, wxGetWorkingDirectory(), 1023);
  _ExePath = new char[1024];
  strncpy(_ExePath, wxGetWorkingDirectory(), 1023);

  m_environment = new wxGSM(this, *m_cancelButton,
			    gin, *m_outputStream, *m_outputStream);
  m_compiler = new GCLCompiler(*m_environment);
  wxCommandLine cmdline(20);
  gPreprocessor preproc(*m_environment, &cmdline, "Include[\"gclini.gcl\"]");
  try {
    while (!preproc.eof()) {
      gText line = preproc.GetLine();
      gText fileName = preproc.GetFileName();
      int lineNumber = preproc.GetLineNumber();
      gText rawLine = preproc.GetRawLine();
      m_compiler->Parse(line, fileName, lineNumber, rawLine);
    }
  }
  catch (gSignalBreak &) {
    m_environment->OutputStream() << "Computation interrupted!\n";
  }
  catch (gclQuitOccurred &) {

  }
  catch (gException &w)  {
    m_environment->OutputStream() << "GCL EXCEPTION:" << w.Description()
				  << "; Caught in GclFrame::GclFrame()\n";
  }

  m_inputWindow->SetInsertionPointEnd();
  m_inputWindow->SetFocus();
}

GclFrame::~GclFrame()
{ }

BEGIN_EVENT_TABLE(GclFrame, wxFrame)
  EVT_MENU(idSAVE_LOG, GclFrame::OnSaveLog)
  EVT_MENU(idSAVE_SCRIPT, GclFrame::OnSaveScript)
  EVT_MENU(wxID_EXIT, wxWindow::Close)
  EVT_MENU(idPREFS_INPUTFONT, GclFrame::OnPrefsInputFont)
  EVT_MENU(idPREFS_OUTPUTFONT, GclFrame::OnPrefsOutputFont)
  EVT_MENU(wxID_ABOUT, GclFrame::OnHelpAbout)
  EVT_MENU(wxID_HELP_CONTENTS, GclFrame::OnHelpContents)
  EVT_TEXT_ENTER(idINPUT_WINDOW, GclFrame::OnTextEnter)
  EVT_CLOSE(GclFrame::OnCloseWindow)
  EVT_SIZE(GclFrame::OnSize)
  EVT_SASH_DRAGGED(idINPUT_SASH_WINDOW, GclFrame::OnSashDrag)
  EVT_BUTTON(idCANCEL_BUTTON, GclFrame::OnCancel)
END_EVENT_TABLE()

void GclFrame::OnSaveLog(wxCommandEvent &)
{
  wxFileDialog dialog(this, "Save Log As...", "", "", "*.log");

  if (dialog.ShowModal() == wxID_OK) {
    try {
      gFileOutput file(dialog.GetPath().c_str());
      file << m_outputWindow->GetValue();
    }
    catch (gFileOutput::OpenFailed &) {
      wxMessageBox((char *) (gText("Could not open ") + dialog.GetPath().c_str() +
			     " for writing."), "Error", wxOK);
    }
    catch (gFileOutput::WriteFailed &) {
      wxMessageBox((char *) (gText("Error occurred in writing ") + dialog.GetPath().c_str()),
		   "Error", wxOK);
    }
  }
}

void GclFrame::OnSaveScript(wxCommandEvent &)
{
  wxFileDialog dialog(this, "Save Script As...", "", "", "*.gcl");

  if (dialog.ShowModal() == wxID_OK) {
    try {
      gFileOutput file(dialog.GetPath().c_str());
      for (int i = 1; i <= m_history.Length(); i++) {
	file << ((char *) m_history[i]) << '\n';
      }
    }
    catch (gFileOutput::OpenFailed &) {
      wxMessageBox((char *) (gText("Could not open ") + dialog.GetPath().c_str() +
			     " for writing."), "Error", wxOK);
    }
    catch (gFileOutput::WriteFailed &) {
      wxMessageBox((char *) (gText("Error occurred in writing ") + dialog.GetPath().c_str()),
		   "Error", wxOK);
    }
  }
}

void GclFrame::OnPrefsInputFont(wxCommandEvent &) 
{
  wxFontData data;
  wxFontDialog dialog(this, &data);
  
  if (dialog.ShowModal() == wxID_OK) {
    m_inputWindow->SetFont(dialog.GetFontData().GetChosenFont());
  }
}

void GclFrame::OnPrefsOutputFont(wxCommandEvent &) 
{
  wxFontData data;
  wxFontDialog dialog(this, &data);
  
  if (dialog.ShowModal() == wxID_OK) {
    m_outputWindow->SetFont(dialog.GetFontData().GetChosenFont());
  }
}


void GclFrame::OnHelpAbout(wxCommandEvent &)
{
  wxMessageBox("Gambit Command Language, Version 0.96.3\n"
	       "Built with " wxVERSION_STRING "\n\n"
	       "Part of the Gambit Project\n"
	       "www.hss.caltech.edu/~gambit/Gambit.html\n"
	       "gambit@hss.caltech.edu\n\n"
	       "Copyright (C) 1999-2000\n"
	       "California Institute of Technology\n"
	       "Funding provided by the National Science Foundation",
	       "About...");
}

void GclFrame::OnHelpContents(wxCommandEvent &)
{ }

void GclFrame::OnCloseWindow(wxCloseEvent &)
{
  Destroy();
}

void GclFrame::OnSize(wxSizeEvent &)
{
  int clientWidth, clientHeight;
  GetClientSize(&clientWidth, &clientHeight);

  m_outputWindow->SetSize(0, 0, clientWidth,
			  gmax(clientHeight - m_inputSashWindow->GetRect().height, 50));
  m_inputSashWindow->SetSize(0, m_outputWindow->GetRect().height,
			     clientWidth,
			     clientHeight - m_outputWindow->GetRect().height);
  m_inputSashWindow->Layout();
}

void GclFrame::OnSashDrag(wxSashEvent &p_event)
{
  int clientWidth, clientHeight;
  GetClientSize(&clientWidth, &clientHeight);

  int dragHeight = gmax(gmin(clientHeight - 50, p_event.GetDragRect().height),
			50);

  m_outputWindow->SetSize(0, 0, clientWidth,
			  clientHeight - dragHeight);
  m_inputSashWindow->SetSize(0, clientHeight - dragHeight,
			     clientWidth, dragHeight);
  m_inputSashWindow->Layout();
}

void GclFrame::OnTextEnter(wxCommandEvent &)
{
  m_outputWindow->AppendText(m_inputWindow->GetValue());
  m_history.Append(m_inputWindow->GetValue().c_str());
  m_outputWindow->AppendText("\n");

  m_inputWindow->Enable(false);
  m_cancelButton->Enable(true);

  wxCommandLine cmdline(20);
  gPreprocessor preproc(*m_environment, &cmdline,
			gText(m_inputWindow->GetValue().c_str()) + "\n");
  m_inputWindow->SetValue("");
  try {
    while (!preproc.eof()) {
      gText line = preproc.GetLine();
      gText fileName = preproc.GetFileName();
      int lineNumber = preproc.GetLineNumber();
      gText rawLine = preproc.GetRawLine();
      m_compiler->Parse(line, fileName, lineNumber, rawLine);
    }
  }
  catch (gSignalBreak &) {
    m_environment->OutputStream() << "Computation interrupted!\n";
  }
  catch (gclQuitOccurred &) {

  }
  catch (gException &w)  {
    m_environment->OutputStream() << "GCL EXCEPTION:" << w.Description() 
				  << "; Caught in GclFrame::OnTextEnter()\n";
  }

  m_cancelButton->Enable(false);
  m_inputWindow->SetValue("<< ");
  m_inputWindow->SetInsertionPointEnd();
  m_inputWindow->Enable(true);
  m_outputWindow->AppendText("\n");
  m_inputWindow->SetFocus();
}

void GclFrame::OnCancel(wxCommandEvent &)
{
  m_cancelButton->Set();
}

