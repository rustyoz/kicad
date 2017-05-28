/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2017 Heikki Pulkkinen.
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

#include "roundedcornertrack.h"
#include "roundedtrackscorner.h"
#include <convert_basic_shapes_to_polygon.h>
#include <gal/graphics_abstraction_layer.h> //GAL

#include <3d_rendering/3d_render_raytracing/shapes2D/croundsegment2d.h>
#include <3d_rendering/3d_render_raytracing/shapes2D/cfilledcircle2d.h>

using namespace TrackNodeItem;


ROUNDEDTRACKSCORNER::ROUNDEDTRACKSCORNER(const BOARD_ITEM* aParent, const TRACK* aTrackSeg, const TRACK* aTrackSegSecond, const wxPoint aPosition, const PARAMS aParams, const bool aCheckNullTrack) : TRACKNODEITEM(const_cast<BOARD_ITEM*>(aParent), PCB_ROUNDEDTRACKSCORNER_T)
{
    m_set_ok = true;
    m_clearance = aTrackSeg->GetClearance();
    m_num_arc_segs = 1;
    m_seg_points.resize(DEFAULT_SEGMENTS_NUM);
    m_seg_outer_points.resize(DEFAULT_OUTER_POLY_POINTS_NUM);
    m_seg_clearance_points.resize(DEFAULT_OUTER_POLY_POINTS_NUM);
    m_trackseg_second = const_cast<TRACK*>(aTrackSegSecond);
    m_length_set = 0;
    m_length_ratio = 100;
    
    m_connected_pos = aPosition;
    CallConstructor(aTrackSeg, aParams, aCheckNullTrack);
}

bool ROUNDEDTRACKSCORNER::CallConstructor(const TRACK* aTrackSeg, const PARAMS aParams, const bool aCheckNullTrack)
{

    if(TRACKNODEITEM::CallConstructor(aTrackSeg, aCheckNullTrack))
    {
        if((m_trackseg->Type() == PCB_TRACE_T) && (m_trackseg->Type() == PCB_TRACE_T))
            if(dynamic_cast<ROUNDEDCORNERTRACK*>(m_trackseg) && dynamic_cast<ROUNDEDCORNERTRACK*>(m_trackseg_second))
            {
                if((m_trackseg_second->GetWidth() == m_trackseg->GetWidth()))// && (AngleBtwTracks(m_trackseg, m_connected_pos, m_trackseg_second, m_connected_pos) != M_PI))
                {
                    SetTrackSecondEndpoint();
                    ConnectTrackSegs();
                    SetParams(aParams); 
                    return true;
                }
            }
    }
    m_created_ok = false;
    return false;
}

void ROUNDEDTRACKSCORNER::SetTrackSecondEndpoint(void)
{
    if(m_trackseg_second)
        if((m_connected_pos == m_trackseg_second->GetStart()) || (m_connected_pos == m_trackseg_second->GetEnd()))
            m_trackseg_second_startpos_is_pos = (m_connected_pos == m_trackseg_second->GetStart());
}

//Needed when removed.
void ROUNDEDTRACKSCORNER::ReleaseTrackSegs(void)
{
    if(m_trackseg && dynamic_cast<ROUNDEDCORNERTRACK*>(m_trackseg))
    {
        if(this == dynamic_cast<ROUNDEDCORNERTRACK*>(m_trackseg)->m_StartPointCorner)
            dynamic_cast<ROUNDEDCORNERTRACK*>(m_trackseg)->m_StartPointCorner = nullptr;
        if(this == dynamic_cast<ROUNDEDCORNERTRACK*>(m_trackseg)->m_EndPointCorner)
            dynamic_cast<ROUNDEDCORNERTRACK*>(m_trackseg)->m_EndPointCorner = nullptr;
    }
    if(m_trackseg_second && dynamic_cast<ROUNDEDCORNERTRACK*>(m_trackseg_second))
    {
        if(this == dynamic_cast<ROUNDEDCORNERTRACK*>(m_trackseg_second)->m_StartPointCorner)
            dynamic_cast<ROUNDEDCORNERTRACK*>(m_trackseg_second)->m_StartPointCorner = nullptr;
        if(this == dynamic_cast<ROUNDEDCORNERTRACK*>(m_trackseg_second)->m_EndPointCorner)
            dynamic_cast<ROUNDEDCORNERTRACK*>(m_trackseg_second)->m_EndPointCorner = nullptr;
    }
}

//Needed when Construct and Undo
void ROUNDEDTRACKSCORNER::ConnectTrackSegs(void)
{
    SetConnectedPos();
    if(m_trackseg && dynamic_cast<ROUNDEDCORNERTRACK*>(m_trackseg))
    {
        if(m_connected_pos == m_trackseg->GetStart())
            dynamic_cast<ROUNDEDCORNERTRACK*>(m_trackseg)->m_StartPointCorner = this;
        if(m_connected_pos == m_trackseg->GetEnd())
            dynamic_cast<ROUNDEDCORNERTRACK*>(m_trackseg)->m_EndPointCorner = this;
    }
    if(m_trackseg_second && dynamic_cast<ROUNDEDCORNERTRACK*>(m_trackseg_second))
    {
        if(m_connected_pos == m_trackseg_second->GetStart())
            dynamic_cast<ROUNDEDCORNERTRACK*>(m_trackseg_second)->m_StartPointCorner = this;
        if(m_connected_pos == m_trackseg_second->GetEnd())
            dynamic_cast<ROUNDEDCORNERTRACK*>(m_trackseg_second)->m_EndPointCorner = this;
    }
}

void ROUNDEDTRACKSCORNER::ResetVisibleEndpoints(void)
{
    if(m_trackseg && dynamic_cast<ROUNDEDCORNERTRACK*>(m_trackseg))
    {
        if(m_trackstartpos_is_pos)
            dynamic_cast<ROUNDEDCORNERTRACK*>(m_trackseg)->m_StartVisible = m_trackseg->GetStart();
        else
            dynamic_cast<ROUNDEDCORNERTRACK*>(m_trackseg)->m_EndVisible = m_trackseg->GetEnd();
    }
    if(m_trackseg_second && dynamic_cast<ROUNDEDCORNERTRACK*>(m_trackseg_second))
    {
        if(m_trackseg_second_startpos_is_pos)
            dynamic_cast<ROUNDEDCORNERTRACK*>(m_trackseg_second)->m_StartVisible = m_trackseg_second->GetStart();
        else
            dynamic_cast<ROUNDEDCORNERTRACK*>(m_trackseg_second)->m_EndVisible = m_trackseg_second->GetEnd();
    }
}

void ROUNDEDTRACKSCORNER::SetTracksVisibleEndpoints(void)
{
    if(m_trackseg && dynamic_cast<ROUNDEDCORNERTRACK*>(m_trackseg))
    {
        if(dynamic_cast<ROUNDEDCORNERTRACK*>(m_trackseg)->m_StartPointCorner == this)
            dynamic_cast<ROUNDEDCORNERTRACK*>(m_trackseg)->m_StartVisible = m_pos_start;
        if(dynamic_cast<ROUNDEDCORNERTRACK*>(m_trackseg)->m_EndPointCorner == this)
            dynamic_cast<ROUNDEDCORNERTRACK*>(m_trackseg)->m_EndVisible = m_pos_start;
    }
    if(m_trackseg_second && dynamic_cast<ROUNDEDCORNERTRACK*>(m_trackseg_second))
    {
        if(dynamic_cast<ROUNDEDCORNERTRACK*>(m_trackseg_second)->m_StartPointCorner == this)
            dynamic_cast<ROUNDEDCORNERTRACK*>(m_trackseg_second)->m_StartVisible = m_pos_end;
        if(dynamic_cast<ROUNDEDCORNERTRACK*>(m_trackseg_second)->m_EndPointCorner == this)
            dynamic_cast<ROUNDEDCORNERTRACK*>(m_trackseg_second)->m_EndVisible = m_pos_end;    
    }
}

bool ROUNDEDTRACKSCORNER::Update(void)
{
    if(!TRACKNODEITEM::Update() || !m_trackseg_second)
    {
        SetNotOKValues();
        return false;
    }

    m_set_ok = true;
    m_trackseg_second_length = m_trackseg_second->GetLength();
    
    m_angle_btw_tracks = AngleBtwTracks(m_trackseg, m_connected_pos, m_trackseg_second, m_connected_pos);
    m_angle_inner_btw_tracks = m_angle_btw_tracks;
    if(m_angle_inner_btw_tracks > M_PI)
        m_angle_inner_btw_tracks = M_PIx2 - m_angle_inner_btw_tracks;
    m_angle_inner_half = m_angle_inner_btw_tracks / 2.0;
    if((m_angle_btw_tracks == M_PI) || (m_angle_inner_btw_tracks < M_PI_2))
    {
        SetNotOKValues();
        return false;
    }
        
    SetArc();
    SetPoints();
    SetTracksVisibleEndpoints();
    return true;
}

void ROUNDEDTRACKSCORNER::SetNotOKValues(void)
{
    m_set_ok = false;
    m_pos_end = m_connected_pos;
    m_pos_start = m_connected_pos;
    m_mid_pos = m_connected_pos;
    SetTracksVisibleEndpoints();
}

void ROUNDEDTRACKSCORNER::SetArc(void)
{
    //Trackseg arced length.
    //Calculated length from trackseg width.
    uint seg_length_max = m_Width;
    //From set value.
    if(m_length_set)
        seg_length_max = m_length_set;
    uint max_dist_pos_to_arc_end = double(seg_length_max) * (double(m_length_ratio) / 100.0);
    
    //Half of the tracksegs length.
    uint seg_min_length = std::min(m_trackseg_second_length / 2, m_trackseg_length / 2);
    m_trackseg_arced_length = std::min(max_dist_pos_to_arc_end, seg_min_length);
}

void ROUNDEDTRACKSCORNER::SetPoints(void)
{
    m_pos = GetPoint(m_connected_pos, m_trackseg_angle, m_trackseg_arced_length);

    uint hyp = double(m_trackseg_arced_length) / cos(m_angle_inner_half);
    m_arc_radius = cathete(hyp, m_trackseg_arced_length);
    double angel_inner_half_opposite = M_PI_2 - m_angle_inner_half;
    
    double angle_tan = 0.0;
    
    if(m_angle_btw_tracks > M_PI)
    {
        angle_tan = m_trackseg_angle - M_PI_2;
        m_arc_center_pos = GetPoint(m_pos, m_trackseg_angle + M_PI_2, m_arc_radius);
        m_mid_pos = GetPoint(m_arc_center_pos, angle_tan - angel_inner_half_opposite, m_arc_radius);
    }
    else
    {
        angle_tan = m_trackseg_angle + M_PI_2;
        m_arc_center_pos = GetPoint(m_pos, m_trackseg_angle - M_PI_2, m_arc_radius);
        m_mid_pos = GetPoint(m_arc_center_pos, angle_tan + angel_inner_half_opposite, m_arc_radius);
    }

    double angle_add = (angel_inner_half_opposite / double(m_num_arc_segs)) * 2.0;

    int oc_ppA{0};
    int oc_ppB{0};
    
    for(int n = 0; n <= m_num_arc_segs; ++n)
    {
        m_seg_points[n] = GetPoint(m_arc_center_pos, angle_tan, m_arc_radius);
        oc_ppA = n;
        oc_ppB = m_num_arc_outer_points - n - 1;
        
        //Piece of cake in outer points is excact implementation, but this is close enough. 
        m_seg_outer_points[oc_ppA] = GetPoint(m_seg_points[n], angle_tan, m_trackseg_rad);
        m_seg_outer_points[oc_ppB] = GetPoint(m_seg_points[n], angle_tan + M_PI, m_trackseg_rad);
        m_seg_clearance_points[oc_ppA] = GetPoint(m_seg_points[n], angle_tan, m_trackseg_rad + m_clearance);
        m_seg_clearance_points[oc_ppB] = GetPoint(m_seg_points[n], angle_tan + M_PI, m_trackseg_rad + m_clearance);

        (m_angle_btw_tracks > M_PI)? angle_tan -= angle_add : angle_tan += angle_add;
    }
    
    m_pos_start = m_seg_points[0];
    m_pos_end = m_seg_points[m_num_arc_segs];
}

void ROUNDEDTRACKSCORNER::SetParams(const PARAMS aParams)
{
    m_length_set = aParams.length_set;
    m_length_ratio = aParams.length_ratio;
    m_num_arc_segs = aParams.num_segments;
    if(m_num_arc_segs > SEGMENTS_MAX)
        m_num_arc_segs = SEGMENTS_MAX;
    if(m_num_arc_segs < SEGMENTS_MIN)
        m_num_arc_segs = SEGMENTS_MIN;      
    m_num_arc_outer_points = m_num_arc_segs * 2 + 2;
    m_seg_points.resize(m_num_arc_segs + 1); 
    m_seg_outer_points.resize(m_num_arc_outer_points); 
    m_seg_clearance_points.resize(m_num_arc_outer_points); 
    
    Update();
}

ROUNDEDTRACKSCORNER::PARAMS ROUNDEDTRACKSCORNER::GetParams(void) const
{
    PARAMS returnParams;
    returnParams.length_ratio = m_length_ratio;
    returnParams.length_set = m_length_set;
    returnParams.num_segments = m_num_arc_segs;
    return returnParams;
}

double ROUNDEDTRACKSCORNER::GetLengthVisible(void) const
{
    double length = 0.0;
    if(m_set_ok)
        length = (M_PI - m_angle_inner_btw_tracks) * double(m_arc_radius);
    return length;
}

wxPoint ROUNDEDTRACKSCORNER::GetPointInArc(const wxPoint aFromPos, const double aLength) const
{
    wxPoint ret_point(0,0);
    double angle_ratio = aLength / GetLengthVisible();
    if( angle_ratio <= 1.0)
    {
        double angle = (M_PI - m_angle_inner_btw_tracks) * angle_ratio;
        if(m_pos_end == aFromPos)
            angle = (M_PI - m_angle_inner_btw_tracks) - angle;

        if((m_pos_start == aFromPos) || (m_pos_end == aFromPos))
        {
            if(m_angle_btw_tracks > M_PI)
                ret_point = GetPoint(m_arc_center_pos, m_trackseg_angle - M_PI_2 - angle, m_arc_radius);
            else
                ret_point = GetPoint(m_arc_center_pos, m_trackseg_angle + M_PI_2 + angle, m_arc_radius);
        }
    }
    return ret_point;
}

EDA_ITEM* ROUNDEDTRACKSCORNER::Clone() const
{
    return new ROUNDEDTRACKSCORNER( *this );
}

const EDA_RECT ROUNDEDTRACKSCORNER::GetBoundingBox() const
{
    int ymax = std::max(m_seg_points[0].y, m_seg_points[m_num_arc_segs].y);
        ymax = std::max(ymax, m_connected_pos.y);
    int xmax = std::max(m_seg_points[0].x, m_seg_points[m_num_arc_segs].x);
        xmax = std::max(xmax, m_connected_pos.x);
    int ymin = std::min(m_seg_points[0].y, m_seg_points[m_num_arc_segs].y);
        ymin = std::min(ymin, m_connected_pos.y);
    int xmin = std::min(m_seg_points[0].x, m_seg_points[m_num_arc_segs].x);
        xmin = std::min(xmin, m_connected_pos.x);

    ymax += m_trackseg_rad + m_clearance;
    xmax += m_trackseg_rad + m_clearance;
    ymin -= m_trackseg_rad + m_clearance;
    xmin -= m_trackseg_rad + m_clearance;
    
    EDA_RECT ret( wxPoint( xmin, ymin ), wxSize( xmax - xmin + 1, ymax - ymin + 1 ) );

    return ret;
}

uint ROUNDEDTRACKSCORNER::GetBoundingRad(void) const
{
    return m_trackseg->GetWidth() >> 1;
}

wxString ROUNDEDTRACKSCORNER::GetSelectMenuText() const
{
    NETINFO_ITEM* net;
    BOARD* board = GetBoard();

    wxString text = GetClass();
    text << wxT(" ") << _(" [");
    text << wxT(", ") << m_length_ratio;
    text << wxT(", ") << m_num_arc_segs;
    text << wxT("]");

    //(this->IsLocked())? text << _( " Locked" ) : text << _( " Unlocked" );

    if( board )
    {
        net = board->FindNet( GetNetCode() );

        if( net )
            text << wxT( " [" ) << net->GetNetname() << wxT( "]" );
    }
    else
    {
        text << _( "** BOARD NOT DEFINED **" );
    }

    text << _( " on " ) << GetLayerName() << wxT("  ") << _("Net:") << GetNetCode();
    
    return text;
}

void ROUNDEDTRACKSCORNER::TransformShapeWithClearanceToPolygon( SHAPE_POLY_SET& aCornerBuffer,
                                               int             aClearanceValue,
                                               int             aCircleToSegmentsCount,
                                               double          aCorrectionFactor ) const
{
    if(IsSetOK() && m_on)
    {
        for(uint n = 0; n < m_seg_points.size() - 1; ++n)
        {
            TransformRoundedEndsSegmentToPolygon( aCornerBuffer, m_seg_points[n], m_seg_points[n + 1], aCircleToSegmentsCount, m_Width + ( 2 * aClearanceValue) );
        }
    }
}

void ROUNDEDTRACKSCORNER::AddTo3DContainer(CBVHCONTAINER2D* aContainer, const double aBiuTo3Dunits)
{
    if(IsSetOK() && m_on)
    {
        for(uint n = 0; n < m_seg_points.size() - 1; ++n)
        {
            SFVEC2F start3DU(m_seg_points[n].x * aBiuTo3Dunits, -m_seg_points[n].y * aBiuTo3Dunits);
            SFVEC2F end3DU (m_seg_points[n + 1].x * aBiuTo3Dunits, -m_seg_points[n + 1].y * aBiuTo3Dunits);
            if(Is_segment_a_circle(start3DU, end3DU))
                aContainer->Add( new CFILLEDCIRCLE2D(start3DU, m_trackseg_rad * aBiuTo3Dunits, *this));
            else
                aContainer->Add(new CROUNDSEGMENT2D(start3DU, end3DU, m_Width * aBiuTo3Dunits, *this));
        }
    }
}

void ROUNDEDTRACKSCORNER::DrawItem(EDA_DRAW_PANEL* aPanel, wxDC* aDC, const COLOR4D aColor, const wxPoint& aOffset, const DISPLAY_OPTIONS* aDisplOpts)
{
    if(m_on && m_trackseg && m_trackseg_second && m_set_ok)
    {
        EDA_RECT* clip_box = aPanel->GetClipBox();
        bool showClearance = (IsCopperLayer(m_trackseg->GetLayer()) && ((aDisplOpts->m_ShowTrackClearanceMode == SHOW_CLEARANCE_NEW_AND_EDITED_TRACKS_AND_VIA_AREAS && ( m_trackseg->IsDragging() || m_trackseg->IsMoving() || m_trackseg->IsNew()  ||  m_trackseg_second->IsDragging() || m_trackseg_second->IsMoving() || m_trackseg_second->IsNew() )) || ( aDisplOpts->m_ShowTrackClearanceMode == SHOW_CLEARANCE_ALWAYS ))) ;//&& m_can_draw_clearance;
        
        if((!aDisplOpts->m_DisplayPcbTrackFill || GetState( FORCE_SKETCH ) || m_draw_mode_unfill))
        {
            for(int n = 0; n < m_num_arc_segs; ++n)
            {
                GRLine(clip_box, aDC, m_seg_outer_points[n] + aOffset, m_seg_outer_points[n + 1] + aOffset, 2, aColor);
                GRLine(clip_box, aDC, m_seg_outer_points[m_num_arc_outer_points - n - 1] + aOffset, m_seg_outer_points[m_num_arc_outer_points - n - 2] + aOffset, 2, aColor);
            }
        }
        else
        {
            //Corner is drawn arc, but it consists segments.
            if(m_angle_btw_tracks < M_PI)
                GRArc1(clip_box, aDC, m_pos_start, m_pos_end, m_arc_center_pos, m_Width, aColor );
            else
                GRArc1(clip_box, aDC, m_pos_end, m_pos_start, m_arc_center_pos, m_Width, aColor );
        }

        if(showClearance)
        {
            for(int n = 0; n < m_num_arc_segs; ++n)
            {
                GRLine(clip_box, aDC, m_seg_clearance_points[n] + aOffset, m_seg_clearance_points[n + 1] + aOffset, 2, aColor);
                GRLine(clip_box, aDC, m_seg_clearance_points[m_num_arc_outer_points - n - 1] + aOffset, m_seg_clearance_points[m_num_arc_outer_points - n - 2] + aOffset, 2, aColor);
            }

            int arc1_first = 0;
            int arc1_second = m_num_arc_outer_points - 1;
            int arc2_first = m_num_arc_segs + 1;
            int arc2_second = m_num_arc_segs;
            if(m_angle_btw_tracks < M_PI)
            {
                arc1_first = m_num_arc_outer_points - 1;
                arc1_second = 0;
                arc2_first = m_num_arc_segs;
                arc2_second = m_num_arc_segs + 1;
            }

            GRArc1(clip_box, aDC, m_seg_clearance_points[arc1_first] + aOffset, m_seg_clearance_points[arc1_second] + aOffset, m_pos_start + aOffset, 0, aColor);
            GRArc1(clip_box, aDC, m_seg_clearance_points[arc2_first] + aOffset, m_seg_clearance_points[arc2_second] + aOffset, m_pos_end + aOffset, 0, aColor);
        }
    }
}


void ROUNDEDTRACKSCORNER::DrawItem(KIGFX::GAL* aGal, const bool aIsSketch )
{
    if(m_on && m_trackseg && m_trackseg_second && m_set_ok)
    {
        std::deque<VECTOR2D> points;
        points.clear();

        if(aIsSketch)
        {
            for(int n = 0; n < m_num_arc_segs; ++n)
            {
                aGal->DrawLine(VECTOR2D(m_seg_outer_points[n]), VECTOR2D(m_seg_outer_points[n+1]));
                aGal->DrawLine(VECTOR2D(m_seg_outer_points[m_num_arc_outer_points - n - 1]),
                            VECTOR2D(m_seg_outer_points[m_num_arc_outer_points - n - 2]));

            }
        }
        else
        {
            for(int n = 0; n < m_num_arc_segs; ++n)
            {
                aGal->DrawSegment(m_seg_points[n], m_seg_points[n+1], m_Width);
            }
        }
    }
}

ROUNDEDTRACKSCORNER_ROUTE_EDIT::ROUNDEDTRACKSCORNER_ROUTE_EDIT(const BOARD_ITEM* aParent, const TRACK* aTrackSeg, const TRACK* aTrackSegSecond, const PARAMS aParams, const bool aCheckNullTrack) : ROUNDEDTRACKSCORNER(aParent, aTrackSeg, aTrackSegSecond, aTrackSeg->GetStart(), aParams, aCheckNullTrack) 
{
    if(aTrackSeg)
        m_connected_pos = aTrackSeg->GetStart();
    
    CallConstructor(aTrackSeg, aParams, aCheckNullTrack);
}

bool ROUNDEDTRACKSCORNER_ROUTE_EDIT::ChangeTracks(const TRACK* aNewTrack, const TRACK* aNewTrackSecond)
{
    ReleaseTrackSegs();
    if(aNewTrack && aNewTrackSecond && (aNewTrack->Type() == PCB_TRACE_T) && (aNewTrackSecond->Type() == PCB_TRACE_T))
    {
        m_connected_pos = aNewTrack->GetStart();
        if(SetTrackSeg(aNewTrack, true))
        {
            if(m_connected_pos == aNewTrackSecond->GetStart() || m_connected_pos == aNewTrackSecond->GetEnd())
            {
                SetTracksSegSecond(aNewTrackSecond);
                ConnectTrackSegs();
                return true;
            }
        }
    }
    return false;
}

void ROUNDEDTRACKSCORNER_ROUTE_EDIT::DrawItem(EDA_DRAW_PANEL* aPanel, wxDC* aDC, const COLOR4D aColor, const wxPoint& aOffset, const DISPLAY_OPTIONS* aDisplOpts)
{
    bool opts_track_fill = aDisplOpts->m_DisplayPcbTrackFill;
    const_cast<DISPLAY_OPTIONS*>(aDisplOpts)->m_DisplayPcbTrackFill = true;
    ROUNDEDTRACKSCORNER::DrawItem(aPanel, aDC, aColor, aOffset, aDisplOpts);
    const_cast<DISPLAY_OPTIONS*>(aDisplOpts)->m_DisplayPcbTrackFill = opts_track_fill;
}

