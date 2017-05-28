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

#include "tracknodeitem.h"

using namespace TrackNodeItem;

//-----------------------------------------------------------------------------------------------------/
// Base class of nodeitem
//-----------------------------------------------------------------------------------------------------/
TRACKNODEITEM::TRACKNODEITEM(const BOARD_ITEM* aParent, KICAD_T aID_Type) : TRACK(const_cast<BOARD_ITEM*>(aParent), aID_Type)
{
    m_created_ok = false;
    m_trackseg = nullptr;
    m_trackstartpos_is_pos = true;
}

bool TRACKNODEITEM::CallConstructor(const TRACK* aTrackSeg, const bool aCheckNullTrack)
{
    if(SetTrackSeg(aTrackSeg, aCheckNullTrack))
    {
        if((m_trackseg->GetEnd() == m_connected_pos) || (m_trackseg->GetStart() == m_connected_pos))
        {
            SetFlags(aTrackSeg->GetFlags());
            SetStatus(aTrackSeg->GetStatus());
            SetNetCode(aTrackSeg->GetNetCode());
            m_created_ok = true;
            return true;
        }
    }
    return false;
}

void TRACKNODEITEM::SetTrackEndpoint(void)
{
    if(m_trackseg)
        if((m_connected_pos == m_trackseg->GetStart()) || (m_connected_pos == m_trackseg->GetEnd()))
            m_trackstartpos_is_pos = (m_connected_pos == m_trackseg->GetStart());
}

bool TRACKNODEITEM::SetTrackSeg(const TRACK* aTrackSeg, const bool aCheckNullTrack)
{
    if(aTrackSeg && aTrackSeg->Type() == PCB_TRACE_T)
    {    
        m_trackseg = nullptr;
        //Try to find first track segment before or after ... hmmmm
        TRACK* track_seg = const_cast<TRACK*>(aTrackSeg);
        bool ok = false;
        do
        {
            if(track_seg)
            {
                if(aCheckNullTrack) 
                    while((track_seg->GetStart() == track_seg->GetEnd()))
                    {
                        track_seg=track_seg->Back();
                        if(!track_seg)
                            return false;
                    }
            }
            if(!track_seg)
                return false;
            else 
                if(!((m_connected_pos == track_seg->GetStart()) || (m_connected_pos == track_seg->GetEnd())))
                    track_seg = track_seg->Back();
                else
                    ok=true;
        }
        while(!ok);
    
        if(aTrackSeg->GetLayer() != track_seg->GetLayer())
            return false;
        
        m_trackseg = track_seg;//Pointer to that track where item belongs.
        SetTrackEndpoint();
        return true;
    }
    return false;
}

void TRACKNODEITEM::SetConnectedPos(void)
{
    m_opposite_pos = m_trackseg->GetEnd();
    m_connected_pos = m_trackseg->GetEnd();
    m_trackstartpos_is_pos? m_connected_pos = m_trackseg->GetStart() : m_opposite_pos = m_trackseg->GetStart();  
}

bool TRACKNODEITEM::Update(void)
{
    if(!m_trackseg || (m_trackseg->Type() != PCB_TRACE_T))
        return false;

    SetConnectedPos();
    
    m_Layer = m_trackseg->GetLayer();
    m_Width = m_trackseg->GetWidth();
    
    m_trackseg_angle = AngleRad(m_connected_pos, m_opposite_pos);
    m_trackseg_angle_inv = m_trackseg_angle + M_PI;
    m_trackseg_rad = m_Width>>1;
    m_trackseg_length = m_trackseg->GetLength();

    m_clearance = m_trackseg->GetClearance();
    
    m_poly_seg_width = m_Width>>1; //Poly segments width.
    m_poly_seg_rad = m_poly_seg_width>>1; //Poly segments rad.

    m_Start = m_connected_pos;
    m_End = m_connected_pos;
    return true;
}

wxPoint TRACKNODEITEM::GetPolyPoint(const uint aPoint) const
{
    if(aPoint < m_seg_points.size())
        return m_seg_points[aPoint];
    return m_seg_points[0];
}

void TRACKNODEITEM::Draw(EDA_DRAW_PANEL* aPanel, wxDC* aDC, GR_DRAWMODE aDrawMode, const wxPoint& aOffset)
{
    //*** Copy from TRACK. ***************************************
    BOARD* brd = GetBoard();
    COLOR4D color = brd->GetLayerColor( m_Layer );

    if( brd->IsLayerVisible( m_Layer ) == false && !( aDrawMode & GR_HIGHLIGHT ) )
        return;

#ifdef USE_WX_OVERLAY
    // If dragged not draw in OnPaint otherwise remains impressed in wxOverlay
    if( (m_Flags & IS_DRAGGED) && aDC->IsKindOf(wxCLASSINFO(wxPaintDC)))
      return;
#endif

    DISPLAY_OPTIONS* displ_opts = (DISPLAY_OPTIONS*) aPanel->GetDisplayOptions();

    if( ( aDrawMode & GR_ALLOW_HIGHCONTRAST ) && displ_opts->m_ContrastModeDisplay )
    {
        PCB_LAYER_ID curr_layer = ( (PCB_SCREEN*) aPanel->GetScreen() )->m_Active_Layer;

        if( !IsOnLayer( curr_layer ) )
            color = COLOR4D( DARKDARKGRAY );
    }

    if( ( aDrawMode & GR_HIGHLIGHT ) && !( aDrawMode & GR_AND ) )
        color.SetToLegacyHighlightColor();

    color.a = 0.588;

    GRSetDrawMode( aDC, aDrawMode );

    //****** Copy from TRACK end. ***********************************

    DrawItem( aPanel, aDC, color, aOffset, displ_opts);
}

//Gal draw.
void TRACKNODEITEM::Draw( KIGFX::GAL* aGal, const KIGFX::PCB_RENDER_SETTINGS* aPcbSettings, const float aOutlineWidth, int aLayer)
{
    if(IsCopperLayer(aLayer))
    {
        const KIGFX::COLOR4D& color = aPcbSettings->GetColor(m_trackseg, aLayer);
        aGal->SetStrokeColor(color);
        aGal->SetIsStroke(true);

        bool sketch = aPcbSettings->GetSketchMode(LAYER_TRACKS);
        aGal->SetIsFill(!sketch);
        aGal->SetIsStroke(sketch);
        if(sketch)
        {
            // Outline mode
            aGal->SetLineWidth(aOutlineWidth);
            aGal->SetStrokeColor(color);
        }
        else
        {
            // Filled mode
            aGal->SetFillColor(color);
        }

        DrawItem(aGal, sketch);
    }
}

//-----------------------------------------------------------------------------------------------------/
// MISC 
//-----------------------------------------------------------------------------------------------------/
int TrackNodeItem::GetMaxWidth(Tracks_Container& aTracksList)
{
    int width_max = 0;
    for(auto track : aTracksList)
    {
        if(width_max < track->GetWidth())
            width_max = track->GetWidth();
    }
    return width_max;
}

TRACK* TrackNodeItem::GetMaxWidthTrack(Tracks_Container& aTracksList)
{
    TRACK* ret_track = nullptr;
    int width_max = 0;
    for(auto track : aTracksList)
    {
        if(width_max < track->GetWidth())
        {
            width_max = track->GetWidth();
            ret_track = track;
        }
    }
    return ret_track;
}

bool TrackNodeItem::EndPosNearest(const TRACK* aTrackSegAt, const wxPoint aPos)
{
    int start_dist = hypot(abs(aTrackSegAt->GetStart().y - aPos.y) , abs(aTrackSegAt->GetStart().x - aPos.x));
    int end_dist = hypot(abs(aTrackSegAt->GetEnd().y - aPos.y) , abs(aTrackSegAt->GetEnd().x - aPos.x));
    return (end_dist < start_dist)? true: false;
}

bool TrackNodeItem::StartPosNearest(const TRACK* aTrackSegAt, const wxPoint aPos)
{
    return !EndPosNearest(aTrackSegAt, aPos);
}

wxPoint TrackNodeItem::TrackSegNearestEndpoint(const TRACK* aTrackSegAt, const wxPoint aPos)
{
    bool end_nearest = EndPosNearest(aTrackSegAt, aPos);
    wxPoint track_pos;
    (end_nearest)? track_pos = aTrackSegAt->GetEnd() : track_pos = aTrackSegAt->GetStart();
    return track_pos;
}

wxPoint TrackNodeItem::TracksCommonPos(const TRACK* aTrackSegFirst, const TRACK* aTrackSegSecond)
{
    wxPoint pos{0,0};
    if(aTrackSegFirst && aTrackSegSecond)
    {
        if((aTrackSegFirst->GetStart() == aTrackSegSecond->GetStart()) || (aTrackSegFirst->GetStart() == aTrackSegSecond->GetEnd()))
            pos = aTrackSegFirst->GetStart();
        else
            if((aTrackSegFirst->GetEnd() == aTrackSegSecond->GetStart()) || (aTrackSegFirst->GetEnd() == aTrackSegSecond->GetEnd()))
                pos = aTrackSegFirst->GetEnd();
    }
    return pos;
}

//------------------------------------------------------------------------------------------------------
//Collect all Track segments conneted to aTrackSeg at point aPosAt
//Do not return aTrackSeg itself in aTracksList vector.
void TrackNodeItem::Collect(const TRACK* aTrackSeg, const wxPoint aPosAt, Tracks_Container& aTracksList)
{
    if(aTrackSeg && ((aTrackSeg->GetStart() == aPosAt) || (aTrackSeg->GetEnd() == aPosAt)))
    {
        std::unique_ptr<SCAN_NET_COLLECT> track(new SCAN_NET_COLLECT(aTrackSeg, aPosAt, &aTracksList));
        if(track)
            track->Execute();
    }
}

SCAN_NET_COLLECT::SCAN_NET_COLLECT(const TRACK* aTrackSeg, const wxPoint aPosAt, Tracks_Container* aTracksList) : SCAN_NET_BASE(aTrackSeg)
{
    m_pos = aPosAt;
    m_layer = aTrackSeg->GetLayer();
    m_tracks_list = aTracksList;
    m_tracks_list->clear();
}

bool SCAN_NET_COLLECT::ExecuteAt(const TRACK* aTrackSeg)
{
    if((aTrackSeg != m_net_start_seg) && (aTrackSeg->Type() == PCB_TRACE_T) && aTrackSeg->IsOnLayer(m_layer))
        if((aTrackSeg->GetStart() == m_pos) || (aTrackSeg->GetEnd() == m_pos))
            m_tracks_list->insert(const_cast<TRACK*>(aTrackSeg));
        
    return false;
}
//------------------------------------------------------------------------------------------------------

//------------------------------------------------------------------------------------------------------
//Look if there is T- connection in aTrackSeg at pos aPosAt.
//Return segments aTrackSegNext and aTrackSegBack values, otherwise nullptr;
bool TrackNodeItem::Find_T_Tracks(const TRACK* aTrackSegAt, const wxPoint aPosAt, TRACK* &aTrackSegNext, TRACK* &aTrackSegBack)
{
    if(aTrackSegAt && (aTrackSegAt->Type() == PCB_TRACE_T))
    {
        aTrackSegBack = nullptr;
        aTrackSegNext = nullptr;
        Tracks_Container track_list;
        
        std::unique_ptr<SCAN_NET_FIND_T_TRACKS> find_t(new SCAN_NET_FIND_T_TRACKS(aTrackSegAt, aPosAt, &track_list));
        if(find_t)
        {
            find_t->Execute();
        
            aTrackSegBack = find_t->GetResultTrack(0);
            aTrackSegNext = find_t->GetResultTrack(1);
        }
        
        if(aTrackSegNext && aTrackSegBack)
            return true;
    }
    return false;
}

TRACK* SCAN_NET_FIND_T_TRACKS::GetResultTrack(bool aFirstSecond) const
{
    if(aFirstSecond)
        return m_result_track_1;
    else
        return m_result_track_0;
}

SCAN_NET_FIND_T_TRACKS::SCAN_NET_FIND_T_TRACKS(const TRACK* aTrackSeg, const wxPoint aPosAt, Tracks_Container* aTracksList) : SCAN_NET_COLLECT(aTrackSeg, aPosAt, aTracksList)
{
    m_result_track_0 = nullptr;
    m_result_track_1 = nullptr;
    double angle_this = TrackSegAngle(m_net_start_seg, aPosAt);
    m_angle_next = angle_this + M_PI_2;
    if(m_angle_next >= M_PIx2)
        m_angle_next -= M_PIx2;
    m_angle_back = angle_this - M_PI_2;
    if(m_angle_back < 0.0)
        m_angle_back += M_PIx2;
    
    //m_angle_next_rnd = Rad2MilsInt(m_angle_next);
    //m_angle_back_rnd = Rad2MilsInt(m_angle_back);
    m_angle_next_rnd = Rad2DeciDegRnd(m_angle_next);
    m_angle_back_rnd = Rad2DeciDegRnd(m_angle_back);
}

bool SCAN_NET_FIND_T_TRACKS::ExecuteAt(const TRACK* aTrackSeg)
{
    if((aTrackSeg != m_net_start_seg) && (aTrackSeg->Type() == PCB_TRACE_T) && aTrackSeg->IsOnLayer(m_layer))
        if((aTrackSeg->GetStart() == m_pos) || (aTrackSeg->GetEnd() == m_pos))
        {
            double angle_track = TrackSegAngle(aTrackSeg, m_pos);
            //int angle_track_rnd = Rad2MilsInt(angle_track);
            int angle_track_rnd = Rad2DeciDegRnd(angle_track);
            
            if(angle_track_rnd == m_angle_back_rnd)
                m_result_track_0 = const_cast<TRACK*>(aTrackSeg);
            if(angle_track_rnd == m_angle_next_rnd)
                m_result_track_1 = const_cast<TRACK*>(aTrackSeg);
        }

    if(m_result_track_0 && m_result_track_1)
        return true;
    
    return false;
}
//------------------------------------------------------------------------------------------------------

//------------------------------------------------------------------------------------------------------
//Base class scanning one net where aNetSeg belongs.
SCAN_NET_BASE::SCAN_NET_BASE(const TRACK* aNetStartSeg)
{
    m_net_start_seg = const_cast<TRACK*>(aNetStartSeg);
    m_reverse = false;
}

void SCAN_NET_BASE::Execute(void)
{
    if(m_net_start_seg)
    {
        int net_code = m_net_start_seg->GetNetCode();
        for(uint n = 0; n < 2; ++n)
        {
            TRACK* track_seg = m_net_start_seg;
            if(n)
                (m_reverse)? track_seg = m_net_start_seg->Next() : track_seg = m_net_start_seg->Back();
                
            while(track_seg)
            {
                if(track_seg->GetNetCode() == net_code)
                {
                    if(ExecuteAt(track_seg)) //Break when virtual function true. MUST REMEMBER!
                        return;
                
                    if(m_reverse)
                        (n)? track_seg = track_seg->Next() : track_seg = track_seg->Back();
                    else
                        (n)? track_seg = track_seg->Back() : track_seg = track_seg->Next();
                }
                else
                    track_seg = nullptr;
            }
        }
    }
}
//------------------------------------------------------------------------------------------------------


//------------------------------------------------------------------------------------------------------
// GAL
//------------------------------------------------------------------------------------------------------
//Gal canvas arc draw with angles.
void TrackNodeItem::DrawArc(KIGFX::GAL* aGal, const wxPoint& aCenterPoint, const double aStartAngle, const double aEndAngle, const double aRad)
{
    double angle = NormAngle(aEndAngle - aStartAngle, 0.0, M_PIx2, M_PIx2);
    wxPoint start_point = GetPoint(aCenterPoint, aStartAngle, aRad);
    for( double n = 0.0; n < angle; n+=ALPHA_INC)
    {
        double endpoint_angle = aStartAngle + n;
        wxPoint end_point = GetPoint(aCenterPoint, endpoint_angle, aRad);
        aGal->DrawLine(VECTOR2D(start_point), VECTOR2D(end_point));
        start_point = end_point;
    }
    aGal->DrawLine(VECTOR2D(start_point), VECTOR2D(GetPoint(aCenterPoint, aEndAngle, aRad)));
}

//Gal canvas arc draw with points. Draw same way as Arc1 in legacy. 
//Radius can set differently than distance to center point from start or end point.
void TrackNodeItem::DrawArc(KIGFX::GAL* aGal, const wxPoint& aCenterPoint, const wxPoint& aStartPoint, const wxPoint& aEndPoint, const double aRad)
{
    double start_angle = AngleRad(aCenterPoint, aStartPoint);
    double end_angle = AngleRad(aCenterPoint, aEndPoint);
    DrawArc(aGal, aCenterPoint, start_angle, end_angle, aRad);
}



