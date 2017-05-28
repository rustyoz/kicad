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

#include "trackitems.h"
#include <pcbnew_id.h>
#include <menus_helpers.h>

using namespace TrackNodeItem;

const wxString TEARDROPS::TXT_TEARDROP = _("Teardrop");
const wxString TEARDROPS::TXT_TJUNCTION = _("T-Junction");
const wxString TEARDROPS::TXT_JUNCTION = _("Junction");


bool TEARDROPS::IsTeardropPlace(const TRACK* aTrackSeg, const wxPoint& aPos) const
{
    bool end_nearest = EndPosNearest(aTrackSeg, aPos);
    return (end_nearest && ((m_Parent->NextVia(aTrackSeg) || (m_Parent->NextPad(aTrackSeg))))) || (!end_nearest && ((m_Parent->BackVia(aTrackSeg) || (m_Parent->BackPad(aTrackSeg)))));
}

void TEARDROPS::Menu_TrackAdd(wxMenu* aMenu, const TRACK* aTrackSeg, const wxPoint& aPos) const
{
    if(aMenu && aTrackSeg && IsOn())
    {
        wxString msg;
        TEARDROP::PARAMS current_params = GetShapeParams(GetCurrentShape());
        bool teardrop_place = IsTeardropPlace(aTrackSeg, aPos);
        if(aTrackSeg->Type() == PCB_VIA_T)
        {
            if(Empty(static_cast<VIA*>(const_cast<TRACK*>(aTrackSeg))))
            {
                msg.Printf(_("Add Teardrops [%s] to Via"), GetChars(ParamsTxtToMenu(current_params)));
                AddMenuItem(aMenu, ID_POPUP_PCB_TEARDROPS_PLACE, msg, KiBitmap(via_xpm));
            }
            int num_locked_tears = 0;
            int num_tears = Contains(static_cast<VIA*>(const_cast<TRACK*>(aTrackSeg)), num_locked_tears);
            if(num_tears)
            {
                if(num_locked_tears < num_tears)
                    AddMenuItem(aMenu, ID_POPUP_PCB_TEARDROPS_LOCK, _("Lock Teardrops of Via"), KiBitmap(locked_xpm) );
                if(num_locked_tears)
                    AddMenuItem(aMenu, ID_POPUP_PCB_TEARDROPS_UNLOCK, _("Unlock Teardrops of Via"), KiBitmap(unlocked_xpm) );
            }
            teardrop_place = true;
        }
        else
        {
            bool end_nearest = EndPosNearest(aTrackSeg, aPos);
            TEARDROP* tear = dynamic_cast<TEARDROP*>(Get(aTrackSeg, aPos));
            wxPoint track_pos = TrackSegNearestEndpoint(aTrackSeg, aPos);
            TRACK* seg_next;
            TRACK* seg_back;
            bool at_t = Find_T_Tracks(aTrackSeg, track_pos, seg_next, seg_back);
            if(tear)
            {
                TEARDROP::PARAMS tear_params = tear->GetParams();
                if(tear_params != current_params)
                {
                    msg.Printf(_("Copy Teardrop Settings [%s] to Current"), GetChars(ParamsTxtToMenu(tear_params)));
                    if(!dynamic_cast<TEARDROP_JUNCTIONS*>(tear))
                        teardrop_place = true;
                    else
                        if(at_t)        
                            msg.replace(msg.find(TXT_TEARDROP), TXT_TEARDROP.Len(), TXT_TJUNCTION);
                        else
                            msg.replace(msg.find(TXT_TEARDROP), TXT_TEARDROP.Len(), TXT_JUNCTION);
                    AddMenuItem(aMenu, ID_POPUP_PCB_TEARDROP_COPYCURRENT, msg, KiBitmap(tools_xpm) );
                }
            }
            if(!tear || (end_nearest && !Get(aTrackSeg, aTrackSeg->GetEnd())) || (!end_nearest && !Get(aTrackSeg, aTrackSeg->GetStart())))
            {
                if(!(dynamic_cast<TEARDROP*>(Next(aTrackSeg))&&(dynamic_cast<TEARDROP*>(Back(aTrackSeg)))))
                {
                    if(teardrop_place)
                    {
                        if((end_nearest && ((m_Parent->NextVia(aTrackSeg) && !dynamic_cast<TEARDROP_VIA*>(Next(aTrackSeg))) || (m_Parent->NextPad(aTrackSeg) && !dynamic_cast<TEARDROP_PAD*>(Next(aTrackSeg))))) ||                         (!end_nearest && ((m_Parent->BackVia(aTrackSeg) && !dynamic_cast<TEARDROP_VIA*>(Back(aTrackSeg))) || (m_Parent->BackPad(aTrackSeg) && !dynamic_cast<TEARDROP_PAD*>(Back(aTrackSeg))))))
                        {
                            msg.Printf(_("Add Teardrop [%s] to Track [%s]"), GetChars(ParamsTxtToMenu(current_params)), GetChars(aTrackSeg->GetLayerName()));
                            AddMenuItem(aMenu, ID_POPUP_PCB_TEARDROP_PLACE, msg, KiBitmap( via_xpm ));
                        }
                    }
                    else
                    {
                        if((!m_Parent->NextVia(aTrackSeg) && !dynamic_cast<TEARDROP_JUNCTIONS*>(Next(aTrackSeg))) || (!m_Parent->BackVia(aTrackSeg) && !dynamic_cast<TEARDROP_JUNCTIONS*>(Back(aTrackSeg))) || (!m_Parent->BackPad(aTrackSeg) && !dynamic_cast<TEARDROP_JUNCTIONS*>(Back(aTrackSeg))) || (!m_Parent->NextPad(aTrackSeg) && !dynamic_cast<TEARDROP_JUNCTIONS*>(Next(aTrackSeg))))
                        {
                            msg.Printf(_("Add T-Junction [%s] to Track [%s]"), GetChars(ParamsTxtToMenu(current_params)), GetChars(aTrackSeg->GetLayerName()));
                            if(at_t)
                            {
                                AddMenuItem( aMenu, ID_POPUP_PCB_TEARDROP_PLACE, msg, KiBitmap( via_xpm ) );
                            }
                            else
                            {
                                msg.erase(msg.find(_("T-")),2);
                                AddMenuItem( aMenu, ID_POPUP_PCB_TEARDROP_PLACE, msg, KiBitmap( via_xpm ) );
                            }
                                
                        }
                    }
                }
            }
            if(tear)
            {
                TEARDROP::PARAMS tear_params = tear->GetParams();
                msg.Printf(_("Lock Teardrop [%s]"), GetChars(ParamsTxtToMenu(tear_params)));
                wxBitmap pix_map = KiBitmap(locked_xpm);
                if(tear->IsLocked())
                {
                    msg.Printf(_("Unlock Teardrop [%s]"), GetChars(ParamsTxtToMenu(tear_params)));
                    pix_map = KiBitmap(unlocked_xpm);
                }
                if(!dynamic_cast<TEARDROP_JUNCTIONS*>(tear))
                    teardrop_place = true;
                else
                    if(at_t)        
                        msg.replace(msg.find(TXT_TEARDROP), TXT_TEARDROP.Len(), TXT_TJUNCTION);
                    else
                        msg.replace(msg.find(TXT_TEARDROP), TXT_TEARDROP.Len(), TXT_JUNCTION);
                AddMenuItem(aMenu, ID_POPUP_PCB_TEARDROP_LOCK_TOGGLE, msg, pix_map);
            }
        }
    }
}

void TEARDROPS::Menu_TrackChange(wxMenu* aMenu, const TRACK* aTrackSeg, const wxPoint& aPos) const
{
    if(aMenu && aTrackSeg && IsOn())
    {
        wxString msg;
        TEARDROP::PARAMS current_params = GetShapeParams(GetCurrentShape());
        if(aTrackSeg->Type() == PCB_VIA_T)
        {
            int num_locked_tears = 0;
            if(Contains(static_cast<VIA*>(const_cast<TRACK*>(aTrackSeg)), num_locked_tears))
            {
                msg.Printf(_("Change Teardrops of Via to [%s]"), GetChars(ParamsTxtToMenu(current_params)));
                AddMenuItem(aMenu, ID_POPUP_PCB_TEARDROPS_CHANGE, msg, KiBitmap(options_pad_xpm));
            }
        }
        else
        {
            TEARDROP* tear = dynamic_cast<TEARDROP*>(Get(aTrackSeg, aPos));
            if(tear)
            {
                if(!tear->IsLocked())
                {
                    TEARDROP::PARAMS tear_params = tear->GetParams();
                    if(tear_params != current_params)
                    {
                        msg.Printf(_("Change Teardrop [%s] to [%s]"), GetChars(ParamsTxtToMenu(tear_params)), GetChars(ParamsTxtToMenu(current_params)));
                        if(dynamic_cast<TEARDROP_VIA*>(tear) || dynamic_cast<TEARDROP_PAD*>(tear))
                        {
                            AddMenuItem(aMenu, ID_POPUP_PCB_TEARDROP_CHANGE, msg, KiBitmap(options_pad_xpm));
                        }
                        else
                        {
                            if(dynamic_cast<TEARDROP_JUNCTIONS*>(tear))
                            {
                                if(dynamic_cast<TEARDROP_JUNCTIONS*>(tear)->Get_T_SegBack() && dynamic_cast<TEARDROP_JUNCTIONS*>(tear)->Get_T_SegNext())
                                {
                                    msg.replace(msg.find(TXT_TEARDROP), TXT_TEARDROP.Len(), TXT_TJUNCTION);
                                    AddMenuItem(aMenu, ID_POPUP_PCB_TEARDROP_CHANGE, msg, KiBitmap(options_pad_xpm));
                                }
                                else
                                {
                                    msg.replace(msg.find(TXT_TEARDROP), TXT_TEARDROP.Len(), TXT_JUNCTION);
                                    AddMenuItem(aMenu, ID_POPUP_PCB_TEARDROP_CHANGE, msg, KiBitmap(options_pad_xpm));
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}

void TEARDROPS::Menu_TrackRemove(wxMenu* aMenu, const TRACK* aTrackSeg, const wxPoint& aPos) const
{
    if(aMenu && aTrackSeg && IsOn())
    {
        if(aTrackSeg->Type() == PCB_VIA_T)
        {
            int num_locked_tears = 0;
            if(Contains(static_cast<VIA*>(const_cast<TRACK*>(aTrackSeg)), num_locked_tears))
                AddMenuItem(aMenu, ID_POPUP_PCB_TEARDROPS_DELETE, _("Remove Teardrops from Via"), KiBitmap( delete_pad_xpm ));
        } 
        else
        {
            TEARDROP* tear = dynamic_cast<TEARDROP*>(Get(aTrackSeg, aPos));
            if(tear)
            {
                if(!tear->IsLocked())
                {
                    if(dynamic_cast<TEARDROP_VIA*>(tear) || dynamic_cast<TEARDROP_PAD*>(tear))
                    {
                        AddMenuItem(aMenu, ID_POPUP_PCB_TEARDROP_DELETE, _("Remove Teardrop"), KiBitmap(delete_pad_xpm));
                    }
                    else
                    {
                        if(dynamic_cast<TEARDROP_JUNCTIONS*>(tear))
                        {
                            if(dynamic_cast<TEARDROP_JUNCTIONS*>(tear)->Get_T_SegBack() && dynamic_cast<TEARDROP_JUNCTIONS*>(tear)->Get_T_SegNext())
                                AddMenuItem(aMenu, ID_POPUP_PCB_TEARDROP_DELETE, _("Remove T-Junction"), KiBitmap(delete_pad_xpm));
                            else
                                AddMenuItem(aMenu, ID_POPUP_PCB_TEARDROP_DELETE, _("Remove Junction"), KiBitmap(delete_pad_xpm));
                        }
                    }
                }
            }
        }
    }
}

bool TEARDROPS::Menu_NetAdd_Teardrops(wxMenu* aMenu, const TRACK* aTrackSeg, const wxPoint& aPos) const
{
    if(aMenu && aTrackSeg && IsOn())
    {
        wxString msg;
        TEARDROP::PARAMS current_params = GetShapeParams(GetCurrentShape());
        msg.Printf(_("Add Teardrops [%s] to NET: %s" ), GetChars(ParamsTxtToMenu(current_params)), GetChars(m_Board->FindNet(aTrackSeg->GetNetCode())->GetShortNetname()));
        TEARDROP* tear = dynamic_cast<TEARDROP*>(Get(aTrackSeg, aPos));
        bool teardrop_place = IsTeardropPlace(aTrackSeg, aPos);
        if((!tear && teardrop_place) || (tear && !dynamic_cast<TEARDROP_JUNCTIONS*>(tear)) || (aTrackSeg->Type() == PCB_VIA_T))
        {
            if(Empty(aTrackSeg->GetNetCode(), aTrackSeg, ONLY_TEARDROPS_T))
                AddMenuItem(aMenu, ID_POPUP_PCB_TEARDROPS_PLACE_NET, msg, KiBitmap(via_xpm));
            return true;
        }
    }
    return false;
}

void TEARDROPS::Menu_NetAdd_TJunctions(wxMenu* aMenu, const TRACK* aTrackSeg) const
{
    if(aMenu && aTrackSeg && IsOn())
    {
        wxString msg;
        TEARDROP::PARAMS current_params = GetShapeParams(GetCurrentShape());
        msg.Printf(_("Add T-Junctions [%s] to NET: %s" ), GetChars(ParamsTxtToMenu(current_params)), GetChars(m_Board->FindNet(aTrackSeg->GetNetCode())->GetShortNetname()));
        if(Empty(aTrackSeg->GetNetCode(), aTrackSeg, ONLY_TJUNCTIONS_T))
        {
            AddMenuItem(aMenu, ID_POPUP_PCB_TJUNCTIONS_PLACE_NET, msg, KiBitmap(via_xpm));
        }
    }
}

void TEARDROPS::Menu_NetAdd_Junctions(wxMenu* aMenu, const TRACK* aTrackSeg) const
{
    if(aMenu && aTrackSeg && IsOn())
    {
        wxString msg;
        TEARDROP::PARAMS current_params = GetShapeParams(GetCurrentShape());
        msg.Printf(_("Add Junctions [%s] to NET: %s" ), GetChars(ParamsTxtToMenu(current_params)), GetChars(m_Board->FindNet(aTrackSeg->GetNetCode())->GetShortNetname()));
        if(Empty(aTrackSeg->GetNetCode(), aTrackSeg, ONLY_JUNCTIONS_T))
        {
            AddMenuItem(aMenu, ID_POPUP_PCB_JUNCTIONS_PLACE_NET, msg, KiBitmap(via_xpm));
        }
    }
}

bool TEARDROPS::Menu_NetChange_Teardrops(wxMenu* aMenu, const TRACK* aTrackSeg, const wxPoint& aPos) const
{
    if(aMenu && aTrackSeg && IsOn())
    {
        wxString msg;
        TEARDROP::PARAMS current_params = GetShapeParams(GetCurrentShape());
        TEARDROP* tear = dynamic_cast<TEARDROP*>(Get(aTrackSeg, aPos));
        msg.Printf( _("Change Teardrops of NET:%s to [%s]" ), GetChars(m_Board->FindNet(aTrackSeg->GetNetCode())->GetShortNetname()), GetChars(ParamsTxtToMenu(current_params)));
        bool teardrop_place = IsTeardropPlace(aTrackSeg, aPos);
        if((!tear && teardrop_place) || (tear && !dynamic_cast<TEARDROP_JUNCTIONS*>(tear)) || (aTrackSeg->Type() == PCB_VIA_T))
        {
            if(Contains(aTrackSeg->GetNetCode(), aTrackSeg, ONLY_TEARDROPS_T))
                AddMenuItem(aMenu, ID_POPUP_PCB_TEARDROPS_CHANGE_NET, msg, KiBitmap(options_pad_xpm));
            return true;
        }
    }
    return false;
}

void TEARDROPS::Menu_NetChange_TJunctions(wxMenu* aMenu, const TRACK* aTrackSeg) const
{
    if(aMenu && aTrackSeg && IsOn())
    {
        wxString msg;
        TEARDROP::PARAMS current_params = GetShapeParams(GetCurrentShape());
        msg.Printf( _("Change T-Junctions of NET:%s to [%s]" ), GetChars(m_Board->FindNet(aTrackSeg->GetNetCode())->GetShortNetname()), GetChars(ParamsTxtToMenu(current_params)));
        if(Contains(aTrackSeg->GetNetCode(), aTrackSeg, ONLY_TJUNCTIONS_T))
        {
            AddMenuItem(aMenu, ID_POPUP_PCB_TJUNCTIONS_CHANGE_NET, msg, KiBitmap(options_pad_xpm));
        }
    }
}

void TEARDROPS::Menu_NetChange_Junctions(wxMenu* aMenu, const TRACK* aTrackSeg) const
{
    if(aMenu && aTrackSeg && IsOn())
    {
        wxString msg;
        TEARDROP::PARAMS current_params = GetShapeParams(GetCurrentShape());
        msg.Printf( _("Change Junctions of NET:%s to [%s]" ), GetChars(m_Board->FindNet(aTrackSeg->GetNetCode())->GetShortNetname()), GetChars(ParamsTxtToMenu(current_params)));
        if(Contains(aTrackSeg->GetNetCode(), aTrackSeg, ONLY_JUNCTIONS_T))
        {
            AddMenuItem(aMenu, ID_POPUP_PCB_JUNCTIONS_CHANGE_NET, msg, KiBitmap(options_pad_xpm));
        }
    }
}

bool TEARDROPS::Menu_NetRemove_Teardrops(wxMenu* aMenu, const TRACK* aTrackSeg, const wxPoint& aPos) const
{
    if(aMenu && aTrackSeg && IsOn())
    {
        wxString msg;
        TEARDROP* tear = dynamic_cast<TEARDROP*>(Get(aTrackSeg, aPos));
        msg.Printf( _("Remove Teardrops from NET:%s" ), GetChars(m_Board->FindNet(aTrackSeg->GetNetCode())->GetShortNetname()));
        bool teardrop_place = IsTeardropPlace(aTrackSeg, aPos);
        if((!tear && teardrop_place) || (tear && !dynamic_cast<TEARDROP_JUNCTIONS*>(tear)) || (aTrackSeg->Type() == PCB_VIA_T))
        {
            if(Contains(aTrackSeg->GetNetCode(), aTrackSeg, ONLY_TEARDROPS_T))
                AddMenuItem(aMenu, ID_POPUP_PCB_TEARDROPS_DELETE_NET, msg, KiBitmap( delete_pad_xpm));
            return true;
        }
    }
    return false;
}

void TEARDROPS::Menu_NetRemove_TJunctions(wxMenu* aMenu, const TRACK* aTrackSeg) const
{
    if(aMenu && aTrackSeg && IsOn())
    {
        wxString msg;
        msg.Printf( _("Remove T-Junctions from NET:%s" ), GetChars(m_Board->FindNet(aTrackSeg->GetNetCode())->GetShortNetname()));
        if(Contains(aTrackSeg->GetNetCode(), aTrackSeg, ONLY_TJUNCTIONS_T))
        {
            AddMenuItem(aMenu, ID_POPUP_PCB_TJUNCTIONS_DELETE_NET, msg, KiBitmap( delete_pad_xpm));
        }
    }
}

void TEARDROPS::Menu_NetRemove_Junctions(wxMenu* aMenu, const TRACK* aTrackSeg) const
{
    if(aMenu && aTrackSeg && IsOn())
    {
        wxString msg;
        msg.Printf( _("Remove Junctions from NET:%s" ), GetChars(m_Board->FindNet(aTrackSeg->GetNetCode())->GetShortNetname()));
        if(Contains(aTrackSeg->GetNetCode(), aTrackSeg, ONLY_JUNCTIONS_T))
        {
            AddMenuItem(aMenu, ID_POPUP_PCB_JUNCTIONS_DELETE_NET, msg, KiBitmap( delete_pad_xpm));
        }
    }
}

void TEARDROPS::Menu_NetLock_Teardrops(wxMenu* aMenu, const TRACK* aTrackSeg, const wxPoint& aPos) const
{
    if(aMenu && aTrackSeg && IsOn())
    {
        wxString msg;
        msg.Printf(_("Lock All in NET: %s" ), GetChars(m_Board->FindNet(aTrackSeg->GetNetCode())->GetShortNetname()));
        AddMenuItem(aMenu, ID_POPUP_PCB_TEARDROPS_LOCK_NET, msg, KiBitmap(locked_xpm));
    }
}

void TEARDROPS::Menu_NetUnlock_Teardrops(wxMenu* aMenu, const TRACK* aTrackSeg, const wxPoint& aPos) const
{
    if(aMenu && aTrackSeg && IsOn())
    {
        wxString msg;
        msg.Printf(_("Unlock All in NET: %s" ), GetChars(m_Board->FindNet(aTrackSeg->GetNetCode())->GetShortNetname()));
        AddMenuItem(aMenu, ID_POPUP_PCB_TEARDROPS_UNLOCK_NET, msg, KiBitmap(unlocked_xpm));
    }
}

void TEARDROPS::Menu_PadAdd(wxMenu* aMenu, const D_PAD* aPad) const
{
    if(aMenu && aPad && IsOn())
    {
        wxString msg;
        TEARDROP::PARAMS current_params = GetShapeParams(GetCurrentShape());
        msg.Printf(_("Add Teardrops [%s] to %s"), GetChars(ParamsTxtToMenu(current_params)), GetChars(aPad->GetSelectMenuText()));
        AddMenuItem(aMenu, ID_POPUP_PCB_TEARDROPS_PLACE, msg, KiBitmap(pad_xpm));
    }
}

void TEARDROPS::Menu_PadChange(wxMenu* aMenu, const D_PAD* aPad) const
{
    if(aMenu && aPad && IsOn())
    {
        wxString msg;
        TEARDROP::PARAMS current_params = GetShapeParams(GetCurrentShape());
        msg.Printf(_("Change Teardrops of %s to [%s]"), GetChars(aPad->GetSelectMenuText()), GetChars(ParamsTxtToMenu(current_params)));
        AddMenuItem(aMenu, ID_POPUP_PCB_TEARDROPS_CHANGE, msg, KiBitmap(options_pad_xpm));
    }
}

void TEARDROPS::Menu_PadRemove(wxMenu* aMenu, const D_PAD* aPad) const
{
    if(aMenu && aPad && IsOn())
    {
        wxString msg;
        msg.Printf(_("Remove Teardrops from %s"), GetChars(aPad->GetSelectMenuText()));
        AddMenuItem(aMenu, ID_POPUP_PCB_TEARDROPS_DELETE, msg, KiBitmap(delete_pad_xpm));
    }
}

void TEARDROPS::Menu_ModuleAdd(wxMenu* aMenu, const MODULE* aModule) const
{
    if(aMenu && aModule && IsOn())
    {
        wxString msg;
        TEARDROP::PARAMS current_params = GetShapeParams(GetCurrentShape());
        msg.Printf(_("Add Teardrops [%s] to %s"), GetChars(ParamsTxtToMenu(current_params)), GetChars(aModule->GetSelectMenuText()));
        AddMenuItem(aMenu, ID_POPUP_PCB_TEARDROPS_PLACE_MODULE, msg, KiBitmap( pad_xpm ));
    }
}

void TEARDROPS::Menu_ModuleChange(wxMenu* aMenu, const MODULE* aModule) const
{
    if(aMenu && aModule && IsOn())
    {
        wxString msg;
        TEARDROP::PARAMS current_params = GetShapeParams(GetCurrentShape());
        msg.Printf(_("Change Teardrops of %s to [%s]"), GetChars(aModule->GetSelectMenuText()), GetChars(ParamsTxtToMenu(current_params)));
        AddMenuItem(aMenu, ID_POPUP_PCB_TEARDROPS_CHANGE_MODULE, msg, KiBitmap(options_pad_xpm));
    }
}

void TEARDROPS::Menu_ModuleRemove(wxMenu* aMenu, const MODULE* aModule) const
{
    if(aMenu && aModule && IsOn())
    {
        wxString msg;
        msg.Printf(_("Remove Teardrops from %s"), GetChars(aModule->GetSelectMenuText()));
        AddMenuItem(aMenu, ID_POPUP_PCB_TEARDROPS_DELETE_MODULE, msg, KiBitmap(delete_pad_xpm));
    }
}


void TEARDROPS::Menu_LockAllSameParam(wxMenu* aMenu, const TRACK* aTrackSeg, const wxPoint& aPos) const
{
    if(aMenu && aTrackSeg && IsOn())
    {
        if(aTrackSeg->Type() == PCB_TRACE_T)
        {
            TEARDROP* tear = dynamic_cast<TEARDROP*>(Get(aTrackSeg, aPos));
            if(tear)
            {
                aMenu->AppendSeparator();
                wxString msg;
                TEARDROP::PARAMS tear_params = tear->GetParams();
                msg.Printf(_("Lock All [%s]"), GetChars(ParamsTxtToMenu(tear_params)));
                AddMenuItem(aMenu, ID_POPUP_PCB_TEARDROPS_LOCK_ALL_SAME, msg, KiBitmap(locked_xpm) );
                msg.Printf(_("Unlock All [%s]"), GetChars(ParamsTxtToMenu(tear_params)));
                AddMenuItem(aMenu, ID_POPUP_PCB_TEARDROPS_UNLOCK_ALL_SAME, msg, KiBitmap(unlocked_xpm) );
            }
        }
    }
}

//To Track or Via
void TEARDROPS::Popup(wxMenu* aMenu, const TRACK* aTrackSeg, const wxPoint& aPos) const
{
    if(aTrackSeg && aMenu && IsOn())
    {
        wxString msg;
        wxPoint track_nearest_endpoint = TrackSegNearestEndpoint(aTrackSeg, aPos); 
        TEARDROP* tear = dynamic_cast<TEARDROP*>(Get(aTrackSeg, track_nearest_endpoint));
        bool teardrop_place = IsTeardropPlace(aTrackSeg, track_nearest_endpoint) || (aTrackSeg->Type() == PCB_VIA_T) ||  (dynamic_cast<TEARDROP_VIA*>(tear) || dynamic_cast<TEARDROP_PAD*>(tear)) || CanEdit();
        if(teardrop_place)
            msg.Printf(_("Teardrops"));
        else
            msg.Printf(_("Junctions / T-Junctions"));
        wxMenu* tear_menu = new wxMenu;
        AddMenuItem(aMenu, tear_menu, ID_POPUP_PCB_TEARDROPS_COMMON_MNU, msg, KiBitmap(pad_xpm));
        if(!CanEdit())
        {
            size_t menu_count = tear_menu->GetMenuItemCount();
            Menu_TrackAdd(tear_menu, aTrackSeg, track_nearest_endpoint);
            Menu_TrackChange(tear_menu, aTrackSeg, track_nearest_endpoint);
            Menu_TrackRemove(tear_menu, aTrackSeg, track_nearest_endpoint);
            if(tear_menu->GetMenuItemCount() > menu_count)
                tear_menu->AppendSeparator();
            
            menu_count = tear_menu->GetMenuItemCount();
            Menu_NetLock_Teardrops(tear_menu, aTrackSeg, track_nearest_endpoint);
            Menu_NetUnlock_Teardrops(tear_menu, aTrackSeg, track_nearest_endpoint);
            bool teardrop_add = false;
            bool teardrop_change = false;
            bool teardrop_delete = false;
            teardrop_add = Menu_NetAdd_Teardrops(tear_menu, aTrackSeg, track_nearest_endpoint);
            teardrop_change = Menu_NetChange_Teardrops(tear_menu, aTrackSeg, track_nearest_endpoint);
            teardrop_delete = Menu_NetRemove_Teardrops(tear_menu, aTrackSeg, track_nearest_endpoint);
            if(tear_menu->GetMenuItemCount() > menu_count)
                tear_menu->AppendSeparator();

            menu_count = tear_menu->GetMenuItemCount();
            if(!teardrop_add)
                Menu_NetAdd_TJunctions(tear_menu, aTrackSeg);
            if(!teardrop_change)
                Menu_NetChange_TJunctions(tear_menu, aTrackSeg);
            if(!teardrop_delete)
                Menu_NetRemove_TJunctions(tear_menu, aTrackSeg);
            if(tear_menu->GetMenuItemCount() > menu_count)
                tear_menu->AppendSeparator();

            menu_count = tear_menu->GetMenuItemCount();
            if(!teardrop_add)
                Menu_NetAdd_Junctions(tear_menu, aTrackSeg);
            if(!teardrop_change)
                Menu_NetChange_Junctions(tear_menu, aTrackSeg);
            if(!teardrop_delete)
                Menu_NetRemove_Junctions(tear_menu, aTrackSeg);
            if(tear_menu->GetMenuItemCount() > menu_count)
                tear_menu->AppendSeparator();
        }
        Menu_Select(tear_menu);
        if(!CanEdit())
        {
            Menu_LockAllSameParam(tear_menu, aTrackSeg, track_nearest_endpoint);
        }
    }
}

//One Pad
void TEARDROPS::Popup(wxMenu* aMenu, const D_PAD* aPad) const
{
    if(aMenu && aPad && IsOn())
    {
        wxMenu* pad_menu = new wxMenu;
        AddMenuItem(aMenu, pad_menu, ID_POPUP_PCB_TEARDROPS_COMMON_MNU, _("Teardrops"), KiBitmap(pad_xpm));
        
        size_t menu_count = pad_menu->GetMenuItemCount();
        
        if(Empty(aPad))
            Menu_PadAdd(pad_menu, aPad);
        
        int num_locked_tears = 0;
        int num_tears = Contains(aPad, num_locked_tears);
        if(num_tears)
        {
            if(num_locked_tears < num_tears)
                AddMenuItem(pad_menu, ID_POPUP_PCB_TEARDROPS_LOCK, _("Lock Teardrops of Pad"), KiBitmap(locked_xpm) );
            if(num_locked_tears)
                AddMenuItem(pad_menu, ID_POPUP_PCB_TEARDROPS_UNLOCK, _("Unlock Teardrops of Pad"), KiBitmap(unlocked_xpm) );
            Menu_PadChange(pad_menu, aPad);
            Menu_PadRemove(pad_menu, aPad);
        }
        
        if(pad_menu->GetMenuItemCount() > menu_count)
            pad_menu->AppendSeparator();
        
        Menu_Select(pad_menu);
    }
}

//One Module
void TEARDROPS::Popup(wxMenu* aMenu, const MODULE* aModule) const
{
    if(aMenu && aModule && IsOn())
    {
        wxMenu* module_menu = new wxMenu;
        AddMenuItem(aMenu, module_menu, ID_POPUP_PCB_TEARDROPS_COMMON_MNU, _("Teardrops"), KiBitmap(pad_xpm));
        
        size_t menu_count = module_menu->GetMenuItemCount();
        
        if(Empty(aModule))
            Menu_ModuleAdd(module_menu, aModule);
        
        int num_locked_tears = 0;
        int num_tears = Contains(aModule, num_locked_tears);
        if(num_tears)
        {
            wxString msg;
            if(num_locked_tears < num_tears)
            {
                msg.Printf(_("Lock Teardrops of %s"), GetChars(aModule->GetSelectMenuText()));
                AddMenuItem(module_menu, ID_POPUP_PCB_TEARDROPS_LOCK_MODULE, msg, KiBitmap(locked_xpm));
            }
            if(num_locked_tears)
            {
                msg.Printf(_("Unlock Teardrops of %s"), GetChars(aModule->GetSelectMenuText()));
                AddMenuItem(module_menu, ID_POPUP_PCB_TEARDROPS_UNLOCK_MODULE, msg, KiBitmap(unlocked_xpm));
            }
            Menu_ModuleChange(module_menu, aModule);
            Menu_ModuleRemove(module_menu, aModule);
        }
        
        if(module_menu->GetMenuItemCount() > menu_count)
            module_menu->AppendSeparator();
        
        Menu_Select(module_menu);
    }
}

