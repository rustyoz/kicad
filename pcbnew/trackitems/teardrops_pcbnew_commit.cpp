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
#include <tool/tool_manager.h>
#include "router/router_tool.h"
#include "router/pns_router.h"

using namespace TrackNodeItem;

//Gal canvas add when drag and route tracks.
void TEARDROPS::GalCommitPushAdd(BOARD_ITEM* aItem, PICKED_ITEMS_LIST* aUndoRedoList)
{
    if(aItem)
    {
        if(aItem->Type() == PCB_TRACE_T)
        {
            m_gal_commit_tracks.push_back(static_cast<TRACK*>(aItem));

            //Routing.
            TOOL_BASE* tool = m_EditFrame->GetToolManager()->FindTool( "pcbnew.InteractiveRouter" );
            ROUTER_TOOL* router = dynamic_cast<ROUTER_TOOL*>(tool);
            if(router && (!router->Router()->IsDragging())) 
            {
                if(dynamic_cast<BOARD_CONNECTED_ITEM*>(aItem)->GetNetCode() == m_current_routed_track_netcode)
                    Add(static_cast<TRACK*>(aItem), aUndoRedoList);
            }

            //Dragging and/or pushing when routing.
            if(m_gal_removed_list->size())
            {
                for(TEARDROP* tear : *m_gal_removed_list)
                {
                    BOARD_CONNECTED_ITEM* citem = tear->GetConnectedItem();
                    if(citem)
                    {
                        BOARD_CONNECTED_ITEM* track_item = nullptr;
                        for(int n = 0; n < 2 ; ++n)
                        {
                            wxPoint citem_pos;
                            if(citem->Type() == PCB_PAD_T)
                                citem_pos = citem->GetPosition();
                            else
                                if(dynamic_cast<TRACK*>(citem))
                                    citem_pos = dynamic_cast<TRACK*>(citem)->GetEnd();
                            if((citem_pos == static_cast<TRACK*>(aItem)->GetStart()) || (citem_pos == static_cast<TRACK*>(aItem)->GetEnd() ))
                            {
                                if(citem->Type() == PCB_PAD_T)
                                    n?  track_item = m_Board->GetPadFast(static_cast<TRACK*>(aItem)->GetEnd(), 
                                                                        LSET(aItem->GetLayer())) :
                                        track_item = m_Board->GetPadFast(static_cast<TRACK*>(aItem)->GetStart(), 
                                                                        LSET(aItem->GetLayer()));
                                else
                                    if(dynamic_cast<VIA*>(citem))
                                        n?  track_item = m_Board->GetViaByPosition(static_cast<TRACK*>(aItem)->GetEnd(),
                                                                                aItem->GetLayer()) : 
                                            track_item = m_Board->GetViaByPosition(static_cast<TRACK*>(aItem)->GetStart(),
                                                                                aItem->GetLayer());

                                if(citem == track_item)
                                {
                                    TEARDROP::PARAMS params = tear->GetParams();
                                    if(params != GetShapeParams(GetCurrentShape()))
                                    {
                                        SetShapeParams(params);
                                        SetCurrentShape(params.shape);
                                    }
                                    Add(static_cast<TRACK*>(aItem), track_item, aUndoRedoList);
                                    m_gal_drag_tears_used.insert(tear);
                                    break;
                                }
                            }
                        }
                    }
                }

                SetShapeParams(m_teardrop_params_gal);
                SetShapeParams(m_fillet_params_gal);
                SetShapeParams(m_subland_params_gal);
                SetCurrentShape(m_current_shape_gal);
            }
        }

        //Drag via.
        if(aItem->Type() == PCB_VIA_T)
            m_gal_drag_vias_added.insert(static_cast<VIA*>(aItem));
    }
}

//Gal canva remove when drag and route tracks.
void TEARDROPS::GalCommitPushRemove(BOARD_ITEM* aItemFrom, PICKED_ITEMS_LIST* aUndoRedoList)
{
    if( dynamic_cast<BOARD_CONNECTED_ITEM*>(aItemFrom) )
        Remove( static_cast<BOARD_CONNECTED_ITEM*>(aItemFrom), aUndoRedoList, true );
    else
        if( dynamic_cast<MODULE*>(aItemFrom) )
            Remove( static_cast<MODULE*>(aItemFrom), aUndoRedoList, true );
}

void TEARDROPS::GalCommitPushPrepare(void)
{
    m_gal_removed_list->clear();
    m_current_shape_gal = GetCurrentShape();
    m_teardrop_params_gal = GetShapeParams(TEARDROP::TEARDROP_T);
    m_fillet_params_gal = GetShapeParams(TEARDROP::FILLET_T);
    m_subland_params_gal = GetShapeParams(TEARDROP::SUBLAND_T);

    TOOL_BASE* tool = m_EditFrame->GetToolManager()->FindTool( "pcbnew.InteractiveRouter" );
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
}

void TEARDROPS::GalCommitPushFinish(PICKED_ITEMS_LIST* aUndoRedoList)
{
    //Add teardrops moved/added vias.
    for(TEARDROP* tear : *m_gal_removed_list)
    {
        std::set<TEARDROP*>::iterator tear_it = m_gal_drag_tears_used.find(tear);
        if(tear_it == m_gal_drag_tears_used.end())
        {
            for(VIA* via : m_gal_drag_vias_added)
            {
                if(tear->GetNetCode() == via->GetNetCode())
                {
                    for(TRACK* track : m_gal_commit_tracks)
                    {
                        if((track->GetNetCode() == via->GetNetCode()) && (track->GetLayer() == tear->GetLayer()) && via->IsOnLayer(track->GetLayer()))
                            if((track->GetStart() == via->GetEnd()) || (track->GetEnd() == via->GetEnd()))
                            {
                                if(!GetTeardrop(track, via))
                                {
                                    TEARDROP::PARAMS params = tear->GetParams();
                                    if(params != GetShapeParams(GetCurrentShape()))
                                    {
                                        SetShapeParams(params);
                                        SetCurrentShape(params.shape);
                                    }
                                    Add(track, via, aUndoRedoList);
                                    break;
                                }
                            }
                    }
                }
            }
        }
    }

    m_gal_removed_list->clear();
    m_gal_drag_vias_added.clear();
    m_gal_drag_tears_used.clear();
    m_gal_commit_tracks.clear();

    SetShapeParams(m_teardrop_params_gal);
    SetShapeParams(m_fillet_params_gal);
    SetShapeParams(m_subland_params_gal);
    SetCurrentShape(m_current_shape_gal);
    RecreateMenu();
}
