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

#include "teardrop.h"
#include "roundedcornertrack.h"

#include <convert_basic_shapes_to_polygon.h>
#include <3d_rendering/3d_render_raytracing/shapes2D/ctriangle2d.h>
#include <3d_rendering/3d_render_raytracing/shapes2D/cfilledcircle2d.h>
#include <3d_rendering/3d_render_raytracing/shapes2D/croundsegment2d.h>

using namespace TrackNodeItem;


//-----------------------------------------------------------------------------------------------------/
// TEARDROP BASE
//-----------------------------------------------------------------------------------------------------/
//Teardrop base class
TEARDROP::TEARDROP(const BOARD_ITEM* aParent, const TRACK* aTrackSeg) : TRACKNODEITEM(const_cast<BOARD_ITEM*>(aParent), PCB_TEARDROP_T)
{
    m_set_ok = true;
    m_clearance = aTrackSeg->GetClearance();
    m_can_draw_clearance = true;
    m_num_arc_segs = 1;
    m_seg_points.resize(FILLET_POLY_POINTS_NUM);
    m_seg_outer_points.resize(FILLET_OUTER_POLY_POINTS_NUM);
    m_seg_clearance_points.resize(FILLET_CLEARANCE_POLY_POINTS_NUM);
    m_draw_mode_unfill = false;
    
    m_connected_item = nullptr;
    m_connected_item_rad = MIN_POLYSEG_WIDTH; 
    m_connected_pos_length_delta = 0;
    m_length_width_corr = 0;
    
}

bool TEARDROP::CallConstructor(const TRACK* aTrackSeg, const PARAMS aParams, const bool aCheckNullTrack)
{
    
    if(TRACKNODEITEM::CallConstructor(aTrackSeg, aCheckNullTrack))
    {
        SetParams(aParams);  
        return true;
    }
    return false;
}

bool TEARDROP::Update(void)
{
    if(!TRACKNODEITEM::Update())
    {
        m_set_ok = false;
        return false;
    }

    m_set_ok = true;

    m_connected_pos_delta = m_connected_pos;
    m_connected_pos_length_delta = 0;    
    m_can_draw_clearance = true;
    
    SetConnectedItem();
    SetRoundedCornerTrack();
    SetSize();

    if(m_shape != ZERO_T)
    {
        TestTrackLength();
        TestTrackWidth();
        TestSize();
    }
    if(!m_set_ok)
        SetNotOKValues();
    SetPoints();

    return true;
}

void TEARDROP::SetRoundedCornerTrack(void)
{
    if(dynamic_cast<ROUNDEDCORNERTRACK*>(m_trackseg))
        m_trackseg_length = dynamic_cast<ROUNDEDCORNERTRACK*>(m_trackseg)->GetLengthVisible();
}

void TEARDROP::SetSize(void)
{
    switch(m_shape)
    {
        case SUBLAND_T:
            CalcSubland(m_width_ratio, m_length_ratio);
            break;
        case FILLET_T:
            CalcFillet(m_width_ratio, m_length_ratio);
            break;
        case TEARDROP_T:
            CalcTeardrop(m_width_ratio, m_length_ratio);
            break;
        case ZERO_T:
            m_width_rad = m_trackseg_rad>>1;
            m_length = m_connected_item_rad<<1;
            break;
        default:;
    }
    m_Width = m_poly_seg_width; //Sets m_Width value for DRC
}

void TEARDROP::SetPoints(void)
{
    m_connected_pos_delta = GetPoint(m_connected_pos, m_trackseg_angle, m_connected_pos_length_delta);
    m_pos = GetPoint(m_connected_pos_delta, m_trackseg_angle, m_length);
    switch(m_shape)
    {
        case SUBLAND_T:
            m_mid_pos = m_pos;
            //With DRC and plot for. Differs TEARDROP and FILLET
            m_Width = m_width_rad<<1; 
            break;
        case FILLET_T:
            m_mid_pos = GetPoint(m_connected_pos_delta, m_trackseg_angle, m_length / 2);
            SetFilletPoints();
            break;
        case TEARDROP_T:
            m_mid_pos = GetPoint(m_connected_pos_delta, m_trackseg_angle, m_length / 2);
            SetTeardropPoints();        //CallConstruct must be call at the end from derived class constructor.

            break;
        case ZERO_T:
            m_pos = GetPoint(m_connected_pos, m_trackseg_angle, m_width_rad);
            m_mid_pos = GetPoint(m_pos, m_trackseg_angle, m_width_rad); //Center pos is second point of line.
            break;
        default:;
    }
}

uint TEARDROP::CalcSublandRad(const int aWidthRatio, const uint aConnectedItemRad, const uint aTrackSegRad)
{
    return (double(aConnectedItemRad) * (double(aWidthRatio) / 100.0));
}

uint TEARDROP::CalcSublandPosDist(const int aLengthRatio, const uint aConnectedItemRad, const uint aSublandRad)
{
    return aConnectedItemRad - aSublandRad + (aSublandRad * aLengthRatio / 100.0);
}

void TEARDROP::CalcSubland(const int aWidthRatio, const int aLengthRatio)
{
    m_width_rad = CalcSublandRad(aWidthRatio, m_connected_item_rad, m_trackseg_rad);
    m_length = CalcSublandPosDist(aLengthRatio, m_connected_item_rad, m_width_rad);
    int tr_d = m_trackseg_length - m_length - m_width_rad + m_trackseg_rad;
    m_connected_pos_length_delta = std::min(m_connected_pos_length_delta, tr_d);
}

uint TEARDROP::CalcFilletWidthRad(const int aWidthRatio, const uint aConnectedItemRad, const uint aTrackSegRad)
{
    if(aWidthRatio > 100)
        return (double(aConnectedItemRad) * (double(aWidthRatio) / 100.0)) - aTrackSegRad;
    else
        return double(aConnectedItemRad - aTrackSegRad) * (double(aWidthRatio) / 100.0);
}

uint TEARDROP::CalcFilletPosDist(const int aLengthRatio, const uint aConnectedItemRad)
{
    return aConnectedItemRad + double(aConnectedItemRad) * (double(aLengthRatio) / 100.0) - m_trackseg_rad;
}

uint TEARDROP::CalcFilletLength(const int aLengthRatio)
{
    uint max_dist_item_to_tear = CalcFilletPosDist(aLengthRatio, m_connected_item_rad) + m_connected_pos_length_delta;
    uint dist_item_to_tear = std::min(max_dist_item_to_tear, m_trackseg_length);
    return dist_item_to_tear - m_connected_pos_length_delta;
}

void TEARDROP::CalcFilletSegs(void)
{
    double seg_angle = asin(double(m_width_rad) / double(m_length));
    m_fillet_seg_A_angle = m_trackseg_angle_inv + seg_angle;
    m_fillet_seg_B_angle = m_trackseg_angle_inv - seg_angle;
    m_fillet_seg_length = cathete(m_length, m_width_rad);
}

void TEARDROP::CalcFillet(const int aWidthRatio, const int aLengthRatio)
{
    m_width_rad = CalcFilletWidthRad(aWidthRatio, m_connected_item_rad, m_trackseg_rad);
    m_length = CalcFilletLength(aLengthRatio);
    
    m_length_width_corr = m_connected_item_rad - (m_width_rad + m_trackseg_rad);
    m_connected_pos_length_delta += m_length_width_corr;
    m_length -= m_length_width_corr;
    
    CalcFilletSegs();
}

void TEARDROP::SetFilletPoints(void)
{
    double seg_A_pos_angle = m_fillet_seg_A_angle + M_PI_2;
    double seg_B_pos_angle = m_fillet_seg_B_angle - M_PI_2;
    uint seg_rad = m_trackseg_rad - m_poly_seg_rad;

    m_seg_points[1] = GetPoint(m_pos, seg_A_pos_angle, seg_rad);
    m_seg_points[2] = GetPoint(m_pos, seg_B_pos_angle, seg_rad);
    m_seg_points[0] = GetPoint(m_seg_points[1], m_fillet_seg_A_angle, m_fillet_seg_length);
    m_seg_points[3] = GetPoint(m_seg_points[2], m_fillet_seg_B_angle, m_fillet_seg_length);
    
    //A
    m_seg_outer_points[0] = GetPoint(m_seg_points[0], m_trackseg_angle_inv, m_poly_seg_rad);
    m_seg_outer_points[1] = GetPoint(m_seg_points[0], seg_A_pos_angle, m_poly_seg_rad);
    m_seg_outer_points[2] = GetPoint(m_seg_points[1], seg_A_pos_angle, m_poly_seg_rad);
    //B
    m_seg_outer_points[3] = GetPoint(m_seg_points[2], seg_B_pos_angle, m_poly_seg_rad);    
    m_seg_outer_points[4] = GetPoint(m_seg_points[3], seg_B_pos_angle, m_poly_seg_rad);
    m_seg_outer_points[5] = GetPoint(m_seg_points[3], m_trackseg_angle_inv, m_poly_seg_rad);

    //Clearance
    uint track_clearance_rad = m_poly_seg_rad + m_clearance;
    //A
    m_seg_clearance_points[0] = GetPoint(m_seg_points[0], m_trackseg_angle_inv, track_clearance_rad);
    m_seg_clearance_points[1] = GetPoint(m_seg_points[0], seg_A_pos_angle, track_clearance_rad);
    m_seg_clearance_points[2] = GetPoint(m_seg_points[1], seg_A_pos_angle, track_clearance_rad);
    //B
    m_seg_clearance_points[3] = GetPoint(m_seg_points[2], seg_B_pos_angle, track_clearance_rad); 
    m_seg_clearance_points[4] = GetPoint(m_seg_points[3], seg_B_pos_angle, track_clearance_rad);
    m_seg_clearance_points[5] = GetPoint(m_seg_points[3], m_trackseg_angle_inv, track_clearance_rad);
}

uint TEARDROP::CalcTeardropWidthRad(const int aWidthRatio, const uint aConnectedItemRad, const uint aTrackSegRad)
{
    if(aWidthRatio > 100)
        return (double(aConnectedItemRad) * (double(aWidthRatio) / 100.0)) - aTrackSegRad;
    else
        return double(aConnectedItemRad - aTrackSegRad) * (double(aWidthRatio) / 100.0);
}

void TEARDROP::CalcTeardropArcs(void)
{
    m_teardrop_segs_arc_rad = (pow(m_length, 2) / m_width_rad - m_width_rad) / 2;
    m_teardrop_segs_angle_add = atan(double(m_length) / double(m_teardrop_segs_arc_rad)) / double(m_num_arc_segs);
}

uint TEARDROP::CalcTeardropPosDist(const int aLengthRatio, const uint aConnectedItemRad)
{
    return aConnectedItemRad + double(aConnectedItemRad) * (double(aLengthRatio) / 100.0);
}

uint TEARDROP::CalcTeardropLength(const int aLengthRatio)
{
    uint max_dist_item_to_tear = CalcTeardropPosDist(aLengthRatio, m_connected_item_rad) + m_connected_pos_length_delta;
    uint dist_item_to_tear = std::min(max_dist_item_to_tear, m_trackseg_length);
    return dist_item_to_tear - m_connected_pos_length_delta;
}

void TEARDROP::CalcTeardrop(const int aWidthRatio, const int aLengthRatio)
{
    m_width_rad = CalcTeardropWidthRad(aWidthRatio, m_connected_item_rad, m_trackseg_rad);
    m_length = CalcTeardropLength(aLengthRatio);

    m_length_width_corr = m_connected_item_rad - (m_width_rad + m_trackseg_rad);
    m_connected_pos_length_delta += m_length_width_corr;
    m_length -= m_length_width_corr;

    CalcTeardropArcs();

    m_can_draw_clearance = (m_length > m_connected_item_rad + m_clearance);
}

void TEARDROP::SetTeardropPoints(void)
{
    m_teardrop_arc_A_center_pos = GetPoint(m_pos, m_trackseg_angle - M_PI_2, m_teardrop_segs_arc_rad);
    m_teardrop_arc_B_center_pos = GetPoint(m_pos, m_trackseg_angle + M_PI_2, m_teardrop_segs_arc_rad);

    double angle_tan_A = m_trackseg_angle + M_PI_2;
    double angle_tan_B = m_trackseg_angle - M_PI_2;     
    uint arc_outer_rad = m_teardrop_segs_arc_rad - m_trackseg_rad;
    uint arc_rad = arc_outer_rad + m_poly_seg_rad;

    //Clearance
    uint arc_clearance_rad = arc_outer_rad - m_clearance;
    uint track_clearance_rad = m_poly_seg_rad + m_clearance;

    int ppA{0};
    int ppB{0};
    int oc_ppA{0};
    int oc_ppB{0};
    
    for(int n = 0; n <= m_num_arc_segs; ++n)
    {
        ppA = m_num_arc_segs - n;
        ppB = m_num_arc_segs + n + 1;
        m_seg_points[ppA] = GetPoint(m_teardrop_arc_A_center_pos, angle_tan_A, arc_rad);
        m_seg_points[ppB] = GetPoint(m_teardrop_arc_B_center_pos, angle_tan_B, arc_rad);
        oc_ppA = ppA + 1;
        oc_ppB = ppB + 1;
        m_seg_outer_points[oc_ppA] = GetPoint(m_teardrop_arc_A_center_pos, angle_tan_A, arc_outer_rad);
        m_seg_outer_points[oc_ppB] = GetPoint(m_teardrop_arc_B_center_pos, angle_tan_B, arc_outer_rad);
        m_seg_clearance_points[oc_ppA] = GetPoint(m_teardrop_arc_A_center_pos, angle_tan_A, arc_clearance_rad);
        m_seg_clearance_points[oc_ppB] = GetPoint(m_teardrop_arc_B_center_pos, angle_tan_B, arc_clearance_rad);
        angle_tan_A += m_teardrop_segs_angle_add;
        angle_tan_B -= m_teardrop_segs_angle_add;
    }

    m_seg_outer_points[0] = GetPoint(m_seg_points[0], m_trackseg_angle_inv, m_poly_seg_rad);
    m_seg_outer_points[oc_ppB + 1] = GetPoint(m_seg_points[ppB], m_trackseg_angle_inv, m_poly_seg_rad);
    
    m_seg_clearance_points[0] = GetPoint(m_seg_points[0], m_trackseg_angle_inv, track_clearance_rad);
    m_seg_clearance_points[oc_ppB + 1] = GetPoint(m_seg_points[ppB], m_trackseg_angle_inv, track_clearance_rad);
}

void TEARDROP::TestTrackLength(void)
{
    switch(m_shape)
    {
        case FILLET_T:
            if(m_connected_item_rad + m_connected_pos_length_delta - m_length_width_corr - m_trackseg_rad >= m_trackseg_length)
                m_set_ok = false;
            break;
        case TEARDROP_T:
            if(m_connected_item_rad + m_connected_pos_length_delta - m_length_width_corr >= m_trackseg_length)
                m_set_ok = false;
            break;
        case SUBLAND_T:
            if(m_connected_item_rad - m_trackseg_rad >= m_trackseg_length)
                m_set_ok = false;
            break;
        default:;
    }
}

void TEARDROP::TestTrackWidth(void)
{
    if(m_connected_item_rad <= m_trackseg_rad)
        m_set_ok = false;
}

void TEARDROP::TestSize(void)
{
    if(m_poly_seg_width < MIN_POLYSEG_WIDTH)
        m_set_ok = false;
}

void TEARDROP::SetNotOKValues(void)
{
    m_length = m_trackseg_rad;
    m_connected_item_rad = m_trackseg_rad>>1;
    m_trackseg_rad = m_trackseg_rad>>3;
    (m_shape == SUBLAND_T)? m_length = m_connected_item_rad : m_length -= m_trackseg_rad;
    m_connected_pos_length_delta = 0;

    m_poly_seg_rad = m_trackseg_rad>>2;
    m_poly_seg_width = m_poly_seg_rad<<1;
    m_Width = m_poly_seg_width;
    m_width_rad = m_connected_item_rad - m_poly_seg_rad;
    
    CalcFilletSegs();
    CalcTeardropArcs();
    
    m_can_draw_clearance = false;
}

uint TEARDROP::GetBoundingRad(void) const
{
    if(m_set_ok)
    {
        if(m_shape == SUBLAND_T)
            return m_width_rad;
        else
            return m_length >> 1;
    }
    else
        return m_trackseg->GetWidth() >> 1;
}

uint TEARDROP::GetCalcLength(void) const
{
    switch(m_shape)
    {
        case SUBLAND_T:
            return const_cast<TEARDROP*>(this)->CalcSublandRad(m_width_ratio, m_connected_item_rad, m_trackseg_rad) + const_cast<TEARDROP*>(this)->CalcSublandPosDist(m_length_ratio, m_connected_item_rad, m_width_rad);
            break;
        case FILLET_T:
            return m_connected_pos_length_delta + const_cast<TEARDROP*>(this)->CalcFilletPosDist(m_length_ratio, m_connected_item_rad);
            break;
        case TEARDROP_T:
            return m_connected_pos_length_delta + const_cast<TEARDROP*>(this)->CalcTeardropPosDist(m_length_ratio, m_connected_item_rad);
        default:;
    }
    return 0;
}

uint TEARDROP::GetRealLength(void) const
{
    switch(m_shape)
    {
        case SUBLAND_T:
            return m_length + m_connected_pos_length_delta + m_width_rad;
        case FILLET_T:
        case TEARDROP_T:
            return m_length + m_connected_pos_length_delta + m_trackseg_rad;
        default:;
    }
    return 0;
}

uint TEARDROP::GetSizeLength(void) const
{
    if(m_shape != ZERO_T)
        return m_length;
    return 0;
}

wxString TEARDROP::GetConnectedItemName(void) const
{
    if(m_connected_item)
        return m_connected_item->GetClass();
    return wxT("xxx");
}


void TEARDROP::TransformShapeWithClearanceToPolygon( SHAPE_POLY_SET& aCornerBuffer,
                                               int             aClearanceValue,
                                               int             aCircleToSegmentsCount,
                                               double          aCorrectionFactor ) const
{
    if(IsSetOK())
    {
        switch(GetShape())
        {
            case FILLET_T:
            case TEARDROP_T:
            {
                if(!aClearanceValue)
                {
                    //was 3D for. Fills shape.
                    //No need, but let it be there.
                    aCornerBuffer.NewOutline();
                    for(wxPoint pos : m_seg_outer_points)
                    {
                        CPolyPt corner(pos);
                        aCornerBuffer.Append(corner);
                    }
                }
                else
                {
                    //Copper areas clearence for. 
                    for(uint n = 0; n < m_seg_points.size() - 1; ++n)
                    {
                        TransformRoundedEndsSegmentToPolygon( aCornerBuffer, m_seg_points[n], m_seg_points[n + 1], aCircleToSegmentsCount, m_poly_seg_width + ( 2 * aClearanceValue) );
                    }
                }
                break;
            }
            case SUBLAND_T:
            {
                int radius = m_width_rad + aClearanceValue;
                radius = KiROUND(radius * aCorrectionFactor);
                TransformCircleToPolygon( aCornerBuffer, m_pos, radius, aCircleToSegmentsCount );
            }
            break;

        }
    }
}

void TEARDROP::AddTo3DContainer(CBVHCONTAINER2D* aContainer, const double aBiuTo3Dunits)
{
    if(IsSetOK())
    {
        switch(GetShape())
        {
            case FILLET_T:
            case TEARDROP_T:
            {
                SHAPE_LINE_CHAIN chain;
                for(wxPoint pos : m_seg_outer_points)
                    chain.Append(pos);
                chain.SetClosed( true );

                SHAPE_POLY_SET polyList;
                polyList.AddOutline(chain);

                polyList.Simplify( SHAPE_POLY_SET::PM_FAST );
                polyList.Simplify( SHAPE_POLY_SET::PM_STRICTLY_SIMPLE );

                Convert_shape_line_polygon_to_triangles( polyList, *aContainer, aBiuTo3Dunits, *this);
            }
            break;
            case SUBLAND_T:
                aContainer->Add(new CFILLEDCIRCLE2D(SFVEC2F(m_pos.x * aBiuTo3Dunits, -m_pos.y * aBiuTo3Dunits), m_width_rad * aBiuTo3Dunits, *this));
            break;

        }
    }
}

void TEARDROP::SetParams(const PARAMS aParams)
{
    if(aParams.shape != ZERO_T)
    {
        m_shape = aParams.shape;
        m_length_ratio = aParams.length_ratio;
        m_width_ratio = aParams.width_ratio;
        switch(aParams.shape)    
        {
            case SUBLAND_T:
                if(m_length_ratio > SUBLAND_POS_MAX)
                    m_length_ratio = SUBLAND_POS_MAX;
                break;
            case FILLET_T:
                m_num_arc_segs = 1;
                m_seg_points.resize(FILLET_POLY_POINTS_NUM); 
                m_seg_outer_points.resize(FILLET_OUTER_POLY_POINTS_NUM);
                m_seg_clearance_points.resize(FILLET_CLEARANCE_POLY_POINTS_NUM);
                break;
            case TEARDROP_T:
                m_num_arc_segs = aParams.num_segments;
                if(m_num_arc_segs > TEARDROP_SEGS_MAX)
                    m_num_arc_segs = TEARDROP_SEGS_MAX;
                if(m_num_arc_segs < TEARDROP_SEGS_MIN)
                    m_num_arc_segs = TEARDROP_SEGS_MIN;      
                m_seg_points.resize(m_num_arc_segs * 2 + 2); 
                m_seg_outer_points.resize(m_num_arc_segs * 2 + 4); 
                m_seg_clearance_points.resize(m_num_arc_segs * 2 + 4); 
                break;
            default:;
        }
        
        if(m_length_ratio < TEARDROPS_POS_MIN)
            m_length_ratio = TEARDROPS_POS_MIN;
        if(m_width_ratio < TEARDROPS_SIZE_MIN)
            m_width_ratio = TEARDROPS_SIZE_MIN;
    }
    else
    {
        m_shape = aParams.shape;
        m_length_ratio = 0;
        m_width_ratio = 0;
    }
    
    Update();
}

TEARDROP::PARAMS TEARDROP::GetParams(void) const
{
    PARAMS returnParams;
    returnParams.shape = m_shape;
    returnParams.length_ratio = m_length_ratio;
    returnParams.width_ratio = m_width_ratio;
    returnParams.num_segments = m_num_arc_segs;
    
    return returnParams;
}


void TEARDROP::SetStart(const wxPoint& aStart)
{
    m_connected_pos = aStart;
    Update();
}

EDA_ITEM* TEARDROP::Clone() const
{
    return new TEARDROP( *this );
}

const EDA_RECT TEARDROP::GetBoundingBox() const
{
    int radius = const_cast<TEARDROP*>(this)->GetBoundingRad() + m_trackseg_rad;
    int ymax = m_mid_pos.y;
    int xmax = m_mid_pos.x;
    int ymin = m_mid_pos.y;
    int xmin = m_mid_pos.x;
    if( m_can_draw_clearance )
        radius += m_clearance + 1;
    ymax += radius;
    xmax += radius;
    ymin -= radius;
    xmin -= radius;
    EDA_RECT ret( wxPoint( xmin, ymin ), wxSize( xmax - xmin + 1, ymax - ymin + 1 ) );

    return ret;
}

wxString TEARDROP::GetShapeName(void) const
{
    switch(m_shape)
    {
        case TEARDROP_T:
            return _("TEARDROP");
        case FILLET_T:
            return _("FILLET");
        case SUBLAND_T:
            return _("SUBLAND");
        case ZERO_T:
            return _("ZERO");
        default:
            return _("NULL");
    }
}

wxString TEARDROP::GetSelectMenuText() const
{
    NETINFO_ITEM* net;
    BOARD* board = GetBoard();

    wxString text = GetClass();
    text << wxT(" ") << _(" [") << GetShapeName();
    if(m_shape != ZERO_T)
    {
        text << wxT(", ") << m_length_ratio;
        text << wxT(", ") << m_width_ratio;
        if(m_shape == TEARDROP_T)
            text << wxT(", ") << m_num_arc_segs;
    }
    text << wxT("]");

    (this->IsLocked())? text << _( " Locked" ) : text << _( " Unlocked" );

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

void TEARDROP::DrawItem(EDA_DRAW_PANEL* aPanel, wxDC* aDC, const COLOR4D aColor, const wxPoint& aOffset, const DISPLAY_OPTIONS* aDisplOpts)
{
    if(!m_trackseg)
        return;
    
    EDA_RECT* clip_box = aPanel->GetClipBox();
    bool showClearance = (IsCopperLayer(m_trackseg->GetLayer()) && ((aDisplOpts->m_ShowTrackClearanceMode == SHOW_CLEARANCE_NEW_AND_EDITED_TRACKS_AND_VIA_AREAS && ( m_trackseg->IsDragging() || m_trackseg->IsMoving() || m_trackseg->IsNew() ) ) || ( aDisplOpts->m_ShowTrackClearanceMode == SHOW_CLEARANCE_ALWAYS ))) && m_can_draw_clearance;
    
    switch(m_shape)
    {
        case SUBLAND_T:
            if(!aDisplOpts->m_DisplayPcbTrackFill || GetState( FORCE_SKETCH ) || m_draw_mode_unfill)
                GRCircle(clip_box, aDC, m_pos.x + aOffset.x, m_pos.y + aOffset.y, m_width_rad, aColor);
            else
                GRFilledCircle(clip_box, aDC, m_pos + aOffset, m_width_rad, aColor);
            if(showClearance)
                GRCircle(clip_box, aDC, m_pos.x + aOffset.x, m_pos.y + aOffset.y, m_width_rad + m_clearance, aColor);
            break;
        case FILLET_T:
            if(!aDisplOpts->m_DisplayPcbTrackFill || GetState( FORCE_SKETCH ) || m_draw_mode_unfill)
            {
                GRArc1(clip_box, aDC, m_seg_outer_points[0] + aOffset, m_seg_outer_points[1] + aOffset, m_seg_points[0] + aOffset, 0, aColor);
                GRLine(clip_box, aDC, m_seg_outer_points[2] + aOffset, m_seg_outer_points[1] + aOffset, 2, aColor);
                GRArc1(clip_box, aDC, m_seg_outer_points[2] + aOffset, m_seg_outer_points[3] + aOffset, m_pos + aOffset, 0, aColor);
                GRLine(clip_box, aDC, m_seg_outer_points[3] + aOffset, m_seg_outer_points[4] + aOffset, 2, aColor);
                GRArc1(clip_box, aDC, m_seg_outer_points[4] + aOffset, m_seg_outer_points[5] + aOffset, m_seg_points[3] + aOffset, 0, aColor);
                GRLine(clip_box, aDC, m_seg_outer_points[5] + aOffset, m_seg_outer_points[0] + aOffset, 2, aColor);
            }
            else
                GRClosedPoly(clip_box, aDC, m_seg_points.size(), &m_seg_points[0], true, m_poly_seg_width, aColor, aColor);
            
            if(showClearance)
            {
                GRArc1(clip_box, aDC, m_seg_clearance_points[0] + aOffset, m_seg_clearance_points[1] + aOffset, m_seg_points[0] + aOffset, 0, aColor);
                GRLine(clip_box, aDC, m_seg_clearance_points[2] + aOffset, m_seg_clearance_points[1] + aOffset, 2, aColor);
                GRArc1(clip_box, aDC, m_seg_clearance_points[2] + aOffset, m_seg_clearance_points[3] + aOffset, m_pos + aOffset, 0, aColor);
                GRLine(clip_box, aDC, m_seg_clearance_points[3] + aOffset, m_seg_clearance_points[4] + aOffset, 2, aColor);
                GRArc1(clip_box, aDC, m_seg_clearance_points[4] + aOffset, m_seg_clearance_points[5] + aOffset, m_seg_points[3] + aOffset, 0, aColor);
                GRLine(clip_box, aDC, m_seg_clearance_points[5] + aOffset, m_seg_clearance_points[0] + aOffset, 2, aColor);
            }
            break;
        case TEARDROP_T:
        {
            int seg_last = int(m_seg_points.size() - 1);
            int seg_outer_last = int(m_seg_outer_points.size() - 1);
            int seg_outer_cusp_A = m_num_arc_segs + 1;
            int seg_outer_cusp_B = m_num_arc_segs + 2;
            if(!aDisplOpts->m_DisplayPcbTrackFill || GetState( FORCE_SKETCH ) || m_draw_mode_unfill)
            {
                for(int n = 1; n <= m_num_arc_segs; ++n)
                {
                    GRLine(clip_box, aDC, m_seg_outer_points[n] + aOffset, m_seg_outer_points[n + 1] + aOffset, 2, aColor);
                    GRLine(clip_box, aDC, m_seg_outer_points[seg_outer_last - n] + aOffset, m_seg_outer_points[seg_outer_last - n - 1] + aOffset, 2, aColor);
                }
                GRArc1(clip_box, aDC, m_seg_outer_points[0] + aOffset, m_seg_outer_points[1] + aOffset, m_seg_points[0] + aOffset, 0, aColor);
                GRArc1(clip_box, aDC, m_seg_outer_points[seg_outer_cusp_A] + aOffset, m_seg_outer_points[seg_outer_cusp_B] + aOffset, m_pos + aOffset, 0, aColor);
                GRArc1(clip_box, aDC, m_seg_outer_points[seg_outer_last - 1] + aOffset, m_seg_outer_points[seg_outer_last] + aOffset, m_seg_points[seg_last] + aOffset, 0, aColor);
                GRLine(clip_box, aDC, m_seg_outer_points[seg_outer_last] + aOffset, m_seg_outer_points[0] + aOffset, 2, aColor);
            }
            else
                GRClosedPoly(clip_box, aDC, m_seg_points.size(), &m_seg_points[0], true, m_poly_seg_width, aColor, aColor);  
            
            if(showClearance)
            {
                for(int n = 1; n <= m_num_arc_segs; ++n)
                {
                    GRLine(clip_box, aDC, m_seg_clearance_points[n] + aOffset, m_seg_clearance_points[n + 1] + aOffset, 2, aColor);
                    GRLine(clip_box, aDC, m_seg_clearance_points[seg_outer_last - n] + aOffset, m_seg_clearance_points[seg_outer_last - n - 1] + aOffset, 2, aColor);
                }
                GRArc1(clip_box, aDC, m_seg_clearance_points[0].x + aOffset.x, m_seg_clearance_points[0].y + aOffset.y, m_seg_clearance_points[1].x + aOffset.x, m_seg_clearance_points[1].y + aOffset.y, m_seg_points[0].x + aOffset.x, m_seg_points[0].y + aOffset.y, aColor);
                GRArc1(clip_box, aDC, m_seg_clearance_points[seg_outer_cusp_A].x + aOffset.x, m_seg_clearance_points[seg_outer_cusp_A].y + aOffset.y, m_seg_clearance_points[seg_outer_cusp_B].x + aOffset.x, m_seg_clearance_points[seg_outer_cusp_B].y + aOffset.y, m_pos.x + aOffset.x, m_pos.y + aOffset.y, aColor);
                GRArc1(clip_box, aDC, m_seg_clearance_points[seg_outer_last - 1].x + aOffset.x, m_seg_clearance_points[seg_outer_last - 1].y + aOffset.y, m_seg_clearance_points[seg_outer_last].x + aOffset.x, m_seg_clearance_points[seg_outer_last].y + aOffset.y, m_seg_points[seg_last].x + aOffset.x, m_seg_points[seg_last].y + aOffset.y, aColor); // )
                GRLine(clip_box, aDC, m_seg_clearance_points[seg_outer_last] + aOffset, m_seg_clearance_points[0] + aOffset, 2, aColor);
            }
            break;
        }
        case ZERO_T:
            if(!aDisplOpts->m_DisplayPcbTrackFill || GetState( FORCE_SKETCH ) || m_draw_mode_unfill)
            {
                GRCircle(clip_box, aDC, m_pos.x + aOffset.x, m_pos.y + aOffset.y, m_width_rad, aColor);
                GRLine(clip_box, aDC, m_pos + aOffset, m_mid_pos + aOffset, 2, aColor);
            }
            break;
        default:;
    }
}

void TEARDROP::DrawItem(KIGFX::GAL* aGal, const bool aIsSketch )
{
    if(!m_trackseg)
        return;
 
    std::deque<VECTOR2D> points;
    points.clear();

    switch(m_shape)
    {
        case SUBLAND_T:
                aGal->DrawCircle( VECTOR2D(m_pos) , m_width_rad );
            break;
        case FILLET_T:
            if(aIsSketch)
            {
                DrawArc(aGal, m_seg_points[0], m_seg_outer_points[0], m_seg_outer_points[1], m_poly_seg_rad);
                DrawArc(aGal, m_seg_points[3], m_seg_outer_points[4], m_seg_outer_points[5], m_poly_seg_rad);
                DrawArc(aGal, m_pos, m_seg_outer_points[2], m_seg_outer_points[3], m_trackseg_rad);
                aGal->DrawLine(VECTOR2D(m_seg_outer_points[1]), VECTOR2D(m_seg_outer_points[2]));
                aGal->DrawLine(VECTOR2D(m_seg_outer_points[3]), VECTOR2D(m_seg_outer_points[4]));
                aGal->DrawLine(VECTOR2D(m_seg_outer_points[5]), VECTOR2D(m_seg_outer_points[0]));
            }
            else
            {
                for(auto& point : m_seg_points)
                    points.push_back(point);
                points.push_back(m_seg_points[0]);
                aGal->SetLineWidth(m_poly_seg_width);
                aGal->DrawPolygon(points);
                aGal->DrawPolyline(points);
            }
            break;
        case TEARDROP_T:
        {
            int seg_last = int(m_seg_points.size() - 1);
            int seg_outer_last = int(m_seg_outer_points.size() - 1);
            int seg_outer_cusp_A = m_num_arc_segs + 1;
            int seg_outer_cusp_B = m_num_arc_segs + 2;
            if(aIsSketch)
            {
                DrawArc(aGal, m_seg_points[0], m_seg_outer_points[0], m_seg_outer_points[1], m_poly_seg_rad);
                DrawArc(aGal, m_seg_points[seg_last], m_seg_outer_points[seg_outer_last-1], m_seg_outer_points[seg_outer_last], m_poly_seg_rad);
                DrawArc(aGal, m_pos, m_seg_outer_points[seg_outer_cusp_A], m_seg_outer_points[seg_outer_cusp_B], m_trackseg_rad);
                for(int n = 1; n <= m_num_arc_segs; ++n)
                {
                    aGal->DrawLine(VECTOR2D(m_seg_outer_points[n]), VECTOR2D(m_seg_outer_points[n+1]));
                    aGal->DrawLine(VECTOR2D(m_seg_outer_points[seg_outer_last - n]), VECTOR2D(m_seg_outer_points[seg_outer_last - n - 1]));
                }
                aGal->DrawLine(VECTOR2D(m_seg_outer_points[seg_outer_last]), VECTOR2D(m_seg_outer_points[0]));
            }
            else
            {
                for(auto& point : m_seg_points)
                    points.push_back(point);
                points.push_back(m_seg_points[0]);
                aGal->SetLineWidth(m_poly_seg_width);
                aGal->DrawPolygon(points);
                aGal->DrawPolyline(points);
            }
            break;
        }
        case ZERO_T:
            if(aIsSketch)
            {
                aGal->DrawCircle(VECTOR2D(m_pos) , m_width_rad);
                aGal->DrawLine(VECTOR2D(m_pos), VECTOR2D(m_mid_pos));
            }
            break;
        default:;
    }
}

//-----------------------------------------------------------------------------------------------------/
// VIA TEARDROP 
//-----------------------------------------------------------------------------------------------------/
TEARDROP_VIA::TEARDROP_VIA(const BOARD_ITEM* aParent, const VIA* aVia, const TRACK* aTrackSeg, const PARAMS aParams, const bool aCheckNullTrack) : TEARDROP(aParent, aTrackSeg)
{
    if(aVia)
    {
        m_connected_via = const_cast<VIA*>(aVia);
        m_connected_item = m_connected_via;
        m_connected_pos = m_connected_via->GetStart();
        
        CallConstructor(aTrackSeg, aParams, aCheckNullTrack);
    }
}

void TEARDROP_VIA::SetConnectedItem(void)
{
    if(m_connected_via)
    {
        m_connected_item_rad = m_connected_via->GetWidth()>>1;
    }
}

void TEARDROP_VIA::SetViaRad(const uint aNewViaRad)
{
    m_connected_item_rad = aNewViaRad;
}

TEARDROP_EDIT_VIA::TEARDROP_EDIT_VIA(const BOARD_ITEM* aParent, const uint aViaRadius, const TRACK* aTrackSeg, const PARAMS aParams, const bool aCheckNullTrack) : TEARDROP_VIA(aParent, nullptr, aTrackSeg, aParams, aCheckNullTrack) 
{
    m_connected_via = nullptr;
    m_connected_item_rad = aViaRadius;
    if(aTrackSeg)
        m_connected_pos = aTrackSeg->GetEnd();
    
    CallConstructor(aTrackSeg, aParams, aCheckNullTrack);
}

bool TEARDROP_EDIT_VIA::ChangeTrack(const TRACK* aNewTrack)
{
    if(aNewTrack && (aNewTrack->Type() == PCB_TRACE_T))
    {
        m_connected_pos = aNewTrack->GetEnd();
        return SetTrackSeg(aNewTrack, true);
    }
    return false;
}


//-----------------------------------------------------------------------------------------------------/
// PAD TEARDROP 
//-----------------------------------------------------------------------------------------------------/
//Rectangular shape teardrop. Edge is stright.
TEARDROP_RECT::TEARDROP_RECT(const BOARD_ITEM* aParent, const TRACK* aTrackSeg) : TEARDROP(aParent, aTrackSeg)
{
    m_rect_type = false;
}   

        
void TEARDROP_RECT::SetConnectedItem(void)
{
    m_rect_type = false;
}

void TEARDROP_RECT::CalcSegment(void)
{
    m_poly_seg_rad = m_trackseg_rad>>2; 
    uint sub_rad = m_connected_item_rad - m_trackseg_rad;
    if(m_trackseg_rad > sub_rad)
        m_poly_seg_rad = sub_rad>>2; 
    m_poly_seg_width = m_poly_seg_rad<<1;
}

void TEARDROP_RECT::SetNotOKValues(void)
{
    TEARDROP::SetNotOKValues();
    m_rect_edge_dist_x = m_length;
    m_rect_type = false;
}

void TEARDROP_RECT::CalcFillet(const int aWidthRatio, const int aLengthRatio)
{
    if(m_rect_type)
    {
        m_width_rad = CalcFilletWidthRad(aWidthRatio, m_connected_item_rad, m_trackseg_rad);
        
        uint length_corr = 9 * m_trackseg_rad / 10;
        uint max_dist_item_to_tear = CalcFilletPosDist(aLengthRatio, m_connected_item_rad) + m_connected_pos_length_delta;
        uint dist_item_to_tear = std::min(max_dist_item_to_tear, m_trackseg_length + length_corr);
        m_length = dist_item_to_tear - m_connected_pos_length_delta;
        CalcFilletSegs();
        m_length -= length_corr;

        CalcSegment();
        
        m_rect_edge_dist_x = m_length - m_connected_item_rad;
        int max_rect_edge_dist_x = max_dist_item_to_tear - m_connected_pos_length_delta - m_connected_item_rad;
        if(m_rect_edge_dist_x + m_trackseg_rad < max_rect_edge_dist_x)
            m_width_rad = double(m_width_rad) * (double(m_rect_edge_dist_x + m_trackseg_rad) / double(max_rect_edge_dist_x));
        
        double seg_angle;
        if(m_rect_edge_dist_x > 0)
        {
            uint dw_op_cha = m_width_rad + m_trackseg_rad - m_poly_seg_rad;
            uint dw_ne_cha = m_rect_edge_dist_x + m_poly_seg_rad;
            double dw_seg_angle = atan((double(dw_op_cha)) / double(dw_ne_cha));
            uint com_hypot = (double(dw_op_cha) / sin(dw_seg_angle));
            uint up_op_cha = m_trackseg_rad - m_poly_seg_rad;
            double up_seg_angle = asin(double(up_op_cha) / double(com_hypot));
            seg_angle = dw_seg_angle - up_seg_angle; //M_PI_2 - (M_PI_2 - dw_seg_angle + up_seg_angle);
        }
        else
            seg_angle = atan( double(m_trackseg_rad + m_width_rad) / double(m_rect_edge_dist_x + m_trackseg_rad));
            
        m_fillet_seg_A_angle = m_trackseg_angle_inv + seg_angle;
        m_fillet_seg_B_angle = m_trackseg_angle_inv - seg_angle;
    }
    else
        TEARDROP::CalcFillet(aWidthRatio, aLengthRatio);
}

void TEARDROP_RECT::SetFilletPoints(void)
{
    if(m_rect_type)
    {
        double seg_A_pos_angle = m_fillet_seg_A_angle + M_PI_2;
        double seg_B_pos_angle = m_fillet_seg_B_angle - M_PI_2;
        uint seg_rad = m_trackseg_rad - m_poly_seg_rad;
        
        m_seg_points[1] = GetPoint(m_pos, seg_A_pos_angle, seg_rad);
        m_seg_points[2] = GetPoint(m_pos, seg_B_pos_angle, seg_rad);
        wxPoint hpos = GetPoint(m_pos, m_trackseg_angle_inv, m_rect_edge_dist_x + m_poly_seg_rad);
        m_seg_points[0] = GetPoint(hpos, m_trackseg_angle_inv + M_PI_2, m_width_rad + seg_rad);
        m_seg_points[3] = GetPoint(hpos, m_trackseg_angle_inv - M_PI_2, m_width_rad + seg_rad);
        
        //A
        m_seg_outer_points[0] = GetPoint(m_seg_points[0], m_trackseg_angle_inv, m_poly_seg_rad);
        m_seg_outer_points[1] = GetPoint(m_seg_points[0], seg_A_pos_angle, m_poly_seg_rad);
        m_seg_outer_points[2] = GetPoint(m_seg_points[1], seg_A_pos_angle, m_poly_seg_rad);
        //B
        m_seg_outer_points[3] = GetPoint(m_seg_points[2], seg_B_pos_angle, m_poly_seg_rad);    
        m_seg_outer_points[4] = GetPoint(m_seg_points[3], seg_B_pos_angle, m_poly_seg_rad);
        m_seg_outer_points[5] = GetPoint(m_seg_points[3], m_trackseg_angle_inv, m_poly_seg_rad);

        //Clearance
        uint track_clearance_rad = m_poly_seg_rad + m_clearance;
        //A
        m_seg_clearance_points[0] = GetPoint(m_seg_points[0], m_trackseg_angle_inv, track_clearance_rad);
        m_seg_clearance_points[1] = GetPoint(m_seg_points[0], seg_A_pos_angle, track_clearance_rad);
        m_seg_clearance_points[2] = GetPoint(m_seg_points[1], seg_A_pos_angle, track_clearance_rad);
        //B
        m_seg_clearance_points[3] = GetPoint(m_seg_points[2], seg_B_pos_angle, track_clearance_rad); 
        m_seg_clearance_points[4] = GetPoint(m_seg_points[3], seg_B_pos_angle, track_clearance_rad);
        m_seg_clearance_points[5] = GetPoint(m_seg_points[3], m_trackseg_angle_inv, track_clearance_rad);
    }
    else
        TEARDROP::SetFilletPoints();
}

void TEARDROP_RECT::CalcTeardrop(const int aWidthRatio, const int aLengthRatio)
{
    if(m_rect_type)
    {
        m_width_rad = CalcTeardropWidthRad(aWidthRatio, m_connected_item_rad, m_trackseg_rad);
        
        uint length_corr = 4 * m_trackseg_rad / 5;
        uint max_dist_item_to_tear = CalcTeardropPosDist(aLengthRatio, m_connected_item_rad) + m_connected_pos_length_delta;
        uint dist_item_to_tear = std::min(max_dist_item_to_tear, m_trackseg_length + length_corr - m_trackseg_rad);
        m_length = dist_item_to_tear - m_connected_pos_length_delta + m_trackseg_rad;
        m_length -= length_corr;

        CalcTeardropArcs();
        CalcSegment();

        m_width_rad += m_trackseg_rad - m_poly_seg_rad;

        m_rect_edge_dist_x = m_length - m_connected_item_rad + m_poly_seg_rad;
        
        if(m_rect_edge_dist_x < m_width_rad - m_trackseg_rad)
            m_width_rad = m_trackseg_rad + m_rect_edge_dist_x;

        m_teardrop_segs_arc_rad = m_width_rad;
        m_teardrop_segs_angle_add = M_PI_2 / double(m_num_arc_segs);

        m_can_draw_clearance = (m_rect_edge_dist_x >= m_poly_seg_rad + m_clearance);
    }
    else
        TEARDROP::CalcTeardrop(aWidthRatio, aLengthRatio);
}

void TEARDROP_RECT::SetTeardropPoints(void)
{
    if(m_rect_type)
    {
        m_teardrop_arc_A_center_pos = GetPoint(m_pos, m_trackseg_angle - M_PI_2, m_teardrop_segs_arc_rad);
        m_teardrop_arc_B_center_pos = GetPoint(m_pos, m_trackseg_angle + M_PI_2, m_teardrop_segs_arc_rad);

        double angle_tan_A = m_trackseg_angle + M_PI_2;
        double angle_tan_B = m_trackseg_angle - M_PI_2;     

        //Clearance
        uint track_clearance_rad = m_poly_seg_rad + m_clearance;

        int arc_rad = m_teardrop_segs_arc_rad - m_trackseg_rad + m_poly_seg_rad;
        int arc_x_move_dist = (int(m_rect_edge_dist_x) - arc_rad) / int(m_num_arc_segs);
        double outer_angle_A = angle_tan_A + M_PI;
        double outer_angle_B = angle_tan_B + M_PI;
        double prev_outer_angle_A = outer_angle_A;
        double prev_outer_angle_B = outer_angle_B;

        int ppA{0};
        int ppB{0};
        int oc_ppA{0};
        int oc_ppB{0};
        for(int n = 0; n <= m_num_arc_segs; ++n)
        {
            ppA = m_num_arc_segs - n;
            ppB = m_num_arc_segs + n + 1;
            m_seg_points[ppA] = GetPoint(m_teardrop_arc_A_center_pos, angle_tan_A, arc_rad);
            m_seg_points[ppB] = GetPoint(m_teardrop_arc_B_center_pos, angle_tan_B, arc_rad);

            if(n)
            {
                if(n > 1)
                {
                    double a1 = AngleRad(m_seg_points[ppA + 2], m_seg_points[ppA + 1]);
                    double a2 = AngleRad(m_seg_points[ppA + 1], m_seg_points[ppA]);
                    a1 += a2;
                    outer_angle_A = a1 / 2.0 + M_PI_2;
                    outer_angle_A = NormAngle(outer_angle_A, prev_outer_angle_A - M_PI_2, prev_outer_angle_A + M_PI_2, M_PI);
                    a1 = AngleRad(m_seg_points[ppB - 2], m_seg_points[ppB - 1]);
                    a2 = AngleRad(m_seg_points[ppB - 1], m_seg_points[ppB]);
                    a1 += a2;
                    outer_angle_B = a1 / 2.0 - M_PI_2;
                    outer_angle_B = NormAngle(outer_angle_B, prev_outer_angle_B - M_PI_2, prev_outer_angle_B + M_PI_2, M_PI);
                    
                    prev_outer_angle_A = outer_angle_A;
                    prev_outer_angle_B = outer_angle_B;
                }
                m_seg_outer_points[oc_ppA] = GetPoint(m_seg_points[ppA + 1], outer_angle_A, m_poly_seg_rad);
                m_seg_outer_points[oc_ppB] = GetPoint(m_seg_points[ppB - 1], outer_angle_B, m_poly_seg_rad);
                m_seg_clearance_points[oc_ppA] = GetPoint(m_seg_points[ppA + 1], outer_angle_A, track_clearance_rad);
                m_seg_clearance_points[oc_ppB] = GetPoint(m_seg_points[ppB - 1], outer_angle_B, track_clearance_rad);
            }
            
            oc_ppA = ppA + 1;
            oc_ppB = ppB + 1;

            angle_tan_A += m_teardrop_segs_angle_add;
            angle_tan_B -= m_teardrop_segs_angle_add;
            m_teardrop_arc_A_center_pos = GetPoint(m_teardrop_arc_A_center_pos, m_trackseg_angle_inv, arc_x_move_dist);
            m_teardrop_arc_B_center_pos = GetPoint(m_teardrop_arc_B_center_pos, m_trackseg_angle_inv, arc_x_move_dist);
        }

        outer_angle_A = AngleRad(m_seg_points[ppA + 1], m_seg_points[ppA]) + M_PI_2;
        outer_angle_B = AngleRad(m_seg_points[ppB - 1], m_seg_points[ppB]) - M_PI_2;

        m_seg_outer_points[oc_ppA] = GetPoint(m_seg_points[ppA], outer_angle_A, m_poly_seg_rad);
        m_seg_outer_points[oc_ppB] = GetPoint(m_seg_points[ppB], outer_angle_B, m_poly_seg_rad);
        m_seg_outer_points[0] = GetPoint(m_seg_points[0], m_trackseg_angle_inv, m_poly_seg_rad);
        m_seg_outer_points[oc_ppB + 1] = GetPoint(m_seg_points[ppB], m_trackseg_angle_inv, m_poly_seg_rad);
    
        m_seg_clearance_points[oc_ppA] = GetPoint(m_seg_points[ppA], outer_angle_A, track_clearance_rad);
        m_seg_clearance_points[oc_ppB] = GetPoint(m_seg_points[ppB], outer_angle_B, track_clearance_rad);
        m_seg_clearance_points[0] = GetPoint(m_seg_points[0], m_trackseg_angle_inv, track_clearance_rad);
        m_seg_clearance_points[oc_ppB + 1] = GetPoint(m_seg_points[ppB], m_trackseg_angle_inv, track_clearance_rad);
        
    }
    else
    {
        TEARDROP::SetTeardropPoints();        
    }
}


//Pad all things

TEARDROP_PAD::TEARDROP_PAD(const BOARD_ITEM* aParent, const D_PAD* aPad, const TRACK* aTrackSeg, const PARAMS aParams, const bool aCheckNullTrack) : TEARDROP_RECT(aParent, aTrackSeg)
{
    if(aPad)
    {
        m_connected_pad = const_cast<D_PAD*>(aPad);
        m_connected_item = m_connected_pad;
        m_connected_pos = m_connected_pad->GetPosition();
    
        CallConstructor(aTrackSeg, aParams, aCheckNullTrack);
    }
}

void TEARDROP_PAD::SetConnectedItem(void)
{
    TEARDROP_RECT::SetConnectedItem();
    if(m_connected_pad)
    {
        PAD_SHAPE_T pad_shape = m_connected_pad->GetShape();
        int pad_width = m_connected_pad->GetSize().GetWidth();
        int pad_height = m_connected_pad->GetSize().GetHeight();
        uint max_rad = std::max(pad_width, pad_height)>>1;
        uint min_rad = std::min(pad_width, pad_height)>>1;
        m_connected_item_rad = min_rad;
        double radius_ratio = 0.0;

        if(pad_shape != PAD_SHAPE_CIRCLE)
        {
            int pad_angle = round(m_connected_pad->GetOrientation() / 10.0) * 10.0;
            
            int track_deg_angle = Rad2DeciDegRnd(m_trackseg_angle);
            int track_90_angle = track_deg_angle + ANGLE_90;
            NORMALIZE_ANGLE_POS(track_90_angle);
            int track_180_angle = track_deg_angle + ANGLE_180;
            NORMALIZE_ANGLE_POS(track_180_angle);
            int track_270_angle = track_deg_angle + ANGLE_270;
            NORMALIZE_ANGLE_POS(track_270_angle);
            
            bool hrz_angle = (track_deg_angle == pad_angle) || (track_180_angle == pad_angle);
            bool vrt_angle = (track_90_angle == pad_angle) || (track_270_angle == pad_angle);
            
            if(hrz_angle || vrt_angle)
            {
                bool hrz_cmp = (pad_width > pad_height) && hrz_angle;
                bool vrt_cmp = (pad_width < pad_height) && vrt_angle;
                if( hrz_cmp || vrt_cmp)
                    m_connected_pos_length_delta = max_rad - m_connected_item_rad;
                
                switch(pad_shape)
                {
                    case PAD_SHAPE_OVAL:
                        //Longest sideline.
                        if(!m_connected_pos_length_delta)
                        {
                            m_rect_type = true;
                            if(GetShape() != SUBLAND_T && GetShape() != ZERO_T)
                            {
                                if(max_rad - min_rad <= m_trackseg_rad)
                                {
                                    m_connected_item_rad = max_rad;
                                    m_connected_pos_length_delta -= (max_rad - min_rad)>>1;
                                    if(m_length_ratio < 100)
                                    {
                                        int l = (max_rad - min_rad) / m_length_ratio;
                                        m_connected_pos_length_delta -= l;
                                    }
                                    m_rect_type = false;
                                }
                                else
                                {
                                    CalcSegment();
                                    m_connected_item_rad = max_rad - min_rad + (m_poly_seg_rad<<1);
                                    m_connected_pos_length_delta += min_rad - m_connected_item_rad;
                                }
                            }
                        }
                        break;
                    case PAD_SHAPE_ROUNDRECT:
                        //TODO
                        radius_ratio = m_connected_pad->GetRoundRectRadiusRatio();
                        //Longest sideline.
                        if(!m_connected_pos_length_delta) 
                        {
                            m_rect_type = true;
                            uint max_half = max_rad>>1;
                            if(max_half > min_rad)
                            {
                                m_connected_item_rad = max_half;
                                m_connected_pos_length_delta -= max_half - min_rad;
                            }
                            else
                            {
                                m_connected_item_rad = max_rad;
                                m_connected_pos_length_delta -= max_rad - min_rad;
                            }
                            m_connected_item_rad -= m_connected_item_rad * radius_ratio;
                        }
                        //if(radius_ratio)
                        //    m_connected_pos_length_delta -= m_connected_item_rad * radius_ratio;
                        break;
                    case PAD_SHAPE_RECT:
                        m_rect_type = true;
                        //Longest sideline.
                        if(!m_connected_pos_length_delta) 
                        {
                            uint max_half = max_rad>>1;
                            if(max_half > min_rad)
                            {
                                m_connected_item_rad = max_half;
                                m_connected_pos_length_delta -= max_half - min_rad;
                            }
                            else
                            {
                                m_connected_item_rad = max_rad;
                                m_connected_pos_length_delta -= max_rad - min_rad;
                            }
                        }
                        break;
                    case PAD_SHAPE_TRAPEZOID:
                    {
                        wxSize delta_size = m_connected_pad->GetDelta();
                        if(m_connected_pos_length_delta)
                        {
                            if(delta_size.y && vrt_cmp)
                            {
                                m_rect_type = true;

                                if(((track_90_angle == pad_angle) && (delta_size.y > 0)) || ((track_270_angle == pad_angle) && (delta_size.y < 0)))
                                    m_connected_item_rad = pad_width>>1;
                                else
                                    m_connected_item_rad = (pad_width - abs(delta_size.y))>>1;
                                
                                m_connected_pos_length_delta = max_rad - m_connected_item_rad;
                            }
                            else
                                if(delta_size.x && hrz_cmp)
                                {
                                    m_rect_type = true;
                                    
                                    if(((track_180_angle == pad_angle) && (delta_size.x > 0)) || ((track_deg_angle == pad_angle) && (delta_size.x < 0)))
                                        m_connected_item_rad = pad_height>>1;
                                    else
                                        m_connected_item_rad = (pad_height - abs(delta_size.x))>>1;

                                    m_connected_pos_length_delta = max_rad - m_connected_item_rad;
                                }
                                else
                                    m_connected_pos_length_delta = 0;
                        }
                        else
                        {
                            if(delta_size.x && hrz_angle)
                            {
                                m_rect_type = true;
                                
                                if(((track_180_angle == pad_angle) && (delta_size.x > 0)) || ((track_deg_angle == pad_angle) && (delta_size.x < 0)))
                                    m_connected_item_rad = pad_height>>1;
                                else
                                    m_connected_item_rad = (pad_height - abs(delta_size.x))>>1;
                                
                                m_connected_pos_length_delta = min_rad - m_connected_item_rad;
                            }

                            if(delta_size.y && vrt_angle)
                            {
                                m_rect_type = true;

                                if(((track_90_angle == pad_angle) && (delta_size.y > 0)) || ((track_270_angle == pad_angle) && (delta_size.y < 0)))
                                    m_connected_item_rad = pad_width>>1;
                                else
                                    m_connected_item_rad = (pad_width - abs(delta_size.y))>>1;

                                m_connected_pos_length_delta = min_rad - m_connected_item_rad;
                            }
                        }
                        break;
                    }
                    default:;
                }
            }
        }
    }
}

bool TEARDROP_PAD::IsAngleOK(void) const
{
    if(m_connected_pad)
    {
        if(IsSetOK())
            switch(m_connected_pad->GetShape())
            {   //Do not even think about breaks!
                case PAD_SHAPE_OVAL:
                    if(m_connected_pos_length_delta)
                        return true;
                case PAD_SHAPE_RECT:
                    if(m_connected_pad->GetSize().GetHeight() == m_connected_pad->GetSize().GetWidth())
                        return true;
                case PAD_SHAPE_TRAPEZOID:
                    return m_rect_type;
                default:
                    return true;
            }
    }
    return true;
}

void TEARDROP_PAD::ChangePad(const D_PAD* aPad)
{
    if(aPad && (aPad->Type() == PCB_PAD_T))
    {
        if((aPad != m_connected_pad) && (aPad->GetPosition() == GetEnd()))
        {
            D_PAD* oldPad = m_connected_pad;
            m_connected_pad = const_cast<D_PAD*>(aPad);
            m_connected_item = m_connected_pad;
            m_connected_pos = m_connected_pad->GetPosition();
        
            if(!CallConstructor(GetTrackSeg(), GetParams(), false))
            {
                m_connected_pad = oldPad;
                m_connected_item = m_connected_pad;
                m_connected_pos = m_connected_pad->GetPosition();
            }
        }
    }
}


//-----------------------------------------------------------------------------------------------------/
// JUNCTION TEARDROPS
//-----------------------------------------------------------------------------------------------------/
TEARDROP_JUNCTIONS::TEARDROP_JUNCTIONS(const BOARD_ITEM* aParent, const TRACK* aTrackSeg, const wxPoint aPosition, const PARAMS aParams, const bool aCheckNullTrack) : TEARDROP_RECT(aParent, aTrackSeg)
{
    if(aTrackSeg && ((aTrackSeg->GetStart() == aPosition) || (aTrackSeg->GetEnd() == aPosition)))
    {
        m_connected_pos = aPosition;
        CollectTracks(aTrackSeg, m_connected_pos);
        m_connected_item_rad = uint(aTrackSeg->GetWidth());
        CallConstructor(aTrackSeg, aParams, aCheckNullTrack);
    }
}

void TEARDROP_JUNCTIONS::SetConnectedItem(void)
{
    m_connected_item_rad = m_Width;
    TEARDROP_RECT::SetConnectedItem();

    m_T_tracks = false;
    Find_T_FromTracks();
    if(m_T_track_next && m_T_track_back) //Tracks are at T.
        m_T_tracks = true;

    //If any of connected track is wider than this track. Make it's rad to item rad. But not in T.
    int max_width = MaxWidthFromTracks();
    BOARD_CONNECTED_ITEM* lock_point = GetBoard()->GetLockPoint(m_connected_pos, GetTrackSeg()->GetLayerSet());
    if(max_width > m_Width) //uint(GetTrackSeg()->GetWidth()))
    {
        m_connected_item_rad = max_width>>1; 
        if(m_T_tracks) 
        {
            //If T-Junction, make it at rect of T.
            m_connected_pos_length_delta += m_connected_item_rad - m_trackseg_rad;
            m_connected_item_rad = m_Width;
        }
        else
            if(lock_point) 
                if(lock_point->Type() == PCB_PAD_T)
                    if((dynamic_cast<D_PAD*>(lock_point)->GetShape() == PAD_SHAPE_RECT) || (dynamic_cast<D_PAD*>(lock_point)->GetShape() == PAD_SHAPE_TRAPEZOID))
                        m_rect_type = true;
    }
    else
    {
        //If Junction inside Oval Pad, make it's item_rad PAD size.
        if(lock_point)
        {
            if(lock_point->Type() == PCB_PAD_T)
            {
                if(dynamic_cast<D_PAD*>(lock_point)->GetShape() == PAD_SHAPE_OVAL)
                {
                    m_connected_item_rad = std::min(dynamic_cast<D_PAD*>(lock_point)->GetSize().GetWidth(), dynamic_cast<D_PAD*>(lock_point)->GetSize().GetHeight())>>1;
                }
            }
        }
    }
}

void TEARDROP_JUNCTIONS::TestTrackLength(void)
{
    if(m_T_tracks)
    {
        if(m_connected_pos_length_delta + m_trackseg_rad  >= m_trackseg_length)
            m_set_ok = false;
    }
    else
        TEARDROP::TestTrackLength();
}

void TEARDROP_JUNCTIONS::CalcSubland(const int aWidthRatio, const int aLengthRatio)
{
    if(m_T_tracks)
    {
        m_width_rad = CalcSublandRad(aWidthRatio, m_connected_item_rad, m_trackseg_rad);
        m_length = double(m_connected_item_rad>>1) * (double(aLengthRatio) / 100.0);
        int tr_d = m_trackseg_length - m_length - m_width_rad;
        m_connected_pos_length_delta = std::min(m_connected_pos_length_delta, tr_d);
    }
    else
        TEARDROP::CalcSubland(aWidthRatio, aLengthRatio);
}

void TEARDROP_JUNCTIONS::CalcFillet(const int aWidthRatio, const int aLengthRatio)
{
    if(m_T_tracks)
    {
        //uint length_corr = 5 * m_Width / 7;
        uint length_corr = 5 * m_poly_seg_width / 7;
        uint max_dist_item_to_tear = CalcFilletPosDist(aLengthRatio, m_connected_item_rad) + m_connected_pos_length_delta;
        uint dist_item_to_tear = std::min(max_dist_item_to_tear, m_trackseg_length + length_corr);
        m_length = dist_item_to_tear - m_connected_pos_length_delta;
        CalcFilletSegs();
        m_length -= length_corr;
        
        m_width_rad = m_length;

        m_fillet_seg_length = hypot(m_length, m_width_rad);
        m_fillet_seg_A_angle = m_trackseg_angle_inv + M_PI_4;
        m_fillet_seg_B_angle = m_trackseg_angle_inv - M_PI_4;
    }
    else
    {
        m_poly_seg_width = GetTrackSeg()->GetWidth();
        m_poly_seg_rad = m_poly_seg_width>>1;
        TEARDROP_RECT::CalcFillet(aWidthRatio, aLengthRatio);
    }
}

void TEARDROP_JUNCTIONS::CalcTeardrop(const int aWidthRatio, const int aLengthRatio)
{
    if(m_T_tracks)
    {
        m_width_rad = CalcTeardropWidthRad(aWidthRatio, m_connected_item_rad, m_trackseg_rad);
        
        uint length_corr = 4 * m_trackseg_rad / 5;
        uint max_dist_item_to_tear = CalcTeardropPosDist(aLengthRatio, m_connected_item_rad) + m_connected_pos_length_delta;
        uint dist_item_to_tear = std::min(max_dist_item_to_tear, m_trackseg_length + length_corr);
        m_length = dist_item_to_tear - m_connected_pos_length_delta;
        m_length -= length_corr;

        CalcTeardropArcs();
        
        m_teardrop_segs_arc_rad = m_length;
        m_teardrop_segs_angle_add = M_PI_2 / double(m_num_arc_segs); 
        
        m_can_draw_clearance = (m_length > m_connected_pos_length_delta + m_width_rad + m_clearance);
    }
    else
    {
        m_poly_seg_width = GetTrackSeg()->GetWidth();
        m_poly_seg_rad = m_poly_seg_width>>1;
        TEARDROP_RECT::CalcTeardrop(aWidthRatio, aLengthRatio);
    }
}

void TEARDROP_JUNCTIONS::CollectTracks(const TRACK* aTrackSeg, const wxPoint aPos)
{
    Collect(aTrackSeg, aPos, m_connected_tracks);
}

int TEARDROP_JUNCTIONS::MaxWidthFromTracks(void)
{
    if(m_connected_tracks.size())
    {
        return TrackNodeItem::GetMaxWidth(m_connected_tracks);
    }   
        //return m_connected_tracks[MaxWidthItemNum(m_connected_tracks)]->GetWidth();
    return 0;
}

bool TEARDROP_JUNCTIONS::Find_T_FromTracks(void)
{
    m_T_track_back = nullptr;
    m_T_track_next = nullptr;
    if(GetTrackSeg() && (m_connected_tracks.size() > 1))
    {
        double angle_next = m_trackseg_angle + M_PI_2;
        if(angle_next >= M_PIx2)
            angle_next -= M_PIx2;
        double angle_back = m_trackseg_angle - M_PI_2;
        if(angle_back < 0.0)
            angle_back += M_PIx2;

        int angle_next_rnd = Rad2DeciDegRnd(angle_next);
        int angle_back_rnd = Rad2DeciDegRnd(angle_back);
        
        for(TRACK* track_seg : m_connected_tracks)
        {
            double angle_track = TrackSegAngle(track_seg, m_connected_pos);
            //int angle_track_rnd = Rad2MilsInt(angle_track);
            int angle_track_rnd = Rad2DeciDegRnd(angle_track);
            
            if(angle_track_rnd == angle_back_rnd)
                m_T_track_back = track_seg;
            if(angle_track_rnd == angle_next_rnd)
                m_T_track_next = track_seg;
            if(m_T_track_back && m_T_track_next)
                return true;
        }
    }
    return false;
}


