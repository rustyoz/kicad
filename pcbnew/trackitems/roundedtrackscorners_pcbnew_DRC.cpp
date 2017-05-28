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


//-----------------------------------------------------------------------------------------------------/
// Roundedtrackscorner -> Track / VIA clearance
//-----------------------------------------------------------------------------------------------------/
bool ROUNDEDTRACKSCORNERS::DRC_DoClearanceTest(const ROUNDEDTRACKSCORNER* aCorner, const wxPoint aTestPoint, const int aMinDist)
{
    for(uint n = 0; n < aCorner->GetPolyPointsNum() - 1; ++n)
    {
        wxPoint segStart = aCorner->GetPolyPoint(n);
        wxPoint segEnd = aCorner->GetPolyPoint(n+1);
        if(!TestSegment(segStart, segEnd, aTestPoint, aMinDist))
            return false;
    }
    return true;
}

bool ROUNDEDTRACKSCORNERS::DRC_ClearanceTest(const ROUNDEDTRACKSCORNER* aCorner, const VIA* aVia, const int aMinDist)
{
    return DRC_DoClearanceTest(aCorner, aVia->GetStart(), aMinDist);
}

bool ROUNDEDTRACKSCORNERS::DRC_ClearanceTest(const ROUNDEDTRACKSCORNER* aCorner, const TRACK* aTrackSeg, const int aMinDist)
{
    wxPoint track_start_pos = aTrackSeg->GetStart();
    wxPoint track_end_pos = aTrackSeg->GetEnd();
    
    if(dynamic_cast<ROUNDEDCORNERTRACK*>(const_cast<TRACK*>(aTrackSeg)))
    {
        track_start_pos = dynamic_cast<ROUNDEDCORNERTRACK*>(const_cast<TRACK*>(aTrackSeg))->GetStartVisible();
        track_end_pos = dynamic_cast<ROUNDEDCORNERTRACK*>(const_cast<TRACK*>(aTrackSeg))->GetEndVisible();
    }

    for(uint n = 0; n < aCorner->GetPolyPointsNum(); ++n)
    {
        if(!TestSegment(track_start_pos, track_end_pos, aCorner->GetPolyPoint(n), aMinDist))
            return false;
    }

    return true;            
}

bool ROUNDEDTRACKSCORNERS::DRC_ClearanceTest(const TrackNodeItem::ROUNDEDTRACKSCORNER* aCornerFirst, TrackNodeItem::ROUNDEDTRACKSCORNER* aCornerSecond, const int aMinDist)
{
    for(uint n = 0; n < aCornerSecond->GetPolyPointsNum(); ++n)
    {
        if(!DRC_DoClearanceTest(aCornerFirst, aCornerSecond->GetPolyPoint(n), aMinDist))
            return false;
    }
    return true;
}

//Only return DRC error when editing. 
bool ROUNDEDTRACKSCORNERS::DRC_TestClearance(const ROUNDEDTRACKSCORNER* aCorner, const TRACK* aTrackSeg, const int aMinDist, DRC* aDRC)
{
    bool editing = DRC_Flags(aCorner->GetTrackSeg()->GetFlags());
    editing |= DRC_Flags(aCorner->GetTrackSegSecond()->GetFlags());
    editing |= DRC_Flags(aTrackSeg->GetFlags());
    
    if(aTrackSeg->Type() == PCB_VIA_T)
    {
        if(!DRC_ClearanceTest(aCorner, static_cast<VIA*>(const_cast<TRACK*>(aTrackSeg)), aMinDist))
        {
            int error_code = DRCE_TRACK_NEAR_VIA;
            if(editing)
            {
                aDRC->m_currentMarker = aDRC->fillMarker(static_cast<TRACK*>(const_cast<ROUNDEDTRACKSCORNER*>(aCorner)), static_cast<VIA*>(const_cast<TRACK*>(aTrackSeg)), error_code, aDRC->m_currentMarker);
                return false;
            }
            m_Parent->DRC_AddMarker(aCorner, aTrackSeg, aCorner->GetPosition(), error_code);
            return true;
        }
    }
    
    if( aTrackSeg->Type() == PCB_TRACE_T )
    {
        if(!DRC_ClearanceTest(aCorner, static_cast<TRACK*>(const_cast<TRACK*>(aTrackSeg)), aMinDist))
        {
            int error_code = DRCE_TRACK_SEGMENTS_TOO_CLOSE;
            if(editing)
            {
                aDRC->m_currentMarker = aDRC->fillMarker(static_cast<TRACK*>(const_cast<ROUNDEDTRACKSCORNER*>(aCorner)), static_cast<TRACK*>(const_cast<TRACK*>(aTrackSeg)), error_code, aDRC->m_currentMarker );
                return false;
            }
            m_Parent->DRC_AddMarker(aCorner, aTrackSeg, aCorner->GetPosition(), error_code);
            return true;
        }
    }
    
    if(aTrackSeg->Type() == PCB_ROUNDEDTRACKSCORNER_T)
    {
        if(!DRC_ClearanceTest(aCorner, static_cast<ROUNDEDTRACKSCORNER*>(const_cast<TRACK*>(aTrackSeg)), aMinDist))
        {
            int error_code = DRCE_TRACK_SEGMENTS_TOO_CLOSE;
            if(editing)
            {
                aDRC->m_currentMarker = aDRC->fillMarker(static_cast<TRACK*>(const_cast<ROUNDEDTRACKSCORNER*>(aCorner)), static_cast<TRACK*>(const_cast<TRACK*>(aTrackSeg)), error_code, aDRC->m_currentMarker );
                return false;
            }
            m_Parent->DRC_AddMarker(aCorner, aTrackSeg, aCorner->GetPosition(), error_code);
            return true;
        }
    }
    
    return true;
}

bool ROUNDEDTRACKSCORNERS::DRC_Clearance(const BOARD_CONNECTED_ITEM* aRefFirst, const BOARD_CONNECTED_ITEM* aRefSecond, const int aMinDist, DRC* aDRC)
{
    if(aRefFirst != aRefSecond)
    {
        if(aRefFirst->Type() == PCB_ROUNDEDTRACKSCORNER_T)
            return DRC_TestClearance(static_cast<ROUNDEDTRACKSCORNER*>(const_cast<BOARD_CONNECTED_ITEM*>(aRefFirst)), static_cast<TRACK*>(const_cast<BOARD_CONNECTED_ITEM*>(aRefSecond)), aMinDist, aDRC);
        if(aRefSecond->Type() == PCB_ROUNDEDTRACKSCORNER_T)
            return DRC_TestClearance(static_cast<ROUNDEDTRACKSCORNER*>(const_cast<BOARD_CONNECTED_ITEM*>(aRefSecond)), static_cast<TRACK*>(const_cast<BOARD_CONNECTED_ITEM*>(aRefFirst)), aMinDist, aDRC);
    }
    return true;
}


//-----------------------------------------------------------------------------------------------------/
// Roundedtrackscorner-> PAD clearance
//-----------------------------------------------------------------------------------------------------/
//drc_clearance_test_functions.cpp base
bool ROUNDEDTRACKSCORNERS::DRC_DoClearanceTest(const ROUNDEDTRACKSCORNER* aCorner, const D_PAD* aPad, const int aMinDist, DRC* aDRC)
{
    wxPoint saved_pad_to_test_pos = aDRC->m_padToTestPos;
    wxPoint saved_segm_end = aDRC->m_segmEnd;
    int saved_segm_angle = aDRC->m_segmAngle;
    int saved_segm_lengt = aDRC->m_segmLength;
    uint poly_points;
    wxPoint pad_shape_pos = aPad->ShapePos();
    wxPoint delta;
    bool ret_val = true;
    
    poly_points = aCorner->GetPolyPointsNum();
    for(uint n = 0; n < poly_points -1; ++n)
    {
        aDRC->m_padToTestPos = pad_shape_pos - aCorner->GetPolyPoint(n);
        aDRC->m_segmEnd = delta = aCorner->GetPolyPoint(n+1) - aCorner->GetPolyPoint(n);
        aDRC->m_segmAngle = ArcTangente(delta.y, delta.x);
        RotatePoint( &delta, aDRC->m_segmAngle);
        aDRC->m_segmLength = delta.x;
        if(!aDRC->checkClearanceSegmToPad(aPad, aCorner->GetWidth(), aMinDist))
            ret_val = false;
    }
    
    aDRC->m_padToTestPos = saved_pad_to_test_pos;
    aDRC->m_segmEnd = saved_segm_end;
    aDRC->m_segmAngle = saved_segm_angle;
    aDRC->m_segmLength = saved_segm_lengt;
    return ret_val;            
}

//Only return DRC error when editing track. 
bool ROUNDEDTRACKSCORNERS::DRC_Clearance(const BOARD_CONNECTED_ITEM* aRef, const D_PAD* aPad, const int aMinDist, DRC* aDRC)
{
    if(aRef->Type() == PCB_ROUNDEDTRACKSCORNER_T)
    {
        bool editing = DRC_Flags(dynamic_cast<ROUNDEDTRACKSCORNER*>(const_cast<BOARD_CONNECTED_ITEM*>(aRef))->GetTrackSeg()->GetFlags());
        editing |= DRC_Flags(dynamic_cast<ROUNDEDTRACKSCORNER*>(const_cast<BOARD_CONNECTED_ITEM*>(aRef))->GetTrackSegSecond()->GetFlags());
        if(!DRC_DoClearanceTest(static_cast<ROUNDEDTRACKSCORNER*>(const_cast<BOARD_CONNECTED_ITEM*>(aRef)), aPad, aMinDist, aDRC))
        {
            int error_code = DRCE_TRACK_NEAR_PAD;
            if(editing)
            {
                aDRC->m_currentMarker = aDRC->fillMarker(static_cast<TRACK*>(const_cast<BOARD_CONNECTED_ITEM*>(aRef)), static_cast<D_PAD*>(const_cast<D_PAD*>(aPad)), error_code, aDRC->m_currentMarker);
                return false;
            }
            m_Parent->DRC_AddMarker(static_cast<ROUNDEDTRACKSCORNER*>(const_cast<BOARD_CONNECTED_ITEM*>(aRef)), aPad, aRef->GetPosition(), error_code);
        }
    }
    return true;
}


//-----------------------------------------------------------------------------------------------------/
// Roundedtrackscorner -> text clearance 
//-----------------------------------------------------------------------------------------------------/
//Do not return DRC error. Only marks errorplaces.
void ROUNDEDTRACKSCORNERS::DRC_Clearance(const BOARD_CONNECTED_ITEM* aRef, const TEXTE_PCB* aText, const int aMinDist, DRC* aDRC)
{
    if(aRef->Type() == PCB_ROUNDEDTRACKSCORNER_T)
    {
        ROUNDEDTRACKSCORNER* corner = static_cast<ROUNDEDTRACKSCORNER*>(const_cast<BOARD_CONNECTED_ITEM*>(aRef));
        std::vector<wxPoint> text_shape;
        text_shape.clear();
        aText->TransformTextShapeToSegmentList(text_shape);
        
        for(uint n = 0; n < corner->GetPolyPointsNum() - 1; n++)
        {
            wxPoint segStart = corner->GetPolyPoint(n);
            wxPoint segEnd = corner->GetPolyPoint(n+1);
            SEG segref(segStart, segEnd);

            for(unsigned jj = 0; jj < text_shape.size(); jj += 2)
            {
                SEG segtest(text_shape[jj], text_shape[jj+1]);
                int dist = segref.Distance(segtest);

                if(dist < aMinDist)
                {
                    int error_code = DRCE_TRACK_INSIDE_TEXT;
                    m_Parent->DRC_AddMarker(corner, corner, corner->GetPosition(), error_code);
                    break;
                }
            }
        }
    }
}

