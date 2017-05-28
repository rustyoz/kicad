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

#include"tracknodeitems.h"
#include "teardrops.h"
#include "roundedtrackscorners.h"

using namespace TrackNodeItem;
using namespace TrackNodeItems;


TRACKNODEITEMS::TRACKNODEITEMS(const TRACKITEMS* aParent, const BOARD* aBoard)
{
    m_Board = const_cast<BOARD*>(aBoard);
    m_Parent = const_cast<TRACKITEMS*>(aParent);
    m_EditFrame = nullptr;
    m_menu = nullptr;
    
    m_get_list = new TrackNodeItem_Container;
    m_get_list->clear(); 
}

TRACKNODEITEMS::~TRACKNODEITEMS()
{
    delete m_get_list;
    m_get_list = nullptr;
}

void TRACKNODEITEMS::SetEditFrame(const PCB_EDIT_FRAME* aEditFrame)
{
    m_EditFrame = const_cast<PCB_EDIT_FRAME*>(aEditFrame);
    if(m_menu)
    {
        delete m_menu;
        m_menu = nullptr;
    }
    m_menu = new wxMenu;
    RecreateMenu();
}

//----------------------------------------------------------------------------------------
// DRC
//----------------------------------------------------------------------------------------
STATUS_FLAGS TRACKNODEITEMS::DRC_Flags(const STATUS_FLAGS aStatus)
{
    return aStatus & (IS_CHANGED | IN_EDIT | IS_MOVED | IS_RESIZED | IS_DRAGGED | IS_NEW);
}

//------------------------------------------------------------------------------------


//----------------------------------------------------------------------------------------
// PLUGIN PARSE 
//----------------------------------------------------------------------------------------

TRACK* TRACKNODEITEMS::GetTrackSegment(const wxPoint aStart, const wxPoint aEnd, const int aLayer, const int aNetCode) const
{
    TRACK* track_seg = m_Board->m_Track; //GetTrack();
    while(track_seg)
    {
        if(track_seg->GetNetCode() == aNetCode)
            if(track_seg->GetLayer() == aLayer)
                if((track_seg->GetStart() == aStart) && (track_seg->GetEnd() == aEnd))
                    return track_seg;
        track_seg = track_seg->Next();
    }
    return nullptr;
}

//------------------------------------------------------------------------------------

//------------------------------------------------------------------------------------
// List add.
//------------------------------------------------------------------------------------
TRACKNODEITEM* TRACKNODEITEMS::Get(const TRACK* aTrackSegAt, const wxPoint& aPosAt) const
{
    TRACKNODEITEM* result = nullptr;
    if(aTrackSegAt && aTrackSegAt->Type() == PCB_TRACE_T)
    {
        const_cast<TRACKNODEITEMS*>(this)->AddGetList(aTrackSegAt);
        if(m_get_list)
        {
            uint min_dist = std::numeric_limits<uint>::max();
            for(TRACKNODEITEM* item : *m_get_list)
            {
                wxPoint center_pos = item->GetPosition(); //Center pos
                uint dist_center = hypot(abs(center_pos.y - aPosAt.y) , abs(center_pos.x - aPosAt.x));
                wxPoint pos = item->GetEnd();
                uint dist_pos = hypot(abs(pos.y - aPosAt.y) , abs(pos.x - aPosAt.x));
                if((dist_pos < min_dist) && ((dist_center < item->GetBoundingRad()) || (aPosAt == pos)))
                {
                    min_dist = dist_pos;
                    result = item;
                }
            }
        }
    }
    return result;
}

TRACKNODEITEM* TRACKNODEITEMS::Get(const TRACK* aTrackSegAt, const wxPoint& aPosAt, const bool aExcactPos) const
{
    if(aTrackSegAt && aTrackSegAt->Type() == PCB_TRACE_T)
    {
        if(aPosAt == aTrackSegAt->GetStart())
            return Back(aTrackSegAt);
        if(aPosAt == aTrackSegAt->GetEnd())
            return Next(aTrackSegAt);
    }
    return nullptr;
}

void TRACKNODEITEMS::AddGetList(const TRACK* aTrackSegFrom)
{
    if(aTrackSegFrom)
    {
        m_get_list->clear();
        if(aTrackSegFrom->Type() == PCB_TRACE_T)
        {
            TRACKNODEITEM* item = Next(aTrackSegFrom);
            if(item)
                m_get_list->insert(item); 

            item = Back(aTrackSegFrom);
            if(item)
                m_get_list->insert(item); 
        }
        else
            if(dynamic_cast<TRACKNODEITEM*>(const_cast<TRACK*>(aTrackSegFrom)))
                m_get_list->insert(static_cast<TRACKNODEITEM*>(const_cast<TRACK*>(aTrackSegFrom))); 
    }
}
//------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------
// Menus 
//----------------------------------------------------------------------------------------

void TRACKNODEITEMS::RecreateMenu(void)
{
    if(m_menu)
    {
        int item_pos = m_menu->GetMenuItemCount();
        while(--item_pos >= 0)
        {
            wxMenuItem* item = m_menu->FindItemByPosition(item_pos);
            m_menu->Destroy(item);
        }
        CreateMenu(m_menu);
    }
}
