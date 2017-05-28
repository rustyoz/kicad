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

using namespace TrackNodeItem;


void ROUNDEDTRACKSCORNERS::ToggleEdit(const TO_EDIT_T aEdit)
{
    if(m_track_edit_corner)  
    {
        if(m_can_edit && aEdit)
        {
            if(m_to_edit)
            {
                switch(m_to_edit)
                {
                    case EDIT_LENGTH_SET_T:
                    case EDIT_LENGTH_RATIO_T:
                        SetParams(m_edit_params);
                        RecreateMenu();
                    default:
                        m_to_edit = EDIT_NULL_T;
                        *m_track45Only = m_track45Only_before_edit;
                        break;
                }
            }
            else
            {
                m_to_edit = aEdit;
                m_track_edit_corner->SetParams(m_edit_params);
            }
        }
        else
        {
            *m_track45Only = m_track45Only_before_edit;
            m_to_edit = EDIT_NULL_T;
        }
        if(m_EditFrame)
            m_EditFrame->GetCanvas()->Refresh();
    }
}

void ROUNDEDTRACKSCORNERS::RouteCreate_Start(void)
{
    m_can_edit = true;
}

void ROUNDEDTRACKSCORNERS::RouteCreate_Stop(void)
{
    m_can_edit = false;
    ToggleEdit(EDIT_NULL_T);
    if(m_track_edit_corner)
    {
        m_track_edit_corner->ResetVisibleEndpoints();
        m_track_edit_corner->ReleaseTrackSegs();
        delete m_track_edit_corner;
        m_track_edit_corner = nullptr;
    }
}

void ROUNDEDTRACKSCORNERS::DrawEditParams(EDA_DRAW_PANEL* aPanel, wxDC* aDC, const wxPoint& aOffset) const
{
    if(m_track_edit_corner) 
    {
        TRACK* segm = m_track_edit_corner->GetTrackSeg();
        if(segm)
        {
            wxPoint txtpos = m_track_edit_corner->GetEnd();
            int txt_size = m_track_edit_corner->GetWidth();
            txtpos.x += txt_size * 3;
            wxPoint txt_t_pos  = txtpos;
            txt_t_pos.y -= txt_size / 2;
            wxPoint txt_n_pos  = txt_t_pos;
            txt_n_pos.y += txt_size;

            wxString txt_t, txt_n; 
            txt_n.Printf( _( "%s" ), m_EditFrame->LengthDoubleToString(m_edit_params.length_set, true));
            txt_t.Printf( _( "%d%s" ), m_edit_params.length_ratio, _("%"));
        
            EDA_RECT* eRect = aPanel->GetClipBox();
            DrawGraphicText( eRect, aDC, txt_t_pos, WHITE, txt_t, 0, wxSize( txt_size, txt_size ), GR_TEXT_HJUSTIFY_CENTER, GR_TEXT_VJUSTIFY_BOTTOM, txt_size / 7, false, false );
            DrawGraphicText( eRect, aDC, txt_n_pos, WHITE, txt_n, 0, wxSize( txt_size, txt_size ), GR_TEXT_HJUSTIFY_CENTER, GR_TEXT_VJUSTIFY_TOP, txt_size / 7, false, false );
        }
    }
}

void ROUNDEDTRACKSCORNERS::DestroyRouteEdit(void)
{
    if(m_track_edit_corner)
    {
        m_track_edit_corner->ResetVisibleEndpoints();
        m_track_edit_corner->ReleaseTrackSegs();
        delete m_track_edit_corner;
        m_track_edit_corner = nullptr;
    }
}

ROUNDEDTRACKSCORNER* ROUNDEDTRACKSCORNERS::UpdateRouteEdit(EDA_DRAW_PANEL* aPanel, wxDC* aDC, const TRACK* aTrack, const TRACK* aTrackSecond, const bool aErase, bool* aTrack45Only)
{
    if(!aTrack || !aTrackSecond|| GetParent()->Teardrops()->IsEditOn() || const_cast<TRACK*>(aTrack)->IsNull() ||const_cast<TRACK*>(aTrackSecond)->IsNull())
    {
        if(m_track_edit_corner)
            m_track_edit_corner->Draw(aPanel, aDC, GR_XOR, wxPoint(0,0));

        DestroyRouteEdit();
        return nullptr;
    }
    
    if(m_can_edit)
    {
        bool erase = aErase;
        if(!m_track_edit_corner)
        {
            ROUNDEDTRACKSCORNER::PARAMS params = GetParams();
            m_track_edit_corner = new ROUNDEDTRACKSCORNER_ROUTE_EDIT(dynamic_cast<PCB_BASE_FRAME*>(aPanel->GetParent())->GetBoard(), aTrack, aTrackSecond, params,  true);
            m_track45Only = aTrack45Only;
            if(m_track_edit_corner)
            {
                if(!m_track_edit_corner->IsCreatedOK())   
                {
                    delete m_track_edit_corner;
                    m_track_edit_corner = nullptr;
                }
            }
            erase = false;
        }
        if(m_track_edit_corner)
        {
            if(erase)
                m_track_edit_corner->Draw(aPanel, aDC, GR_XOR, wxPoint(0,0));
            if(dynamic_cast<ROUNDEDTRACKSCORNER_ROUTE_EDIT*>(m_track_edit_corner)->ChangeTracks(aTrack, aTrackSecond))
            {
                if(IsEditOn())
                {
                    *m_track45Only = true;
                    int dist = std::min(m_track_edit_corner->GetTrackSeg()->GetLength() / 2, m_track_edit_corner->GetTrackSegSecond()->GetLength() / 2);
                    if(dist > 0)
                    {
                        if(aErase && m_editparams_drawn)
                            DrawEditParams(aPanel, aDC, wxPoint(0,0));
                        switch(m_to_edit)
                        {
                            case EDIT_LENGTH_SET_T:
                            {
                                m_edit_params.length_set = double(dist);
                                break;
                            }
                            case EDIT_LENGTH_RATIO_T:
                            {
                                m_edit_params.length_ratio = double(dist) / 25000.0; 
                                break;
                            }
                            default:;
                        }
                        m_track_edit_corner->SetParams(m_edit_params);
                        DrawEditParams(aPanel, aDC, wxPoint(0,0));
                        m_editparams_drawn = true;
                    }
                    else
                    {
                        if(m_editparams_drawn)
                            DrawEditParams(aPanel, aDC, wxPoint(0,0));
                        m_editparams_drawn = false;
                    }
                }
                else
                {
                    m_track45Only_before_edit = *m_track45Only;
                    m_edit_start_point = aTrack->GetStart();
                    m_edit_params = GetParams();
                    m_track_edit_corner->SetParams(m_edit_params);
                    m_editparams_drawn = false;
                }
                m_track_edit_corner->Draw(aPanel, aDC, GR_XOR, wxPoint(0,0));
            }
            else
            {
                delete m_track_edit_corner;
                m_track_edit_corner = nullptr;
            }
        }
    }
    return m_track_edit_corner;
}

