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


void TEARDROPS::UpdateListClear(void)
{
    m_update_list->clear();
}

void TEARDROPS::UpdateListAdd(const TRACK* aTrackSegFrom)
{
    AddToDoList(aTrackSegFrom, m_update_list);
}

void TEARDROPS::UpdateListAdd(const TEARDROP* aTear)
{
    if(aTear)
        if(aTear->Type() == PCB_TEARDROP_T)
            m_update_list->insert(const_cast<TEARDROP*>(aTear)); 
}

void TEARDROPS::UpdateListAdd(const ROUNDEDTRACKSCORNERS::RoundedCornerTrack_Container* aRoundedTracks)
{
   for(auto track : *aRoundedTracks)
        UpdateListAdd( track );
}

void TEARDROPS::UpdateListDo(void)
{
    if(m_update_list)
        for(TEARDROP* tear : *m_update_list)
            tear->Update();
}

void TEARDROPS::UpdateListDo(EDA_DRAW_PANEL* aPanel, wxDC* aDC, GR_DRAWMODE aDrawMode, bool aErase)
{
    if(m_update_list)
    {
        for(TEARDROP* tear : *m_update_list)
        {
            if(aErase)
                tear->Draw(aPanel, aDC, aDrawMode);
            tear->Update();
            tear->Draw(aPanel, aDC, aDrawMode);
        }
    }
}

void TEARDROPS::UpdateListDo_UndoRedo(void)
{
    if(m_update_list)
        for(TEARDROP* tear : *m_update_list)
        {
            tear->SetTrackEndpoint();
            tear->Update();
        }
}

void TEARDROPS::UpdateListDo_BlockDuplicate(const wxPoint aMoveVector, PICKED_ITEMS_LIST* aUndoRedoList)
{
    if(m_update_list)
    {
        TEARDROP::SHAPES_T current_shape = GetCurrentShape();
        TEARDROP::PARAMS teardrop_params = GetShapeParams(TEARDROP::TEARDROP_T);
        TEARDROP::PARAMS fillet_params = GetShapeParams(TEARDROP::FILLET_T);
        TEARDROP::PARAMS subland_params = GetShapeParams(TEARDROP::SUBLAND_T);
        for(TEARDROP* tear : *m_update_list)
        {
            TEARDROP::PARAMS params {TEARDROP::NULL_T, 1, 1, 1};
            if(tear)
            {
                TEARDROP::PARAMS tear_params = tear->GetParams();
                if(params != tear_params)
                {
                    params = tear_params;
                    SetShapeParams(params);
                    SetCurrentShape(params.shape);
                }
                wxPoint track_start = tear->GetTrackSeg()->GetStart() + aMoveVector;
                wxPoint track_end = tear->GetTrackSeg()->GetEnd() + aMoveVector;
                TRACK* new_track_seg = GetTrackSegment(track_start, track_end, tear->GetLayer(), tear->GetNetCode());
                if(new_track_seg)
                {
                    BOARD_CONNECTED_ITEM* tear_item = tear->GetConnectedItem();
                    if(tear_item)
                    {
                        if(tear_item->Type() == PCB_VIA_T)
                        {
                            TEARDROP_VIA* tear_via = static_cast<TEARDROP_VIA*>(tear);
                            if(tear_via)
                            {
                                VIA* via = static_cast<VIA*>(m_Board->GetViaByPosition(tear_via->GetEnd(), tear_via->GetLayer()));
                                if(via)
                                    Add(new_track_seg, static_cast<BOARD_CONNECTED_ITEM*>(via), aUndoRedoList);
                            }
                        }
                        if(tear_item->Type() == PCB_PAD_T)
                        {
                            TEARDROP_PAD* tear_pad = static_cast<TEARDROP_PAD*>(tear);
                            if(tear_pad)
                            {
                                D_PAD* pad = m_Board->GetPad(tear_pad->GetEnd(), tear_pad->GetLayer());
                                if(pad)
                                    Add(new_track_seg, static_cast<BOARD_CONNECTED_ITEM*>(pad), aUndoRedoList);
                            }
                        }
                    }
                    else //Junctions and T-Junctions.
                        Add(new_track_seg, tear_item, aUndoRedoList, tear->GetEnd());
                }
                Remove(static_cast<BOARD_CONNECTED_ITEM*>(tear), aUndoRedoList, true);
            }
        }
        SetShapeParams(teardrop_params);
        SetShapeParams(fillet_params);
        SetShapeParams(subland_params);
        SetCurrentShape(current_shape);
        RecreateMenu();
    }
}

void TEARDROPS::AddToDoList(const TRACK* aTrackSegFrom, Teardrop_Container* aListToAdd)
{
    if(aTrackSegFrom)
    {

        if(aTrackSegFrom->Type() == PCB_TEARDROP_T)
            aListToAdd->insert(static_cast<TEARDROP*>(const_cast<TRACK*>(aTrackSegFrom))); 
        
        TEARDROP* tear = dynamic_cast<TEARDROP*>(Next(aTrackSegFrom));
        if(tear)
            aListToAdd->insert(tear); 

        tear = dynamic_cast<TEARDROP*>(Back(aTrackSegFrom));
        if(tear)
            aListToAdd->insert(tear); 
    }
}
