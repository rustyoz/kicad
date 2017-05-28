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
#include <class_pcb_screen.h>
#include <class_drawpanel.h>
#include <convert_basic_shapes_to_polygon.h>
#include "tracknodeitem.h"

using namespace TrackNodeItem;


ROUNDEDCORNERTRACK::ROUNDEDCORNERTRACK(const ROUNDEDCORNERTRACK& aTrack) : TRACK(aTrack)
{
    m_StartVisible = m_Start;
    m_EndVisible = m_End;
    m_StartPointCorner = nullptr;
    m_EndPointCorner = nullptr;
}

//Create a new rounded corner track.
ROUNDEDCORNERTRACK::ROUNDEDCORNERTRACK(const BOARD_ITEM* aParent) : TRACK(const_cast<BOARD_ITEM*>(aParent))
{
    m_StartVisible = m_Start;
    m_EndVisible = m_End;
    m_StartPointCorner = nullptr;
    m_EndPointCorner = nullptr;
}

//Copy (Convert) old normal track.
ROUNDEDCORNERTRACK::ROUNDEDCORNERTRACK(const BOARD_ITEM* aParent, const TRACK* aTrack) : TRACK(const_cast<BOARD_ITEM*>(aParent), aTrack->Type())
{
    SetStart(aTrack->GetStart());
    SetEnd(aTrack->GetEnd());
    SetWidth(aTrack->GetWidth());
    SetLayer(aTrack->GetLayer());
    SetNetCode(aTrack->GetNetCode());
    SetStatus(aTrack->GetStatus());
    
    m_StartVisible = m_Start;
    m_EndVisible = m_End;
    m_StartPointCorner = nullptr;
    m_EndPointCorner = nullptr;
}

void ROUNDEDCORNERTRACK::ResetVisibleEndpoints(void)
{
    m_StartVisible = m_Start;
    m_EndVisible = m_End;
}

TrackNodeItem::ROUNDEDTRACKSCORNER* ROUNDEDCORNERTRACK::Contains(const wxPoint& aPos) const
{
    if(aPos == m_Start)
        return m_StartPointCorner;
    if(aPos == m_End)
        return m_EndPointCorner;
    return nullptr;
}

void ROUNDEDCORNERTRACK::TransformShapeWithClearanceToPolygon( SHAPE_POLY_SET& aCornerBuffer,
                                               int             aClearanceValue,
                                               int             aCircleToSegmentsCount,
                                               double          aCorrectionFactor ) const
{
    ::TransformRoundedEndsSegmentToPolygon( aCornerBuffer,
                                              m_StartVisible, m_EndVisible,
                                              aCircleToSegmentsCount,
                                              m_Width + ( 2 * aClearanceValue) );
}

//Copy from class_track.cpp and made modifications.
void ROUNDEDCORNERTRACK::Draw( EDA_DRAW_PANEL* aPanel, wxDC* aDC, GR_DRAWMODE aDrawMode, const wxPoint& aOffset )
{
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

    int l_trace = m_Width / 2;

    EDA_RECT* clip_box = aPanel->GetClipBox();
    
    if( aDC->LogicalToDeviceXRel( m_Width ) <= 1 )
    {
        GRLine( clip_box, aDC, m_StartVisible + aOffset, m_EndVisible + aOffset, 0, color );
        return;
    }

    double angle = AngleRad(m_StartVisible, m_EndVisible);

    if( !displ_opts->m_DisplayPcbTrackFill || GetState( FORCE_SKETCH ) )
    {
        wxPoint op_start_A = GetPoint(m_StartVisible, angle + M_PI_2, l_trace);
        wxPoint op_start_B = GetPoint(m_StartVisible, angle - M_PI_2, l_trace);
        wxPoint op_end_A = GetPoint(m_EndVisible, angle + M_PI_2, l_trace);
        wxPoint op_end_B = GetPoint(m_EndVisible, angle - M_PI_2, l_trace);
        
        GRLine( clip_box, aDC, op_start_A + aOffset, op_end_A + aOffset, 0, color );
        GRLine( clip_box, aDC, op_start_B + aOffset, op_end_B + aOffset, 0, color );

        GRArc1(clip_box, aDC, op_start_A + aOffset, op_start_B + aOffset, m_StartVisible + aOffset, 0, color);
        GRArc1(clip_box, aDC, op_end_B + aOffset, op_end_A + aOffset, m_EndVisible + aOffset, 0, color);

    }
    else
    {
        GRFillCSegm( clip_box, aDC, m_StartVisible.x + aOffset.x,m_StartVisible.y + aOffset.y, m_EndVisible.x + aOffset.x, m_EndVisible.y + aOffset.y, m_Width, color );
    }

    // Show clearance for tracks, not for zone segments
    if( IsCopperLayer( GetLayer() ) && ( ( displ_opts->m_ShowTrackClearanceMode == SHOW_CLEARANCE_NEW_AND_EDITED_TRACKS_AND_VIA_AREAS && ( IsDragging() || IsMoving() || IsNew() ) ) || ( displ_opts->m_ShowTrackClearanceMode == SHOW_CLEARANCE_ALWAYS ) ))
    {
        l_trace += GetClearance();

        wxPoint op_start_A = GetPoint(m_StartVisible, angle + M_PI_2, l_trace);
        wxPoint op_start_B = GetPoint(m_StartVisible, angle - M_PI_2, l_trace);
        wxPoint op_end_A = GetPoint(m_EndVisible, angle + M_PI_2, l_trace);
        wxPoint op_end_B = GetPoint(m_EndVisible, angle - M_PI_2, l_trace);
        
        GRLine( clip_box, aDC, op_start_A + aOffset, op_end_A + aOffset, 0, color );
        GRLine( clip_box, aDC, op_start_B + aOffset, op_end_B + aOffset, 0, color );
        
        GRArc1(clip_box, aDC, op_start_A + aOffset, op_start_B + aOffset, m_StartVisible + aOffset, 0, color);
        GRArc1(clip_box, aDC, op_end_B + aOffset, op_end_A + aOffset, m_EndVisible + aOffset, 0, color);
    }
    
    DrawShortNetname( aPanel, aDC, aDrawMode, color );
}

