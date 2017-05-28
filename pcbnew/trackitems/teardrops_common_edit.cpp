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


void TEARDROPS::ToggleEdit(const TO_EDIT_T aEdit)
{
    if(m_track_edit_tear)  
    {
        if(m_can_edit && aEdit)
        {
            if(m_to_edit)
            {
                switch(m_to_edit)
                {
                    case EDIT_SIZE_LENGTH_T:
                    case EDIT_SIZE_WIDTH_T:
                        m_edit_params = SetParamLimits(m_edit_params);
                        m_current_shape = m_edit_params.shape;
                        SetShapeParams(m_edit_params);
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
                m_track_edit_tear->SetParams(m_edit_params);
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

void TEARDROPS::RouteCreate_Start(void)
{
    m_can_edit = true;
}

void TEARDROPS::RouteCreate_Stop(void)
{
    m_can_edit = false;
    ToggleEdit(EDIT_NULL_T);
    if(m_track_edit_tear)
    {
        delete m_track_edit_tear;
        m_track_edit_tear = nullptr;
    }
}

void TEARDROPS::DrawEditParams(EDA_DRAW_PANEL* aPanel, wxDC* aDC, const wxPoint& aOffset) const
{
    if(m_track_edit_tear) 
    {
        //Size texts.
        TRACK* segm = m_track_edit_tear->GetTrackSeg();
        if(segm)
        {
            wxPoint txtpos = m_track_edit_tear->GetEnd();
            int txt_size = m_track_edit_tear->GetViaRad() / 6;
            txtpos.x += txt_size * 3;
            wxPoint txt_t_pos  = txtpos;
            txt_t_pos.y -= txt_size / 2;
            wxPoint txt_n_pos  = txt_t_pos;
            txt_n_pos.y += txt_size;

            wxString txt_t, txt_n; 
            switch(m_to_edit)
            {
                case EDIT_SIZE_LENGTH_T:
                    txt_t.Printf( _( "X" ));
                    if(m_edit_params.shape == TEARDROP::SUBLAND_T)
                    {
                        if(m_edit_params.length_ratio > TEARDROP::SUBLAND_POS_MAX)
                        {
                            txt_n.Printf( _( "<--" ));
                            break;
                        }
                    }
                    else
                        if(m_edit_params.length_ratio < TEARDROP::TEARDROPS_POS_MIN)
                        {
                            txt_n.Printf( _( "-->" ));
                            break;
                        }
                    txt_n.Printf( _( "%d" ), m_edit_params.length_ratio);
                    break;
                case EDIT_SIZE_WIDTH_T:
                    txt_t.Printf( _( "Y" ));
                    if(m_edit_params.width_ratio > TEARDROP::TEARDROPS_WIDTH_MAX)
                    {
                        txt_n.Printf( _( "<--" ));
                        break;
                    }
                    if(m_edit_params.width_ratio < TEARDROP::TEARDROPS_SIZE_MIN)
                    {
                        txt_n.Printf( _( "-->" ));
                        break;
                    }
                    txt_n.Printf( _( "%d" ), m_edit_params.width_ratio);
                    break;
                default:
                    txt_t.Printf( _( "?" ));
                    txt_n.Printf( _( "?" ));
            }
        
            EDA_RECT* eRect = aPanel->GetClipBox();
            DrawGraphicText( eRect, aDC, txt_t_pos, WHITE, txt_t, 0, wxSize( txt_size, txt_size ), GR_TEXT_HJUSTIFY_CENTER, GR_TEXT_VJUSTIFY_BOTTOM, txt_size / 7, false, false );
            DrawGraphicText( eRect, aDC, txt_n_pos, WHITE, txt_n, 0, wxSize( txt_size, txt_size ), GR_TEXT_HJUSTIFY_CENTER, GR_TEXT_VJUSTIFY_TOP, txt_size / 7, false, false );
        }
    }
}

void TEARDROPS::UpdateRouteEdit(EDA_DRAW_PANEL* aPanel, wxDC* aDC, const TRACK* aTrack, const uint aViaRad,  const bool aErase, bool* aTrack45Only)
{
    if(m_current_shape && m_can_edit)
    {
        if(!m_track_edit_tear)
        {
            TEARDROP::PARAMS params = GetShapeParams(m_current_shape);
            m_track_edit_tear = new TEARDROP_EDIT_VIA(dynamic_cast<PCB_BASE_FRAME*>(aPanel->GetParent())->GetBoard(), aViaRad, aTrack, params,  true);
            m_track45Only = aTrack45Only;
            if(m_track_edit_tear)
            {
                if(m_track_edit_tear->IsCreatedOK())   
                {
                    m_track_edit_tear->SetDrawMode_Unfill(true);
                }
                else
                {
                    delete m_track_edit_tear;
                    m_track_edit_tear = nullptr;
                }
            }
        }
        if(m_track_edit_tear)
        {
            if(aErase)
                m_track_edit_tear->Draw(aPanel, aDC, GR_XOR, wxPoint(0,0));
            dynamic_cast<TEARDROP_EDIT_VIA*>(m_track_edit_tear)->ChangeTrack(aTrack);
            if(IsEditOn())
            {
                *m_track45Only = false;
                long int dist = aTrack->GetEnd().x - m_edit_start_point.x - aViaRad;
                if(dist > 0)
                {
                    if(aErase && m_editparams_drawn)
                        DrawEditParams(aPanel, aDC, wxPoint(0,0));
                    switch(m_to_edit)
                    {
                        case EDIT_SIZE_WIDTH_T:
                        {
                            int mltplr = 50;
                            if(m_edit_params.shape == TEARDROP::SUBLAND_T)
                                mltplr = 20;
                            int sz_x = dist - m_track_edit_tear->GetSizeLength();
                            if(sz_x > 0)
                            {
                                m_edit_params.width_ratio = double(sz_x) / double(double(aViaRad)) * mltplr;
                            }
                            else
                                m_edit_params.width_ratio = 1;
                            break;
                        }
                        case EDIT_SIZE_LENGTH_T:
                        {
                            ulong mltplr = 100;
                            if(m_edit_params.shape == TEARDROP::SUBLAND_T)
                                mltplr = 50;
                            m_edit_params.length_ratio = mltplr * dist / aViaRad;
                            break;
                        }
                        default:;
                    }
                    m_edit_params = SetParamLimits(m_edit_params);
                    m_track_edit_tear->SetParams(m_edit_params);
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
                m_edit_params = GetShapeParams(m_current_shape);
                m_track_edit_tear->SetParams(m_edit_params);
                m_editparams_drawn = false;
            }
            m_track_edit_tear->SetStart(aTrack->GetEnd());                
            m_track_edit_tear->SetViaRad(aViaRad);
            m_track_edit_tear->Draw(aPanel, aDC, GR_XOR, wxPoint(0,0));
        }
    }
}

