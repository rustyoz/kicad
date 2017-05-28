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
#include <bitmaps.h>
#include <pcbnew_id.h>
#include <menus_helpers.h>

using namespace TrackNodeItem;


void ROUNDEDTRACKSCORNERS::Popup(wxMenu* aMenu, const TRACK* aTrackSeg, const wxPoint& aPos) const
{
    if(aTrackSeg && aMenu)
    {
        wxString msg;
        wxPoint track_nearest_endpoint = TrackNodeItem::TrackSegNearestEndpoint(aTrackSeg, aPos); 
        ROUNDEDTRACKSCORNER* corner = dynamic_cast<ROUNDEDTRACKSCORNER*>(Get(aTrackSeg, track_nearest_endpoint));
        msg.Printf(_("Rounded Corners"));

        wxMenu* corner_menu = new wxMenu;
        AddMenuItem(aMenu, corner_menu, ID_POPUP_PCB_ROUNDEDTRACKSCORNERS_COMMON_MNU, msg, KiBitmap(add_tracks_xpm));
        if(IsOn())
        {
            if(!corner)
                Menu_AddToTrack(corner_menu, aTrackSeg, track_nearest_endpoint);
            if(corner)
            {
                if(corner->GetParams() != GetParams())
                {
                    Menu_CopyParamsToCurrent(corner_menu, aTrackSeg, track_nearest_endpoint);
                    Menu_ChangeFromTrack(corner_menu, aTrackSeg, track_nearest_endpoint);
                }
                Menu_RemoveFromTrack(corner_menu, aTrackSeg, track_nearest_endpoint);
            }
            corner_menu->AppendSeparator();
        }
        Menu_ChangeSize(corner_menu);
    }
}

void ROUNDEDTRACKSCORNERS::Menu_AddToTrack(wxMenu* aMenu, const TRACK* aTrackSeg, const wxPoint& aPos) const
{
    if(aMenu && aTrackSeg && IsOn())
    {
        wxString msg;
        msg.Printf(_("Add Corner %s"), GetChars(ParamsTxtToMenu(GetParams())));
        AddMenuItem(aMenu, ID_POPUP_PCB_ROUNDEDTRACKSCORNER_PLACE, msg, KiBitmap(add_arc_xpm));
    }
}

void ROUNDEDTRACKSCORNERS::Menu_RemoveFromTrack(wxMenu* aMenu, const TRACK* aTrackSeg, const wxPoint& aPos) const
{
    if(aMenu && aTrackSeg && IsOn())
    {
        if(aTrackSeg->Type() == PCB_TRACE_T)
        {
            ROUNDEDTRACKSCORNER* corner = dynamic_cast<ROUNDEDTRACKSCORNER*>(Get(aTrackSeg, aPos));
            if(corner)
            {
                AddMenuItem(aMenu, ID_POPUP_PCB_ROUNDEDTRACKSCORNER_DELETE, _("Remove Corner"), KiBitmap(delete_arc_xpm));
            }
        }
    }
}

void ROUNDEDTRACKSCORNERS::Menu_ChangeFromTrack(wxMenu* aMenu, const TRACK* aTrackSeg, const wxPoint& aPos) const
{
    if(aMenu && aTrackSeg && IsOn())
    {
        if(aTrackSeg->Type() == PCB_TRACE_T)
        {
            ROUNDEDTRACKSCORNER* corner = dynamic_cast<ROUNDEDTRACKSCORNER*>(Get(aTrackSeg, aPos));
            if(corner)
            {
                wxString msg;
                msg.Printf(_("Change Corner %s"), GetChars(ParamsTxtToMenu(GetParams())));
                AddMenuItem(aMenu, ID_POPUP_PCB_ROUNDEDTRACKSCORNER_CHANGE, msg, KiBitmap(add_arc_xpm));
            }
        }
    }
}

void ROUNDEDTRACKSCORNERS::Menu_CopyParamsToCurrent(wxMenu* aMenu, const TRACK* aTrackSeg, const wxPoint& aPos) const
{
    if(aMenu && aTrackSeg && IsOn())
    {
        if(aTrackSeg->Type() == PCB_TRACE_T)
        {
            ROUNDEDTRACKSCORNER* corner = dynamic_cast<ROUNDEDTRACKSCORNER*>(Get(aTrackSeg, aPos));
            if(corner)
            {
                wxString msg;
                msg.Printf(_("Copy Corner Settings %s to Current"), GetChars(ParamsTxtToMenu(corner->GetParams())));
                AddMenuItem(aMenu, ID_POPUP_PCB_ROUNDEDTRACKSCORNER_COPYCURRENT, msg, KiBitmap(tools_xpm));
            }
        }
    }
}

