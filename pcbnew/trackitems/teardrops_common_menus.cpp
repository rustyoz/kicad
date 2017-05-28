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

#include "teardrops.h"

using namespace TrackNodeItem;

const wxString TEARDROPS::TXT_TEARDROPS = _("Teardrops");


int TEARDROPS::MenuToDo_CalcSizeLength(const int aMenuID)
{
    return (aMenuID - ID_POPUP_PCB_TEARDROP_SIZE_LENGTH_FIRST + 1) * 10;
}

int TEARDROPS::MenuToDo_CalcSizeWidth(const int aMenuID, const TEARDROP::SHAPES_T aShape)
{
    int w = (aMenuID - ID_POPUP_PCB_TEARDROP_SIZE_WIDTH_FIRST + 1) * 10;
    switch(aShape)    
    {
        case TEARDROP::SUBLAND_T:
            if(w >= TEARDROP::SUBLAND_POS_MAX)
                break;
        case TEARDROP::FILLET_T:
        case TEARDROP::TEARDROP_T:
            return w;
        default:;
    }
    return 0;
}

int TEARDROPS::MenuToDo_CalcNumSegs(const int aMenuID, const TEARDROP::SHAPES_T aShape)
{
    if(aShape == TEARDROP::TEARDROP_T)
    {
        int w = aMenuID - ID_POPUP_PCB_TEARDROP_NUM_SEGMENTS_FIRST + 2;
        if(w <= TEARDROP::TEARDROP_SEGS_MAX)
            return w;
    }

    return 0;
}

void TEARDROPS::MenuToDo_ChangeSize(const int aMenuID)
{
    TEARDROP::PARAMS params = GetShapeParams(GetCurrentShape());
    if((aMenuID >= ID_POPUP_PCB_TEARDROP_SIZE_LENGTH_FIRST) && (aMenuID <= ID_POPUP_PCB_TEARDROP_SIZE_LENGTH_LAST))
        if(int size_l = MenuToDo_CalcSizeLength(aMenuID))
            params.length_ratio = size_l;
    if((aMenuID >= ID_POPUP_PCB_TEARDROP_SIZE_WIDTH_FIRST) && (aMenuID <= ID_POPUP_PCB_TEARDROP_SIZE_WIDTH_LAST))
        if(int size_w = MenuToDo_CalcSizeWidth(aMenuID, params.shape))
            params.width_ratio = size_w;
    if((aMenuID >= ID_POPUP_PCB_TEARDROP_NUM_SEGMENTS_FIRST) && (aMenuID <= ID_POPUP_PCB_TEARDROP_NUM_SEGMENTS_LAST))
        if(int num_s = MenuToDo_CalcNumSegs(aMenuID, params.shape))
            params.num_segments = num_s;
    SetShapeParams(params);
    m_EditFrame->GetCanvas()->Refresh();
    RecreateMenu();
}

void TEARDROPS::Menu_ChangeSize(wxMenu* aMenu) const
{
    if(aMenu && IsOn())
    {
        wxString msg;
        TEARDROP::PARAMS current_params = GetShapeParams(GetCurrentShape());
        wxMenu* size_length_menu = new wxMenu;
        (current_params.shape == TEARDROP::SUBLAND_T)? msg.Printf(_("Select Position Ratio")) : msg.Printf(_("Select Length Ratio"));
        AddMenuItem(aMenu, size_length_menu, ID_POPUP_PCB_TEARDROPS_COMMON_MNU, msg, KiBitmap(tools_xpm));
        for(int n = ID_POPUP_PCB_TEARDROP_SIZE_LENGTH_FIRST; n <= ID_POPUP_PCB_TEARDROP_SIZE_LENGTH_LAST; ++n)
        {
            int size_l = const_cast<TEARDROPS*> (this)->MenuToDo_CalcSizeLength(n);
            if(size_l && (size_l != current_params.length_ratio))
            {
                msg.Printf(_("%d"), size_l);
                size_length_menu->Append(n, msg, wxEmptyString, false);
            }
        }
        wxMenu* size_width_menu = new wxMenu;
        msg.Printf(_("Select Width Ratio"));
        AddMenuItem(aMenu, size_width_menu, ID_POPUP_PCB_TEARDROPS_COMMON_MNU, msg, KiBitmap(tools_xpm));
        for(int n = ID_POPUP_PCB_TEARDROP_SIZE_WIDTH_FIRST; n <= ID_POPUP_PCB_TEARDROP_SIZE_WIDTH_LAST; ++n)
        {
            int size_w = const_cast<TEARDROPS*> (this)->MenuToDo_CalcSizeWidth(n, current_params.shape);
            if(size_w && (size_w != current_params.width_ratio))
            {
                msg.Printf(_("%d"), size_w);
                size_width_menu->Append(n, msg, wxEmptyString, false);
            }
        }

        if(current_params.shape == TEARDROP::TEARDROP_T)
        {
            wxMenu* num_segments_menu = new wxMenu;
            msg.Printf(_("Select Number of Segments"));
            AddMenuItem(aMenu, num_segments_menu, ID_POPUP_PCB_TEARDROPS_COMMON_MNU, msg, KiBitmap(tools_xpm));
            for(int n = ID_POPUP_PCB_TEARDROP_NUM_SEGMENTS_FIRST; n <= ID_POPUP_PCB_TEARDROP_NUM_SEGMENTS_LAST; ++n)
            {
                int num_s = const_cast<TEARDROPS*> (this)->MenuToDo_CalcNumSegs(n, current_params.shape);
                if(num_s && (num_s != current_params.num_segments))
                {
                    msg.Printf(_("%d"), num_s);
                    num_segments_menu->Append(n, msg, wxEmptyString, false);
                }
            }
        }
        
        if(CanEdit() && !IsEditOn())
        {
            AddMenuItem(size_length_menu, ID_POPUP_PCB_TEARDROP_EDIT_LENGTH, _("Custom"), KiBitmap(tools_xpm));
            AddMenuItem(size_width_menu, ID_POPUP_PCB_TEARDROP_EDIT_WIDTH, _("Custom"), KiBitmap(tools_xpm));
        }

        TEARDROP::PARAMS default_params = GetDefaultParams(GetCurrentShape());
        if(current_params != default_params)
        {
            msg.Printf(_("Set Default [%s]"), GetChars(ParamsTxtToMenu(default_params)));
            AddMenuItem(aMenu, ID_POPUP_PCB_TEARDROP_SET_DEFAULT_PARAMS, msg, KiBitmap(hammer_xpm));
        }
    }
}

wxString TEARDROPS::ParamsTxtToMenu(const TEARDROP::PARAMS aParams) const
{
    wxString return_s;
    return_s.Printf(_("%s"), GetChars(GetShapeName(aParams.shape)));
    if(aParams.shape != TEARDROP::ZERO_T)
    {
        wxString append_s;
        append_s.Printf(_(":%d:%d"), aParams.length_ratio, aParams.width_ratio);
        return_s.Append(append_s);
        if(aParams.shape == TEARDROP::TEARDROP_T)
        {
            append_s.Printf(_(":%d"),  aParams.num_segments);
            return_s.Append(append_s);
        }
    }
    return return_s;
}

void TEARDROPS::Menu_Select(wxMenu* aMenu) const
{
    if(aMenu)
    {
        wxString msg;
        
        TEARDROP::PARAMS tear_params = GetShapeParams(TEARDROP::TEARDROP_T);
        msg = ParamsTxtToMenu(tear_params);
        aMenu->Append(ID_POPUP_PCB_TEARDROP_SELECT_TEARDROP, msg, wxEmptyString, true);

        tear_params = GetShapeParams(TEARDROP::FILLET_T);
        msg = ParamsTxtToMenu(tear_params);
        aMenu->Append(ID_POPUP_PCB_TEARDROP_SELECT_FILLET, msg, wxEmptyString, true);

        tear_params = GetShapeParams(TEARDROP::SUBLAND_T);
        msg = ParamsTxtToMenu(tear_params);
        aMenu->Append(ID_POPUP_PCB_TEARDROP_SELECT_SUBLAND, msg, wxEmptyString, true);

        tear_params = GetShapeParams(TEARDROP::ZERO_T);
        msg = ParamsTxtToMenu(tear_params);
        aMenu->Append(ID_POPUP_PCB_TEARDROP_SELECT_ZERO, msg, wxEmptyString, true);

        if(IsOn() && (m_current_shape != TEARDROP::ZERO_T))
            const_cast<TEARDROPS*> (this)->Menu_ChangeSize(aMenu);
    }
}

void TEARDROPS::Menu_SelectToDo(wxMenu* aMenu, const int aBaseToDo) const
{
    if(IsOn())
    {
        AddMenuItem(aMenu, aBaseToDo, _("Footprints"), KiBitmap(module_xpm));
        AddMenuItem(aMenu, aBaseToDo + 1, _("Vias"), KiBitmap(via_xpm));
        AddMenuItem(aMenu, aBaseToDo + 2, _("T-Junctions"), KiBitmap(add_junction_xpm));
        AddMenuItem(aMenu, aBaseToDo + 3, _("Junctions"), KiBitmap(add_line_xpm));
    }
}

void TEARDROPS::CreateMenu(wxMenu* aMenu) const
{
    if(aMenu)
    {
        if(IsOn())
        {
            //Add
            wxMenu* add_menu = new wxMenu;
            AddMenuItem(aMenu, add_menu, ID_POPUP_PCB_TEARDROPS_COMMON_MNU, _("Add"), KiBitmap(via_xpm));
            Menu_SelectToDo(add_menu, ID_POPUP_PCB_TEARDROPS_PLACE_MODULES_ALL);
            //Change
            wxMenu* change_menu = new wxMenu;
            AddMenuItem(aMenu, change_menu, ID_POPUP_PCB_TEARDROPS_COMMON_MNU, _("Change"), KiBitmap(options_pad_xpm));
            Menu_SelectToDo(change_menu, ID_POPUP_PCB_TEARDROPS_CHANGE_MODULES_ALL);
            //Remove
            wxMenu* delete_menu = new wxMenu;
            AddMenuItem(aMenu, delete_menu, ID_POPUP_PCB_TEARDROPS_COMMON_MNU, _("Remove"), KiBitmap(delete_pad_xpm));
            Menu_SelectToDo(delete_menu, ID_POPUP_PCB_TEARDROPS_DELETE_MODULES_ALL);
            
            aMenu->AppendSeparator();
        }
        
        Menu_Select(aMenu);
        aMenu->AppendSeparator();
        
        if(IsOn())
        {
            //Lock
            wxMenu* lock_menu = new wxMenu;
            AddMenuItem(aMenu, lock_menu, ID_POPUP_PCB_TEARDROPS_COMMON_MNU, _("Lock"), KiBitmap(locked_xpm));
            Menu_SelectToDo(lock_menu, ID_POPUP_PCB_TEARDROPS_LOCK_MODULES_ALL);
            //Unlock
            wxMenu* unlock_menu = new wxMenu;
            AddMenuItem(aMenu, unlock_menu, ID_POPUP_PCB_TEARDROPS_COMMON_MNU, _("Unlock"), KiBitmap(unlocked_xpm));
            Menu_SelectToDo(unlock_menu, ID_POPUP_PCB_TEARDROPS_UNLOCK_MODULES_ALL);
            
            aMenu->AppendSeparator();

            //List different kind of teardrops.
            wxMenu* list_menu = new wxMenu;
            AddMenuItem(aMenu, list_menu, ID_POPUP_PCB_TEARDROPS_COMMON_MNU, _("Mark Diff"), KiBitmap(tools_xpm));
            Menu_SelectToDo(list_menu, ID_POPUP_PCB_TEARDROPS_MARK_DIFF_MODULES);
            
            //List alla current type teardrops.
            wxString msg;
            TEARDROP::PARAMS tear_params = GetShapeParams(GetCurrentShape());
            msg = ParamsTxtToMenu(tear_params);
            wxString prepend_s;
            prepend_s.Printf(_("Mark: "));
            msg.Prepend(prepend_s);
            wxMenu* list_curr = new wxMenu;
            AddMenuItem(aMenu, list_curr, ID_POPUP_PCB_TEARDROPS_COMMON_MNU, msg, KiBitmap(tools_xpm));
            Menu_SelectToDo(list_curr, ID_POPUP_PCB_TEARDROPS_MARK_CURR_MODULES);

            aMenu->AppendSeparator();
            
            //Warnings
            wxMenu* mark_warnings_menu = new wxMenu;
            AddMenuItem(aMenu, mark_warnings_menu, ID_POPUP_PCB_TEARDROPS_COMMON_MNU, _("Warnings"), KiBitmap(drc_xpm));
            Menu_SelectToDo(mark_warnings_menu, ID_POPUP_PCB_TEARDROPS_MARK_WARNINGS_MODULES);    

        }
    }
}

