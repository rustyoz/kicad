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


//Gal canvas add when drag and route tracks.
void ROUNDEDTRACKSCORNERS::GalCommitPushAdd(BOARD_ITEM* aItem, PICKED_ITEMS_LIST* aUndoRedoList)
{
    if(aItem)
    {
        if(aItem->Type() == PCB_TRACE_T)
        {
            m_gal_commit_tracks.insert(static_cast<TRACK*>(aItem));

            /*
            //Routing.
            TOOL_BASE* tool = m_edit_frame->GetToolManager()->FindTool( "pcbnew.InteractiveRouter" );
            ROUTER_TOOL* router = dynamic_cast<ROUTER_TOOL*>(tool);
            if(router && (!router->Router()->IsDragging())) 
            {
                if(dynamic_cast<BOARD_CONNECTED_ITEM*>(aItem)->GetNetCode() == m_current_routed_track_netcode)
                    Add(static_cast<TRACK*>(aItem), aUndoRedoList);
            }
            */

            //Dragging and/or pushing when routing.
        }
    }
}

//Gal canva remove when drag and route tracks.
void ROUNDEDTRACKSCORNERS::GalCommitPushRemove(BOARD_ITEM* aItemFrom, PICKED_ITEMS_LIST* aUndoRedoList)
{
    if( dynamic_cast<ROUNDEDCORNERTRACK*>(aItemFrom) )
        Remove( static_cast<ROUNDEDCORNERTRACK*>(aItemFrom), aUndoRedoList, true );
}

void ROUNDEDTRACKSCORNERS::GalCommitPushPrepare(void)
{
    m_gal_removed_list->clear();
    m_gal_commit_tracks.clear();

    /*
    TOOL_BASE* tool = m_edit_frame->GetToolManager()->FindTool( "pcbnew.InteractiveRouter" );
    ROUTER_TOOL* router = dynamic_cast<ROUTER_TOOL*>(tool);
    if(router) 
    {
        PNS::ITEM* start_item = router->GetStartItem();
        if(start_item)
        {
            m_current_routed_track_netcode = start_item->Net();
            m_gal_drag_via_dragged = nullptr;
            if((start_item->Kind() == PNS::ITEM::VIA_T) && router->Router()->IsDragging())
                m_gal_drag_via_dragged = static_cast<VIA*>(start_item->Parent());
            m_gal_drag_vias_added.clear();
            m_gal_drag_tears_used.clear();
            m_gal_commit_tracks.clear();
        }
    }
    */
}

void ROUNDEDTRACKSCORNERS::GalCommitPushFinish(PICKED_ITEMS_LIST* aUndoRedoList)
{
        /*
    if(m_gal_removed_list->size())
    {
        for(ROUNDEDTRACKSCORNER* corner : *m_gal_removed_list)
        {
            for(TRACK* track : m_gal_commit_tracks)
            {
                wxPoint pos = corner->GetEnd();
                if((static_cast<TRACK*>(track)->GetStart() == pos) || (static_cast<TRACK*>(track)->GetEnd() == pos))
                {
                    if(static_cast<TRACK*>(track)->IsOnLayer(corner->GetLayer()))
                    {
                        ROUNDEDTRACKSCORNER* added_corner = Add(static_cast<TRACK*>(aItem), pos, aUndoRedoList);
                        m_gal_commit_tracks.insert(static_cast<TRACK*>(aItem));
                        m_gal_commit_tracks.insert(static_cast<TRACK*>(aItem));
                    }
                }
            }
        }
    }
        */

    //int netcode = 0;
    for(TRACK* track : m_gal_commit_tracks)
    {
        //if(netcode != track->GetNetCode())
        //{
        //    netcode = track->GetNetCode();
        //    Add(netcode, aUndoRedoList);
        //}
        Add(track, aUndoRedoList);
    }

    m_gal_removed_list->clear();
    m_gal_commit_tracks.clear();
}

