/**
 * GeoDa TM, Copyright (C) 2011-2015 by Luc Anselin - all rights reserved
 *
 * This file is part of GeoDa.
 *
 * GeoDa is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * GeoDa is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include <string>
#include <wx/filedlg.h>
#include <wx/dir.h>
#include <wx/filefn.h>
#include <wx/msgdlg.h>
#include <wx/checkbox.h>
#include <wx/gauge.h>
#include <wx/stattext.h>
#include <wx/xrc/xmlres.h>
#include <wx/hyperlink.h>
#include <wx/tokenzr.h>
#include <wx/stdpaths.h>
#include <wx/progdlg.h>
#include <wx/panel.h>
#include <wx/textfile.h>
#include <wx/regex.h>
#include <wx/uri.h>
#include <wx/combobox.h>
#include <json_spirit/json_spirit.h>
#include <json_spirit/json_spirit_writer.h>
#include "curl/curl.h"

#include "../ShapeOperations/OGRDataAdapter.h"
#include "../GeneralWxUtils.h"
#include "../GenUtils.h"
#include "../GdaConst.h"
#include "../GdaJson.h"
#include "../logger.h"
#include "ReportBugDlg.h"

using namespace std;
using namespace GdaJson;

ReportResultDlg::ReportResultDlg( wxWindow* parent, wxString issue_url,
                                 wxWindowID id,
                                 const wxString& title,
                                 const wxPoint& pos,
                                 const wxSize& size)
: wxDialog(parent, id, title, pos, size)
{
    wxPanel* panel = new wxPanel(this);
    panel->SetBackgroundColour(*wxWHITE);

    wxBoxSizer* bSizer = new wxBoxSizer( wxVERTICAL );
    
    wxString result_tip = _("Thanks for reporting bug! You can click the following link to check and trace the reported bug. \n\nGeoDa team thanks you to upload your data or screenshots for troubleshooting using this link or send to spatial@uchicago.edu privately.");
    
    wxStaticText* lbl_tip = new wxStaticText(panel, wxID_ANY, result_tip);
    m_hyperlink1 = new wxHyperlinkCtrl(panel, wxID_ANY, issue_url,
                                       issue_url);
    bSizer->Add(lbl_tip,  1, wxALIGN_TOP | wxEXPAND | wxLEFT | wxRIGHT | wxTOP, 10);
    bSizer->Add(m_hyperlink1,  1, wxALIGN_TOP | wxEXPAND | wxLEFT | wxRIGHT | wxTOP, 0);
    
    panel->SetSizerAndFit(bSizer);
    
    wxBoxSizer* sizerAll = new wxBoxSizer(wxVERTICAL);
    sizerAll->Add(panel, 1, wxEXPAND|wxALL);
    SetSizer(sizerAll);
    SetAutoLayout(true);
    Centre( wxBOTH );
}

ReportResultDlg::~ReportResultDlg()
{
}


size_t write_to_string_(void *ptr, size_t size, size_t count, void *stream) {
    ((string*)stream)->append((char*)ptr, 0, size*count);
    return size*count;
}

string CreateIssueOnGithub(string& post_data)
{
    std::vector<std::string> tester_ids = OGRDataAdapter::GetInstance().GetHistory("tester_id");
    if (tester_ids.empty()) {
        return "";
    }
    
    wxString tester_id = tester_ids[0];
    
    string url = "https://api.github.com/repos/lixun910/colamap/issues";
    
    wxString header_auth = "Authorization: token " + tester_id;
   
    wxString header_user_agent = "User-Agent: GeoDaTester";
    
    string response;
    
    CURL* curl = curl_easy_init();
    CURLcode res;
    if (curl) {
        struct curl_slist *chunk = NULL;
        
        chunk = curl_slist_append(chunk, header_auth.c_str());
        chunk = curl_slist_append(chunk, header_user_agent.c_str());
       
        // set our custom set of headers
        res = curl_easy_setopt(curl, CURLOPT_HTTPHEADER, chunk);
 
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, post_data.c_str());
        
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_to_string_);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
        curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 1L);
        
        res = curl_easy_perform(curl);
        curl_easy_cleanup(curl);
        
        /* free the custom headers */
        curl_slist_free_all(chunk);
        
    }
    return response;
}



ReportBugDlg::ReportBugDlg(wxWindow* parent, wxWindowID id,
                           const wxString& title,
                           const wxPoint& pos,
                           const wxSize& size)
: wxDialog(parent, id, title, pos, size)
{
    //
    // Description: please briefly describe what went wrong
    // Steps you took before something went wrong (Optional):
    // Data you used (Optional): __________________________
    //
    // Create controls UI
    wxPanel* panel = new wxPanel(this);
    panel->SetBackgroundColour(*wxWHITE);
   
    desc_tip = _("[Please briefly describe what went wrong]");
    steps_txt = _("[Steps you took before something went wrong]");
    
    title_txt_ctrl = new wxTextCtrl(panel, wxID_ANY, desc_tip);
    steps_txt_ctrl = new wxTextCtrl(panel, wxID_ANY, steps_txt,
                                                wxDefaultPosition,
                                                wxSize(500,200),
                                                wxTE_MULTILINE);
    
    //wxString data_txt = _("Share part of your data for troubleshooting? (first 10 records)");
    //wxCheckBox* data_chk = new wxCheckBox(panel, wxID_ANY, data_txt);
  
    wxString user_tip = _("Your Github account (Optional):");
    wxStaticText* lbl_user = new wxStaticText(panel, wxID_ANY, user_tip);
    user_txt_ctrl = new wxTextCtrl(panel, wxID_ANY, "",
                                   wxDefaultPosition, wxSize(150,-1));
    wxBoxSizer* user_box = new wxBoxSizer(wxHORIZONTAL);
    user_box->Add(lbl_user);
    user_box->Add(user_txt_ctrl);
    
    wxString email_tip = _("Your Email address (Optional):");
    wxStaticText* lbl_email = new wxStaticText(panel, wxID_ANY, email_tip);
    email_txt_ctrl = new wxTextCtrl(panel, wxID_ANY, "",
                                    wxDefaultPosition, wxSize(150,-1));
    wxBoxSizer* email_box = new wxBoxSizer(wxHORIZONTAL);
    email_box->Add(lbl_email);
    email_box->AddSpacer(10);
    email_box->Add(email_txt_ctrl);
    
    // buttons
    wxButton* btn_cancel= new wxButton(panel, wxID_CANCEL, _("Cancel"),
                                       wxDefaultPosition,
                                       wxDefaultSize, wxBU_EXACTFIT);
    wxButton* btn_submit = new wxButton(panel, wxID_ANY, _("Submit Bug Report"),
                                       wxDefaultPosition,
                                       wxDefaultSize, wxBU_EXACTFIT);
    
    wxBoxSizer* btn_box = new wxBoxSizer(wxHORIZONTAL);
    btn_box->Add(btn_cancel, 1, wxALIGN_CENTER |wxEXPAND| wxALL, 10);
    btn_box->Add(btn_submit, 1, wxALIGN_CENTER | wxEXPAND | wxALL, 10);
    
    wxBoxSizer* box = new wxBoxSizer(wxVERTICAL);
    box->Add(title_txt_ctrl, 0, wxALIGN_TOP | wxEXPAND | wxLEFT | wxRIGHT | wxTOP, 10);
    box->Add(steps_txt_ctrl, 0, wxALIGN_TOP|wxEXPAND|wxLEFT|wxRIGHT | wxTOP, 10);
    //box->Add(data_chk, 0, wxALIGN_TOP | wxEXPAND | wxLEFT | wxRIGHT | wxTOP, 20);
    box->Add(user_box, 0, wxALIGN_TOP | wxEXPAND | wxLEFT | wxRIGHT | wxTOP, 10);
    box->Add(email_box, 0, wxALIGN_TOP | wxEXPAND | wxLEFT | wxRIGHT | wxTOP, 10);
    box->Add(btn_box, 0, wxALIGN_TOP | wxEXPAND | wxLEFT | wxRIGHT | wxTOP, 20);
    
    panel->SetSizerAndFit(box);
    
    wxBoxSizer* sizerAll = new wxBoxSizer(wxVERTICAL);
    sizerAll->Add(panel, 1, wxEXPAND|wxALL);
    SetSizer(sizerAll);
    SetAutoLayout(true);
    
    btn_submit->Bind(wxEVT_BUTTON, &ReportBugDlg::OnOkClick, this);
    
                     
    SetParent(parent);
    SetPosition(pos);
    Centre();
   
    
}

ReportBugDlg::~ReportBugDlg()
{
    
}

void ReportBugDlg::OnOkClick(wxCommandEvent& event)
{
    //wxString rst = CreateIssueOnGithub("{\"title\": \"Test reporting bug from GeoDa software\", \"body\": \"We should have one\"}");

    wxString title =  title_txt_ctrl->GetValue();
    wxString body = steps_txt_ctrl->GetValue();
    wxString user_github = user_txt_ctrl->GetValue();
    wxString email = email_txt_ctrl->GetValue();
    
    if (title.IsEmpty() || title == desc_tip) {
        wxMessageDialog msgDlg(this,
                               _("Please briefly describe what went wrong."),
                               _("Input is required"),
                               wxOK |wxICON_INFORMATION);
        msgDlg.ShowModal();
        return;
    }
    if (body.IsEmpty() || body == steps_txt) {
        wxMessageDialog msgDlg(this,
                               _("Please describe steps you took before something went wrong."),
                               _("Input is required"),
                               wxOK |wxICON_INFORMATION);
        msgDlg.ShowModal();
        return;
    }
    
    if (!user_github.IsEmpty()) {
        body << "\\n\\n@" << user_github << " " << email ;
    }
    
    body << "\\n\\n";
    
    // parse results
    string result = CreateIssue(title, body);
    
    if (!result.empty()) {
        json_spirit::Value v;
        try {
            if (!json_spirit::read(result, v)) {
                throw std::runtime_error("Could not parse title as JSON");
            }
            json_spirit::Value url_val;
            if (!GdaJson::findValue(v, url_val, "html_url")) {
                throw std::runtime_error("could not find url");
            }
            wxString url_issue = url_val.get_str();
            ReportResultDlg dlg(this, url_issue);
            dlg.ShowModal();
            EndDialog(wxID_OK);
            return;
        } catch (std::runtime_error e) {
            wxString msg;
            msg << "JSON parsing failed: ";
            msg << e.what();
        }
    }
    
    wxMessageDialog msgDlg(this,
                           _("Oops. GeoDa encountered an error to submit bug report to our Github site. Please try again or send us an email spatial@uchicago.edu"),
                           _("Submit Bug Error"),
                           wxOK |wxICON_INFORMATION);
    msgDlg.ShowModal();
    
    LOG_MSG("Submit Bug Report Error:");
    LOG_MSG("title:");
    LOG_MSG(title);
    LOG_MSG("body:");
    LOG_MSG(body);
    LOG_MSG("return from Github:");
    LOG_MSG(result);
}

void ReportBugDlg::OnCancelClick(wxCommandEvent& event)
{
    
}

string ReportBugDlg::CreateIssue(wxString title, wxString body)
{
    // get log file to body
    wxString logger_path;
    if (GeneralWxUtils::isMac()) {
        logger_path <<  GenUtils::GetBasemapCacheDir() <<  "../../../logger.txt";
    } else {
        logger_path <<  GenUtils::GetBasemapCacheDir() << "\\logger.txt";
    }
    wxTextFile tfile;
    tfile.Open(logger_path);
    
    body << "\\n";
    
    while(!tfile.Eof())
    {
        body << tfile.GetNextLine() << "\\n";
    }
    
    wxString labels = "[\"AutoBugReport\"]";
    //wxString assignees = "[\"GeoDaTester\"]";
    
    wxString msg_templ = "{\"title\": \"%s\", \"body\": \"%s\", \"labels\": %s}";
    wxString json_msg = wxString::Format(msg_templ, title, body, labels);
    
    string msg( json_msg.c_str());
    
    return CreateIssueOnGithub(msg);
}
