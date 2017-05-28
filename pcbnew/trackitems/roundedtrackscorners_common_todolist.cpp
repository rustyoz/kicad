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
#include <view/view.h>

using namespace TrackNodeItem;


void ROUNDEDTRACKSCORNERS::UpdateListClear(void)
{
    m_update_list->clear();
    m_update_tracks_list->clear();
}

void ROUNDEDTRACKSCORNERS::UpdateListAdd(const ROUNDEDTRACKSCORNER* aCorner)
{
    if(aCorner)
    {
        m_update_list->insert(const_cast<ROUNDEDTRACKSCORNER*>(aCorner));
        
        //Must collect connected tracks corners too.
        TRACK* track = aCorner->GetTrackSeg();
        m_update_list->insert(dynamic_cast<ROUNDEDTRACKSCORNER*>(Get(track, track->GetStart())));
        m_update_list->insert(dynamic_cast<ROUNDEDTRACKSCORNER*>(Get(track, track->GetEnd())));
        //aUpdatedTracks.insert(static_cast<ROUNDEDCORNERTRACK*>(track));
        m_update_tracks_list->insert(static_cast<ROUNDEDCORNERTRACK*>(track));
        
        track = aCorner->GetTrackSegSecond();
        m_update_list->insert(dynamic_cast<ROUNDEDTRACKSCORNER*>(Get(track, track->GetStart())));
        m_update_list->insert(dynamic_cast<ROUNDEDTRACKSCORNER*>(Get(track, track->GetEnd())));
        //aUpdatedTracks.insert(static_cast<ROUNDEDCORNERTRACK*>(track));
        m_update_tracks_list->insert(static_cast<ROUNDEDCORNERTRACK*>(track));
    }
}

void ROUNDEDTRACKSCORNERS::UpdateListAdd(const TRACK* aTrackSegFrom)
{
    if(aTrackSegFrom)
    {

        if(aTrackSegFrom->Type() == PCB_ROUNDEDTRACKSCORNER_T)
            UpdateListAdd(static_cast<ROUNDEDTRACKSCORNER*>(const_cast<TRACK*>(aTrackSegFrom))); 
        
        if(dynamic_cast<ROUNDEDCORNERTRACK*>(const_cast<TRACK*>(aTrackSegFrom)))
        {
            m_update_tracks_list->insert(static_cast<ROUNDEDCORNERTRACK*>(const_cast<TRACK*>(aTrackSegFrom)));
            
            ROUNDEDTRACKSCORNER* corner = dynamic_cast<ROUNDEDTRACKSCORNER*>(Next(aTrackSegFrom));
            if(corner)
                UpdateListAdd(corner); 

            corner = dynamic_cast<ROUNDEDTRACKSCORNER*>(Back(aTrackSegFrom));
            if(corner)
                UpdateListAdd(corner); 
        }
    }
}

void ROUNDEDTRACKSCORNERS::UpdateList_DrawTracks(EDA_DRAW_PANEL* aPanel, wxDC* aDC, GR_DRAWMODE aDrawMode)
{
    if(m_update_tracks_list)
        for(auto r_t: *m_update_tracks_list )
            r_t->Draw( aPanel, aDC, aDrawMode );
}

void ROUNDEDTRACKSCORNERS::UpdateListDo(void)
{
    if(m_update_tracks_list)
        for(auto r_t: *m_update_tracks_list )
            r_t->ResetVisibleEndpoints();
        
    if(m_update_list)
    {
        for(ROUNDEDTRACKSCORNER* corner : *m_update_list)
            if(corner)
                corner->ResetVisibleEndpoints();
        for(ROUNDEDTRACKSCORNER* corner : *m_update_list)
            if(corner)
                corner->Update();
    }
}

void ROUNDEDTRACKSCORNERS::UpdateListDo(EDA_DRAW_PANEL* aPanel, wxDC* aDC, GR_DRAWMODE aDrawMode, bool aErase)
{
    if(m_update_tracks_list)
        for(auto r_t: *m_update_tracks_list )
            r_t->ResetVisibleEndpoints();
    
    if(m_update_list)
    {
        for(ROUNDEDTRACKSCORNER* corner : *m_update_list)
            if(corner)
                if(aErase)
                    corner->Draw(aPanel, aDC, aDrawMode);
        for(ROUNDEDTRACKSCORNER* corner : *m_update_list)
            if(corner)
                corner->ResetVisibleEndpoints();
        for(ROUNDEDTRACKSCORNER* corner : *m_update_list)
            if(corner)
                corner->Update();
        for(ROUNDEDTRACKSCORNER* corner : *m_update_list)
            if(corner)
                corner->Draw(aPanel, aDC, aDrawMode);
    }
}

void ROUNDEDTRACKSCORNERS::UpdateListDo_UndoRedo(void)
{
    if(m_update_tracks_list)
        for(auto r_t: *m_update_tracks_list )
            r_t->ResetVisibleEndpoints();

    if(m_update_list)
    {
        for(ROUNDEDTRACKSCORNER* corner : *m_update_list)
            if(corner)
                corner->ResetVisibleEndpoints();
        for(ROUNDEDTRACKSCORNER* corner : *m_update_list)
        {
            if(corner)
            {
                corner->SetTrackEndpoint();
                corner->SetTrackSecondEndpoint();
                corner->Update();
            }
        }
    }
    
    //GAL
    if(m_EditFrame)
        if( m_EditFrame->IsGalCanvasActive() )
            if(m_update_tracks_list)
                for(auto r_t: *m_update_tracks_list )
                    m_EditFrame->GetGalCanvas()->GetView()->Update(r_t, KIGFX::GEOMETRY);
}

void ROUNDEDTRACKSCORNERS::UpdateListDo_BlockDuplicate(const wxPoint aMoveVector, PICKED_ITEMS_LIST* aUndoRedoList)
{
    if(m_update_list)
    {
        ROUNDEDTRACKSCORNER::PARAMS current_params = GetParams();
        for(ROUNDEDTRACKSCORNER* corner : *m_update_list)
        {
            ROUNDEDTRACKSCORNER::PARAMS params {0, 0, 0};
            if(corner)
            {
                ROUNDEDTRACKSCORNER::PARAMS corner_params = corner->GetParams();
                if(params != corner_params)
                {
                    params = corner_params;
                    SetParams(params);
                }
                
                wxPoint corner_pos = corner->GetEnd() + aMoveVector;
                wxPoint track_start = corner->GetTrackSeg()->GetStart() + aMoveVector;
                wxPoint track_end = corner->GetTrackSeg()->GetEnd() + aMoveVector;
                TRACK* new_track_seg = GetTrackSegment(track_start, track_end, corner->GetLayer(), corner->GetNetCode());
                if(new_track_seg)
                    Add(new_track_seg, corner_pos, aUndoRedoList);
            }
        }
        SetParams(current_params);
        RecreateMenu();
    }
}
