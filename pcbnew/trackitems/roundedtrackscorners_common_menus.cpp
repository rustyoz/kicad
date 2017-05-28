/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) Heikki Pulkkinen.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#include "roundedtrackscorners.h"

using namespace TrackNodeItem;

void ROUNDEDTRACKSCORNERS::CreateMenu(wxMenu* aMenu) const
{
    if(aMenu)
    {
        if(IsOn())
        {
            wxString add_s;
            add_s.Printf(_("Add %s"), GetChars(ParamsTxtToMenu(GetParams())));
            AddMenuItem(aMenu, ID_POPUP_PCB_ROUNDEDTRACKSCORNERS_PLACE_ALL, add_s, KiBitmap(add_arc_xpm));
            AddMenuItem(aMenu, ID_POPUP_PCB_ROUNDEDTRACKSCORNERS_DELETE_ALL, _("Remove"), KiBitmap(delete_arc_xpm));
            aMenu->AppendSeparator();
        }
        Menu_ChangeSize(aMenu);
    }
}

int ROUNDEDTRACKSCORNERS::MenuToDo_CalcSizeLengthSet(const int aMenuID)
{
    return (aMenuID - ID_POPUP_PCB_ROUNDEDTRACKSCORNERS_SIZE_LENGTH_SET_FIRST) * ROUNDEDTRACKSCORNER::DEFAULT_MIN_LENGTH_SET;
}

int ROUNDEDTRACKSCORNERS::MenuToDo_CalcSizeLengthRatio(const int aMenuID)
{
    return (aMenuID - ID_POPUP_PCB_ROUNDEDTRACKSCORNERS_SIZE_LENGTH_RATIO_FIRST) * 10;
}

void ROUNDEDTRACKSCORNERS::MenuToDo_ChangeSize(const int aMenuID)
{
    ROUNDEDTRACKSCORNER::PARAMS params = GetParams();
    if((aMenuID >= ID_POPUP_PCB_ROUNDEDTRACKSCORNERS_SIZE_LENGTH_SET_FIRST) && (aMenuID <= ID_POPUP_PCB_ROUNDEDTRACKSCORNERS_SIZE_LENGTH_SET_LAST))
        params.length_set = MenuToDo_CalcSizeLengthSet(aMenuID);
    if((aMenuID >= ID_POPUP_PCB_ROUNDEDTRACKSCORNERS_SIZE_LENGTH_RATIO_FIRST) && (aMenuID <= ID_POPUP_PCB_ROUNDEDTRACKSCORNERS_SIZE_LENGTH_RATIO_LAST))
        params.length_ratio = MenuToDo_CalcSizeLengthRatio(aMenuID);
        
    SetParams(params);
    m_EditFrame->GetCanvas()->Refresh();
    RecreateMenu();
}

void ROUNDEDTRACKSCORNERS::Menu_ChangeSize(wxMenu* aMenu) const
{
    if(aMenu)
    {
        wxString msg;
        ROUNDEDTRACKSCORNER::PARAMS current_params = GetParams();
        wxMenu* size_length_set_menu = new wxMenu;
        msg.Printf(_("Select Length"));
        AddMenuItem(aMenu, size_length_set_menu, ID_POPUP_PCB_ROUNDEDTRACKSCORNERS_COMMON_MNU, msg, KiBitmap(tools_xpm));
        for(int n = ID_POPUP_PCB_ROUNDEDTRACKSCORNERS_SIZE_LENGTH_SET_FIRST; n <= ID_POPUP_PCB_ROUNDEDTRACKSCORNERS_SIZE_LENGTH_SET_LAST; ++n)
        {
            uint length_set = const_cast<ROUNDEDTRACKSCORNERS*> (this)->MenuToDo_CalcSizeLengthSet(n);
            if(length_set != current_params.length_set)
            {
                if(!length_set)
                    msg.Printf(_("Track Width"));
                else
                    msg = m_EditFrame->LengthDoubleToString(length_set, true);
                size_length_set_menu->Append(n, msg, wxEmptyString, false);
            }
        }
        
        wxMenu* size_length_ratio_menu = new wxMenu;
        msg.Printf(_("Select Length Ratio"));
        AddMenuItem(aMenu, size_length_ratio_menu, ID_POPUP_PCB_ROUNDEDTRACKSCORNERS_COMMON_MNU, msg, KiBitmap(tools_xpm));
        for(int n = ID_POPUP_PCB_ROUNDEDTRACKSCORNERS_SIZE_LENGTH_RATIO_FIRST; n <= ID_POPUP_PCB_ROUNDEDTRACKSCORNERS_SIZE_LENGTH_RATIO_LAST; ++n)
        {
            int length_ratio = const_cast<ROUNDEDTRACKSCORNERS*> (this)->MenuToDo_CalcSizeLengthRatio(n);
            if(length_ratio != current_params.length_ratio)
            {
                if(!length_ratio)
                    msg.Printf(_("OFF"));
                else
                    msg.Printf(_("%d"), length_ratio);
                size_length_ratio_menu->Append(n, msg, wxEmptyString, false);
            }
        }

        if(CanEdit() && !IsEditOn())
        {
            AddMenuItem(size_length_set_menu, ID_POPUP_PCB_ROUNDEDTRACKSCORNERS_EDIT_LENGTH_SET, _("Custom"), KiBitmap(tools_xpm));
            AddMenuItem(size_length_ratio_menu, ID_POPUP_PCB_ROUNDEDTRACKSCORNERS_EDIT_LENGTH_RATIO, _("Custom"), KiBitmap(tools_xpm));
        }

        ROUNDEDTRACKSCORNER::PARAMS default_params = GetDefaultParams();
        if(current_params != default_params)
        {
            msg.Printf(_("Set Default %s"), GetChars(ParamsTxtToMenu(default_params)));
            AddMenuItem(aMenu, ID_POPUP_PCB_ROUNDEDTRACKSCORNERS_SET_DEFAULT_PARAMS, msg, KiBitmap(hammer_xpm));
        }
    }
}

wxString ROUNDEDTRACKSCORNERS::ParamsTxtToMenu(const ROUNDEDTRACKSCORNER::PARAMS aParams) const
{
    wxString return_s = m_EditFrame->LengthDoubleToString(aParams.length_set, true);
    if(!aParams.length_set)
        return_s.Printf( _("Track Width"));
    return_s.Printf(_("[%s:%d%]"), return_s, aParams.length_ratio);
    return return_s;
}



