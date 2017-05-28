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

#include <geometry/seg.h>

using namespace TrackNodeItem;


//-----------------------------------------------------------------------------------------------------/
// Teardrop -> Track / VIA clearance
//-----------------------------------------------------------------------------------------------------/
bool TEARDROPS::DRC_DoClearanceTest(const TEARDROP* aTear, const wxPoint aTestPoint, const int aMinDist)
{
    wxPoint delta_circles;
    switch(aTear->GetShape())
    {
        case TEARDROP::FILLET_T:
        case TEARDROP::TEARDROP_T:
            for(uint n = 0; n < aTear->GetPolyPointsNum() - 1; ++n)
            {
                wxPoint segStart = aTear->GetPolyPoint(n);
                wxPoint segEnd = aTear->GetPolyPoint(n+1);
                if(!TestSegment(segStart, segEnd, aTestPoint, aMinDist))
                    return false;
            }
            break;
        case TEARDROP::SUBLAND_T:
            delta_circles = aTestPoint - aTear->GetPosition();
            if((int)hypot(delta_circles.x, delta_circles.y) < aMinDist)
                return false;
    }
    return true;
}

bool TEARDROPS::DRC_ClearanceTest(const TEARDROP* aTear, const VIA* aVia, const int aMinDist)
{
    return DRC_DoClearanceTest(aTear, aVia->GetStart(), aMinDist);
}

bool TEARDROPS::DRC_ClearanceTest(const TEARDROP* aTear, const TRACK* aTrackSeg, const int aMinDist)
{
    wxPoint track_start_pos = aTrackSeg->GetStart();
    wxPoint track_end_pos = aTrackSeg->GetEnd();
    
    if(dynamic_cast<ROUNDEDCORNERTRACK*>(const_cast<TRACK*>(aTrackSeg)))
    {
        track_start_pos = dynamic_cast<ROUNDEDCORNERTRACK*>(const_cast<TRACK*>(aTrackSeg))->GetStartVisible();
        track_end_pos = dynamic_cast<ROUNDEDCORNERTRACK*>(const_cast<TRACK*>(aTrackSeg))->GetEndVisible();
    }
    
    if(!DRC_DoClearanceTest(aTear, track_start_pos, aMinDist))
        return false;
    if(!DRC_DoClearanceTest(aTear, track_end_pos, aMinDist))
        return false;

    //Junctions must have to test with whole track segment length.
    if(dynamic_cast<TEARDROP_JUNCTIONS*>(const_cast<TEARDROP*>(aTear)))
    {
        wxPoint delta = track_end_pos - track_start_pos;
        double angle = ArcTangente( delta.y, delta.x );
        RotatePoint( &delta, angle );
        switch(aTear->GetShape())
        {
            case TEARDROP::FILLET_T:
            case TEARDROP::TEARDROP_T:
            {
                //Only two outest points have to be tested.
                //First poly point.
                wxPoint segStartPoint = aTear->GetPolyPoint(0) - track_start_pos;
                RotatePoint( &segStartPoint, angle );
                if(!m_EditFrame->GetDrcController()->checkMarginToCircle(segStartPoint, aMinDist, delta.x))
                    return false;

                //Last poly point.
                segStartPoint = aTear->GetPolyPoint(aTear->GetPolyPointsNum() - 1) - track_start_pos;
                RotatePoint( &segStartPoint, angle );
                if(!m_EditFrame->GetDrcController()->checkMarginToCircle(segStartPoint, aMinDist, delta.x))
                    return false;

                break;
            }
            case TEARDROP::SUBLAND_T:
            {
                wxPoint segStartPoint = aTear->GetPosition() - track_start_pos;
                RotatePoint( &segStartPoint, angle );
                if(!m_EditFrame->GetDrcController()->checkMarginToCircle(segStartPoint, aMinDist, delta.x))
                    return false;
            }
        }
    }
    return true;            
}

bool TEARDROPS::DRC_ClearanceTest(const TEARDROP* aTear, ROUNDEDTRACKSCORNER* aCorner, const int aMinDist)
{
    for(uint n = 0; n < aCorner->GetPolyPointsNum(); ++n)
    {
        if(!DRC_DoClearanceTest(aTear, aCorner->GetPolyPoint(n), aMinDist))
            return false;
    }
    return true;
}

//Only return DRC error when editing. 
bool TEARDROPS::DRC_TestClearance(const TEARDROP* aTear, const TRACK* aTrackSeg, const int aMinDist, DRC* aDRC)
{
    bool editing = DRC_Flags(aTear->GetTrackSeg()->GetFlags());
    if(dynamic_cast<TEARDROP_VIA*>(const_cast<TEARDROP*>(aTear)))
        editing |= DRC_Flags(dynamic_cast<TEARDROP_VIA*>(const_cast<TEARDROP*>(aTear))->GetConnectedVia()->GetFlags()); 
    editing |= DRC_Flags(aTrackSeg->GetFlags());
    if(dynamic_cast<TEARDROP_VIA*>(const_cast<TRACK*>(aTrackSeg)))
        editing |= DRC_Flags(dynamic_cast<TEARDROP_VIA*>(const_cast<TRACK*>(aTrackSeg))->GetConnectedVia()->GetFlags()); 
    
    if(aTrackSeg->Type() == PCB_VIA_T)
    {
        if(!DRC_ClearanceTest(aTear, static_cast<VIA*>(const_cast<TRACK*>(aTrackSeg)), aMinDist))
        {
            int error_code = DRCE_TEARDROP_NEAR_VIA;
            if(dynamic_cast<TEARDROP_JUNCTIONS*>(const_cast<TEARDROP*>(aTear)))
                error_code = DRCE_JUNCTION_NEAR_VIA;
            if(editing)
            {
                aDRC->m_currentMarker = aDRC->fillMarker(static_cast<TRACK*>(const_cast<TEARDROP*>(aTear)), static_cast<VIA*>(const_cast<TRACK*>(aTrackSeg)), error_code, aDRC->m_currentMarker);
                return false;
            }
            m_Parent->DRC_AddMarker(aTear, aTrackSeg, aTear->GetPosition(), error_code);
            return true;
        }
    }
    
    if( aTrackSeg->Type() == PCB_TRACE_T )
    {
        if(!DRC_ClearanceTest(aTear, static_cast<TRACK*>(const_cast<TRACK*>(aTrackSeg)), aMinDist))
        {
            int error_code = DRCE_TEARDROP_NEAR_TRACK;
            if(dynamic_cast<TEARDROP_JUNCTIONS*>(const_cast<TEARDROP*>(aTear)))
                error_code = DRCE_JUNCTION_NEAR_TRACK;
            if(editing)
            {
                aDRC->m_currentMarker = aDRC->fillMarker(static_cast<TRACK*>(const_cast<TEARDROP*>(aTear)), static_cast<TRACK*>(const_cast<TRACK*>(aTrackSeg)), error_code, aDRC->m_currentMarker );
                return false;
            }
            m_Parent->DRC_AddMarker(aTear, aTrackSeg, aTear->GetPosition(), error_code);
            return true;
        }
    }
    
    if(aTrackSeg->Type() == PCB_TEARDROP_T)
    {
        int error_code = DRCE_TEARDROP_NEAR_TEARDROP;
        //If aTear is junction, or aTrackSeg is junction.
        TEARDROP* tear = dynamic_cast<TEARDROP*>(const_cast<TRACK*>(aTrackSeg));
        TEARDROP* junction = const_cast<TEARDROP*>(aTear);
        if(dynamic_cast<TEARDROP_JUNCTIONS*>(const_cast<TRACK*>(aTrackSeg)) && !(dynamic_cast<TEARDROP_JUNCTIONS*>(const_cast<TEARDROP*>(aTear))))
        {
            junction = dynamic_cast<TEARDROP*>(const_cast<TRACK*>(aTrackSeg));
            tear = const_cast<TEARDROP*>(aTear);
        }
        if(dynamic_cast<TEARDROP_JUNCTIONS*>(junction))
        {
            error_code = DRCE_JUNCTION_NEAR_TEARDROP;
            if(dynamic_cast<TEARDROP_JUNCTIONS*>(tear))
                error_code = DRCE_JUNCTION_NEAR_JUNCTION;
        }
        
        switch(junction->GetShape())
        {
            case TEARDROP::FILLET_T:
            case TEARDROP::TEARDROP_T:
            {
                //Only two outest points have to be tested.
                //First poly point.
                if(!DRC_DoClearanceTest(tear, junction->GetPolyPoint(0), aMinDist))
                {
                    if(editing)
                    {
                        aDRC->m_currentMarker = aDRC->fillMarker(static_cast<TRACK*>(junction), static_cast<TRACK*>(tear), error_code, aDRC->m_currentMarker );
                        return false;
                    }
                    m_Parent->DRC_AddMarker(junction, tear, junction->GetPosition(), error_code);
                    return true;
                }
                //Last poly point.
                if(!DRC_DoClearanceTest(tear, junction->GetPolyPoint(junction->GetPolyPointsNum() - 1), aMinDist))
                {
                    if(editing)
                    {
                        aDRC->m_currentMarker = aDRC->fillMarker(static_cast<TRACK*>(junction), static_cast<TRACK*>(tear), error_code, aDRC->m_currentMarker );
                        return false;
                    }
                    m_Parent->DRC_AddMarker(junction, tear, junction->GetPosition(), error_code);
                    return true;
                }

                break;
            }
            case TEARDROP::SUBLAND_T:
            {
                if(!DRC_DoClearanceTest(tear, junction->GetPosition(), aMinDist))
                {
                    if(editing)
                    {
                        aDRC->m_currentMarker = aDRC->fillMarker(static_cast<TRACK*>(junction), static_cast<TRACK*>(tear), error_code, aDRC->m_currentMarker );
                        return false;
                    }
                    m_Parent->DRC_AddMarker(junction, tear, junction->GetPosition(), error_code);
                    return true;
                }
            }
        }
    }   
    
    if(aTrackSeg->Type() == PCB_ROUNDEDTRACKSCORNER_T)
    {
        if(!DRC_ClearanceTest(aTear, static_cast<ROUNDEDTRACKSCORNER*>(const_cast<TRACK*>(aTrackSeg)), aMinDist))
        {
            int error_code = DRCE_TEARDROP_NEAR_TRACK;
            if(dynamic_cast<TEARDROP_JUNCTIONS*>(const_cast<TEARDROP*>(aTear)))
                error_code = DRCE_JUNCTION_NEAR_TRACK;
            if(editing)
            {
                aDRC->m_currentMarker = aDRC->fillMarker(static_cast<TRACK*>(const_cast<TEARDROP*>(aTear)), static_cast<TRACK*>(const_cast<TRACK*>(aTrackSeg)), error_code, aDRC->m_currentMarker );
                return false;
            }
            m_Parent->DRC_AddMarker(aTear, aTrackSeg, aTear->GetPosition(), error_code);
            return true;
        }
    }
    
    return true;
}

bool TEARDROPS::DRC_Clearance(const BOARD_CONNECTED_ITEM* aRefFirst, const BOARD_CONNECTED_ITEM* aRefSecond, const int aMinDist, DRC* aDRC)
{
    if(aRefFirst != aRefSecond)
    {
        if(aRefFirst->Type() == PCB_TEARDROP_T)
            return DRC_TestClearance(static_cast<TEARDROP*>(const_cast<BOARD_CONNECTED_ITEM*>(aRefFirst)), static_cast<TRACK*>(const_cast<BOARD_CONNECTED_ITEM*>(aRefSecond)), aMinDist, aDRC);
        if(aRefSecond->Type() == PCB_TEARDROP_T)
            return DRC_TestClearance(static_cast<TEARDROP*>(const_cast<BOARD_CONNECTED_ITEM*>(aRefSecond)), static_cast<TRACK*>(const_cast<BOARD_CONNECTED_ITEM*>(aRefFirst)), aMinDist, aDRC);
    }
    return true;
}


//-----------------------------------------------------------------------------------------------------/
// Teardrop -> PAD clearance 
//-----------------------------------------------------------------------------------------------------/
//drc_clearance_test_functions.cpp base
bool TEARDROPS::DRC_DoClearanceTest(const TEARDROP* aTear, const D_PAD* aPad, const int aMinDist, DRC* aDRC)
{
    wxPoint saved_pad_to_test_pos = aDRC->m_padToTestPos;
    wxPoint saved_segm_end = aDRC->m_segmEnd;
    int saved_segm_angle = aDRC->m_segmAngle;
    int saved_segm_lengt = aDRC->m_segmLength;
    uint poly_points;
    wxPoint pad_shape_pos = aPad->ShapePos();
    wxPoint delta;
    bool ret_val = true;
    switch(aTear->GetShape())
    {
        case TEARDROP::FILLET_T:
        case TEARDROP::TEARDROP_T:
            poly_points = aTear->GetPolyPointsNum();
            for(uint n = 0; n < poly_points -1; ++n)
            {
                aDRC->m_padToTestPos = pad_shape_pos - aTear->GetPolyPoint(n);
                aDRC->m_segmEnd = delta = aTear->GetPolyPoint(n+1) - aTear->GetPolyPoint(n);
                aDRC->m_segmAngle = ArcTangente(delta.y, delta.x);
                RotatePoint( &delta, aDRC->m_segmAngle);
                aDRC->m_segmLength = delta.x;
                if(!aDRC->checkClearanceSegmToPad(aPad, aTear->GetWidth(), aMinDist))
                    ret_val = false;
            }
            break;
        case TEARDROP::SUBLAND_T:
            aDRC->m_padToTestPos = pad_shape_pos - aTear->GetPosition();
            aDRC->m_segmEnd = aTear->GetPosition() - aTear->GetPosition();
            aDRC->m_segmAngle = 0;
            aDRC->m_segmLength = 0;
            if(!aDRC->checkClearanceSegmToPad(aPad, aTear->GetWidth(), aMinDist))
                ret_val = false;
    }
    aDRC->m_padToTestPos = saved_pad_to_test_pos;
    aDRC->m_segmEnd = saved_segm_end;
    aDRC->m_segmAngle = saved_segm_angle;
    aDRC->m_segmLength = saved_segm_lengt;
    return ret_val;            
}

//Only return DRC error when editing track. 
bool TEARDROPS::DRC_Clearance(const BOARD_CONNECTED_ITEM* aRef, const D_PAD* aPad, const int aMinDist, DRC* aDRC)
{
    if(aRef->Type() == PCB_TEARDROP_T)
    {
        bool editing = DRC_Flags(dynamic_cast<TEARDROP*>(const_cast<BOARD_CONNECTED_ITEM*>(aRef))->GetTrackSeg()->GetFlags());
        if(!DRC_DoClearanceTest(static_cast<TEARDROP*>(const_cast<BOARD_CONNECTED_ITEM*>(aRef)), aPad, aMinDist, aDRC))
        {
            int error_code = DRCE_TEARDROP_NEAR_PAD;
            if(dynamic_cast<TEARDROP_JUNCTIONS*>(const_cast<BOARD_CONNECTED_ITEM*>(aRef)))
                error_code = DRCE_JUNCTION_NEAR_PAD;
            if(editing)
            {
                aDRC->m_currentMarker = aDRC->fillMarker(static_cast<TRACK*>(const_cast<BOARD_CONNECTED_ITEM*>(aRef)), static_cast<D_PAD*>(const_cast<D_PAD*>(aPad)), error_code, aDRC->m_currentMarker);
                return false;
            }
            m_Parent->DRC_AddMarker(static_cast<TEARDROP*>(const_cast<BOARD_CONNECTED_ITEM*>(aRef)), aPad, aRef->GetPosition(), error_code);
        }
    }
    return true;
}


//-----------------------------------------------------------------------------------------------------/
// Teardrop -> text clearance 
//-----------------------------------------------------------------------------------------------------/
//Do not return DRC error. Only marks errorplaces.
void TEARDROPS::DRC_Clearance(const BOARD_CONNECTED_ITEM* aRef, const TEXTE_PCB* aText, const int aMinDist, DRC* aDRC)
{
    if(aRef->Type() == PCB_TEARDROP_T)
    {
        TEARDROP* tear = static_cast<TEARDROP*>(const_cast<BOARD_CONNECTED_ITEM*>(aRef));
        std::vector<wxPoint> text_shape;
        text_shape.clear();
        aText->TransformTextShapeToSegmentList(text_shape);
        
        switch(tear->GetShape())
        {
            case TEARDROP::FILLET_T:
            case TEARDROP::TEARDROP_T:
                for(uint n = 0; n < tear->GetPolyPointsNum() - 1; n++)
                {
                    wxPoint segStart = tear->GetPolyPoint(n);
                    wxPoint segEnd = tear->GetPolyPoint(n+1);
                    SEG segref(segStart, segEnd);

                    for(unsigned jj = 0; jj < text_shape.size(); jj += 2)
                    {
                        SEG segtest(text_shape[jj], text_shape[jj+1]);
                        int dist = segref.Distance(segtest);

                        if(dist < aMinDist)
                        {
                            int error_code = DRCE_TEARDROP_INSIDE_TEXT;
                            if(dynamic_cast<TEARDROP_JUNCTIONS*>(const_cast<TEARDROP*>(tear)))
                                error_code = DRCE_JUNCTION_INSIDE_TEXT; //Junctions too.
                            m_Parent->DRC_AddMarker(tear, aText, tear->GetPosition(), error_code);

                            break;
                        }
                    }
                }
                break;
            case TEARDROP::SUBLAND_T:
                for(unsigned jj = 0; jj < text_shape.size(); jj += 2)
                {
                    SEG segtest(text_shape[jj], text_shape[jj+1]);

                    if(segtest.PointCloserThan(tear->GetPosition(), aMinDist))
                    {
                        int error_code = DRCE_TEARDROP_INSIDE_TEXT;
                        if(dynamic_cast<TEARDROP_JUNCTIONS*>(const_cast<TEARDROP*>(tear)))
                            error_code = DRCE_JUNCTION_INSIDE_TEXT; //Junctions too.
                        m_Parent->DRC_AddMarker(tear, aText, tear->GetPosition(), error_code);
                        break;
                    }
                }
        }
    }
}

//-----------------------------------------------------------------------------------------------------/
// Teardrop Rules 
//-----------------------------------------------------------------------------------------------------/
//Do not return DRC error. Only marks errorplaces.
void  TEARDROPS::DRC_Rules(const TEARDROP* aTeardrop, DRC* aDRC)
{
    if(aTeardrop && (aTeardrop->Type() == PCB_TEARDROP_T))
    {
        //Do not check when drag.
        bool editing = DRC_Flags(aTeardrop->GetTrackSeg()->GetFlags());
        if(dynamic_cast<TEARDROP_VIA*>(const_cast<TEARDROP*>(aTeardrop)))
            editing |= DRC_Flags(dynamic_cast<TEARDROP_VIA*>(const_cast<TEARDROP*>(aTeardrop))->GetConnectedVia()->GetFlags()); 
        if(!editing)
        {
            //Size test
            if(!aTeardrop->IsSetOK()) 
            {
                int error_code = DRCE_TEARDROP_TOO_SMALL;
                if(dynamic_cast<TEARDROP_JUNCTIONS*>(const_cast<TEARDROP*>(aTeardrop)))
                    error_code = DRCE_JUNCTION_TOO_SMALL; //Junctions too.
                m_Parent->DRC_AddMarker(aTeardrop, nullptr, aTeardrop->GetPosition(), error_code);
            }
            
            //Teardrop PAD pos
            if(dynamic_cast<TEARDROP_PAD*>(const_cast<TEARDROP*>(aTeardrop)))
            {
                if(aTeardrop->GetEnd() != aTeardrop->GetConnectedItem()->GetPosition())
                {
                    m_Parent->DRC_AddMarker(aTeardrop, aTeardrop->GetConnectedItem(), aTeardrop->GetPosition(), DRCE_TEARDROP_NOT_IN_PAD);
                }
            }
            
            //Teardrop VIA pos
            if(dynamic_cast<TEARDROP_VIA*>(const_cast<TEARDROP*>(aTeardrop)))
            {
                if(aTeardrop->GetEnd() != dynamic_cast<TEARDROP_VIA*>(const_cast<TEARDROP*>(aTeardrop))->GetConnectedVia()->GetEnd())
                {
                    m_Parent->DRC_AddMarker(aTeardrop, dynamic_cast<TEARDROP_VIA*>(const_cast<TEARDROP*>(aTeardrop))->GetConnectedVia(), aTeardrop->GetPosition(), DRCE_TEARDROP_NOT_IN_VIA);
                }
            }

            if(dynamic_cast<TEARDROP_JUNCTIONS*>(const_cast<TEARDROP*>(aTeardrop)))
            {
                TRACK* track_next = dynamic_cast<TEARDROP_JUNCTIONS*>(const_cast<TEARDROP*>(aTeardrop))->Get_T_SegNext(); 
                TRACK* track_back = dynamic_cast<TEARDROP_JUNCTIONS*>(const_cast<TEARDROP*>(aTeardrop))->Get_T_SegBack();
                if(track_next && track_back)
                {
                    //T-Junctions track segment test.
                    if((track_back->GetLength() < dynamic_cast<TEARDROP_JUNCTIONS*>(const_cast<TEARDROP*>(aTeardrop))->GetSizeLength()) || (track_next->GetLength() < dynamic_cast<TEARDROP_JUNCTIONS*>(const_cast<TEARDROP*>(aTeardrop))->GetLength()))
                        m_Parent->DRC_AddMarker(aTeardrop, nullptr, aTeardrop->GetPosition(), DRCE_TJUNCTION_TRACK_SEGS_LENGTH);
                }
            }
            
            //Teardrops track test.
            TRACK* track_seg = aTeardrop->GetTrackSeg();
            if(track_seg)
            {
                if(!((aTeardrop->GetEnd() == track_seg->GetStart()) || (aTeardrop->GetEnd() == track_seg->GetEnd())))
                    m_Parent->DRC_AddMarker(aTeardrop, track_seg, aTeardrop->GetPosition(), DRCE_TEARDROP_TRACK_ERROR);
            }
            else
                m_Parent->DRC_AddMarker(aTeardrop, nullptr, aTeardrop->GetPosition(), DRCE_TEARDROP_TRACK_ERROR);
    
            //Multiple teardrops.
            NET_SCAN_MULTI_TEARDROPS multi_tears = NET_SCAN_MULTI_TEARDROPS(aTeardrop, this);
            multi_tears.Execute();
            if(multi_tears.GetResult())
            {
                m_Parent->DRC_AddMarker(aTeardrop, multi_tears.GetResult(), aTeardrop->GetPosition(), DRCE_TEARDOPS_MULTIPLE);
            }
            
        }
    }
}

TEARDROPS::NET_SCAN_MULTI_TEARDROPS::NET_SCAN_MULTI_TEARDROPS(const TEARDROP* aTeardrop, const TEARDROPS* aParent) : NET_SCAN_BASE(aTeardrop, aParent)
{
    m_result_teardrop = nullptr;    
}

bool TEARDROPS::NET_SCAN_MULTI_TEARDROPS::ExecuteAt(const TRACK* aTrackSeg)
{
    if(aTrackSeg && (aTrackSeg->Type() == PCB_TEARDROP_T))
    {
        if(m_net_start_seg->Type() == PCB_TEARDROP_T)
            if(aTrackSeg != m_net_start_seg)
                if(dynamic_cast<TEARDROP*>(const_cast<TRACK*>(aTrackSeg))->GetTrackSeg() == dynamic_cast<TEARDROP*>(m_net_start_seg)->GetTrackSeg())
                    if(aTrackSeg->GetEnd() == m_net_start_seg->GetEnd())
                    {
                        m_result_teardrop = dynamic_cast<TEARDROP*>(const_cast<TRACK*>(aTrackSeg));
                        return true;
                    }
    }
    return false;
}
  
  
//-----------------------------------------------------------------------------------------------------/
// WARNINGS
//-----------------------------------------------------------------------------------------------------/
void TEARDROPS::MarkWarnings(const DLIST<MODULE>* aModulesAt, DRC* aDRC)
{
    std::unique_ptr<MODULES_PROGRESS_MARK_WARNINGS> warning_marks(new MODULES_PROGRESS_MARK_WARNINGS(this, aModulesAt, aDRC));
    if(warning_marks)
        warning_marks->Execute();
}

TEARDROPS::MODULES_PROGRESS_MARK_WARNINGS::MODULES_PROGRESS_MARK_WARNINGS(const TEARDROPS* aParent, const DLIST<MODULE>* aModules, DRC* aDRC) : TEARDROPS_MODULES_PROGRESS(aParent, aModules, nullptr)
{
    m_DRC = aDRC;
    aParent->GetBoard()->DeleteMARKERs();
    m_progress_title.Printf(_("Searching Warnings of Teardrops of Footprints"));
}

TEARDROPS::MODULES_PROGRESS_MARK_WARNINGS::~MODULES_PROGRESS_MARK_WARNINGS()
{
    if(m_cancelled)
        m_Parent->GetBoard()->DeleteMARKERs();
}

uint TEARDROPS::MODULES_PROGRESS_MARK_WARNINGS::DoAtPad(const D_PAD* aPadAt)
{
    uint num_marks = 0;
    if(aPadAt)
    {
        for(uint n = 0; n < aPadAt->m_TracksConnected.size(); ++n)
        {
            TRACK* track_seg = static_cast<TRACK*>(aPadAt->m_TracksConnected.at(n));
            bool same_seg = false;
            for(uint m = 0 ; m < n; ++m)
                if(aPadAt->m_TracksConnected.at(m) == track_seg)
                    same_seg = true;
            wxPoint pad_pos = aPadAt->GetPosition();
            if(!same_seg)
            {
                if((track_seg->Type() == PCB_TRACE_T) && ((track_seg->GetEnd() == pad_pos) || (track_seg->GetStart() == pad_pos)))
                {
                    TEARDROP* tear = dynamic_cast<TEARDROP*>(m_Parent->Get(track_seg, pad_pos));
                    if(tear)
                    {
                        if(!tear->IsLocked()) //Warn only unlocked.
                        {
                            //Trimmed
                            if(m_Parent->IsTrimmed(tear))
                            {
                                m_Parent->GetParent()->DRC_AddMarker(tear, nullptr, tear->GetPosition(), DRCE_TEARDROP_TRIMMED);
                                ++num_marks;
                            }
                            
                            if(dynamic_cast<TEARDROP_PAD*>(tear))
                            {
                                //Useless                      
                                if(!dynamic_cast<TEARDROP_PAD*>(tear)->IsAngleOK() && (tear->GetShape() != TEARDROP::ZERO_T))
                                {
                                    m_Parent->GetParent()->DRC_AddMarker(tear, nullptr, tear->GetPosition(), DRCE_TEARDROP_USELESS);
                                    ++num_marks;
                                }
                                //Too big to PAD.
                                if(tear->GetParams().width_ratio > 100)
                                {
                                    m_Parent->GetParent()->DRC_AddMarker(tear, nullptr, tear->GetPosition(), DRCE_TEARDROP_BIG);
                                    ++num_marks;
                                }
                            }
                            
                            //If same track segments teardros are inside each other.
                            wxPoint opp_seg_pos = track_seg->GetEnd();
                            if(track_seg->GetEnd() == tear->GetEnd())
                                opp_seg_pos = track_seg->GetStart();
                            TEARDROP* opp_tear = dynamic_cast<TEARDROP*>(m_Parent->Get(track_seg, opp_seg_pos));
                            if(opp_tear)
                            {
                                if(opp_tear->GetShape() != TEARDROP::ZERO_T)
                                {
                                    int tears_lengths = tear->GetRealLength();
                                    if(tear->GetShape() == TEARDROP::TEARDROP_T)
                                        tears_lengths -= track_seg->GetWidth()>>1;
                                    tears_lengths += opp_tear->GetRealLength();
                                    if(opp_tear->GetShape() == TEARDROP::TEARDROP_T)
                                        tears_lengths -= track_seg->GetWidth()>>1;
                                    if(track_seg->GetLength() < tears_lengths)
                                    {
                                        m_Parent->GetParent()->DRC_AddMarker(tear, opp_tear, tear->GetPosition(), DRCE_TEARDROPS_INSIDE);
                                        ++num_marks;
                                    }
                                }
                            }
                        }
                    }
                    else
                    {
                        m_Parent->GetParent()->DRC_AddMarker(track_seg, aPadAt, pad_pos, DRCE_TEARDROP_MISSING);
                        ++num_marks;
                    }
                }
                else
                {
                    //If pad connection not in center.
                    if(track_seg->Type() == PCB_TRACE_T) // Only track segments.
                    {
                        for(int m = 0; m < 2; ++m)
                        {
                            wxPoint pos;
                            (m)? pos = track_seg->GetEnd() : pos = track_seg->GetStart();
                            BOARD_CONNECTED_ITEM* at_item = m_Parent->m_Board->GetLockPoint(pos, track_seg->GetLayerSet());
                            if(at_item && (at_item->Type() == PCB_PAD_T))
                            {
                                if(at_item->GetPosition() == pad_pos)
                                {
                                    Tracks_Container tracks_list;
                                    Collect(track_seg, pos, tracks_list);
                                    if(!tracks_list.size())
                                    {
                                        if(!m_Parent->Get(track_seg, pos)) //If teardrop do not mark.
                                        {
                                            m_Parent->GetParent()->DRC_AddMarker(track_seg, at_item, pos, DRCE_TEARDROP_IMPOSSIBLE);
                                            ++num_marks;
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    return num_marks;
}

void TEARDROPS::MarkWarnings(const DLIST<TRACK>* aTracksAt, const TEARDROPS::TEARDROPS_TYPE_TODO aTypeToDo, DRC* aDRC)
{
    std::unique_ptr<TRACKS_PROGRESS_MARK_WARNINGS> warning_marks(new TRACKS_PROGRESS_MARK_WARNINGS(this, aTracksAt, aTypeToDo, aDRC));
    if(warning_marks)
        warning_marks->Execute();
}

TEARDROPS::TRACKS_PROGRESS_MARK_WARNINGS::TRACKS_PROGRESS_MARK_WARNINGS(const TEARDROPS* aParent, const DLIST<TRACK>* aTracks, const TEARDROPS::TEARDROPS_TYPE_TODO aTypeToDo, DRC* aDRC) : TEARDROPS_TRACKS_PROGRESS(aParent, aTracks, nullptr)
{
    m_DRC = aDRC;
    m_type_todo = aTypeToDo;
    if(aTypeToDo != TEARDROPS::ALL_TYPES_T)
        aParent->GetBoard()->DeleteMARKERs();
    switch(aTypeToDo)
    {
        case ONLY_TEARDROPS_T:
            m_progress_title.Printf(_("Searching Warnings of Teardrops of Vias"));
            break;
        case ONLY_TJUNCTIONS_T:
            m_progress_title.Printf(_("Searching Warnings of T-Junctions"));
            break;
        case ONLY_JUNCTIONS_T:
            m_progress_title.Printf(_("Searching Warnings of Junctions"));
            break;
        default:
            m_progress_title.Printf(_("Searching Warnings of: Teardrops of Vias, T-Junctions, Junctions"));
    }
}

TEARDROPS::TRACKS_PROGRESS_MARK_WARNINGS::~TRACKS_PROGRESS_MARK_WARNINGS()
{
    if(m_cancelled)
        m_Parent->GetBoard()->DeleteMARKERs();
}

uint TEARDROPS::TRACKS_PROGRESS_MARK_WARNINGS::ExecuteItem(const BOARD_ITEM* aItemAt)
{
    uint num_marks = 0;
    TRACK* track_seg = dynamic_cast<TRACK*>(const_cast<BOARD_ITEM*>(aItemAt));
    if(track_seg)
    {
        //Segment is teardrop, junction or T- junction.
        if((track_seg->Type() == PCB_TEARDROP_T) && (!track_seg->IsLocked()))
        {
            TEARDROP* tear = static_cast<TEARDROP*>(track_seg);
            if(tear->GetShape() != TEARDROP::ZERO_T) //Do not check zero shape tears.
            {
                TRACK* tears_track_seg = tear->GetTrackSeg();
                
                //Trimmed
                if(m_Parent->IsTrimmed(tear))
                {
                    int warning_code = 0;
                    if(dynamic_cast<TEARDROP_JUNCTIONS*>(tear))
                    {
                        bool isT = dynamic_cast<TEARDROP_JUNCTIONS*>(tear)->Is_T_Junction();
                        if((m_type_todo == ALL_TYPES_T) || (isT && (m_type_todo == ONLY_TJUNCTIONS_T)) || (!isT && (m_type_todo == ONLY_JUNCTIONS_T)))
                            warning_code = DRCE_TEARDROP_TRIMMED;
                    }
                    else
                        if(!dynamic_cast<TEARDROP_PAD*>(tear))
                        {
                            if((m_type_todo == ALL_TYPES_T) || (m_type_todo == ONLY_TEARDROPS_T))
                                warning_code = DRCE_TEARDROP_TRIMMED;
                        }
                    if(warning_code)
                    {
                        m_Parent->GetParent()->DRC_AddMarker(tear, nullptr, tear->GetPosition(), warning_code);
                        ++num_marks;
                    }
                }
                
                //Junctions & T-junctions track segments.
                if(dynamic_cast<TEARDROP_JUNCTIONS*>(tear))
                {
                    TRACK* track_next = dynamic_cast<TEARDROP_JUNCTIONS*>(tear)->Get_T_SegNext(); 
                    TRACK* track_back = dynamic_cast<TEARDROP_JUNCTIONS*>(tear)->Get_T_SegBack();
                    if(track_next && track_back)
                    {
                        //T-Junctions track segment test.
                        if((m_type_todo == ALL_TYPES_T) || (m_type_todo == ONLY_TJUNCTIONS_T))
                        {
                            //Segments width.
                            if((track_back->GetWidth() < tears_track_seg->GetWidth()) || (track_next->GetWidth() < tears_track_seg->GetWidth()) || (track_back->GetWidth() != track_next->GetWidth()))
                            {
                                m_Parent->GetParent()->DRC_AddMarker(tear, nullptr, tear->GetPosition(), DRCE_JUNCTION_TRACK_SEGS_WIDTH);
                                ++num_marks;
                            }
                        }
                    }
                    else
                    {
                        Tracks_Container tracks_list;
                        Collect(tears_track_seg, tear->GetStart(), tracks_list);
                        //Junctions track segment test.
                        if((m_type_todo == ALL_TYPES_T) || (m_type_todo == ONLY_JUNCTIONS_T))
                        {
                            //Do not check inside pad.
                            BOARD_CONNECTED_ITEM* in_item = m_Parent->m_Board->GetLockPoint(tear->GetEnd(), tear->GetLayerSet());
                            bool item_not_pad = false;
                            if(in_item)
                                if(in_item->Type() != PCB_PAD_T)
                                    item_not_pad = true;
                                
                            if(!in_item || item_not_pad)
                            {
                                bool er = false;
                                if(tracks_list.size())
                                {
                                    if(!(GetMaxWidth(tracks_list) > dynamic_cast<TEARDROP_JUNCTIONS*>(tear)->GetTrackSeg()->GetWidth()))
                                    {
                                        er = true;
                                    }
                                }
                                else
                                    er = true;
                                if(er)
                                {
                                    m_Parent->GetParent()->DRC_AddMarker(tear, nullptr, tear->GetPosition(), DRCE_JUNCTION_TRACK_SEGS_WIDTH);
                                    ++num_marks;
                                }
                            }
                        }
                        if((m_type_todo == ALL_TYPES_T) || (m_type_todo == ONLY_TJUNCTIONS_T))
                        {
                            //T-Segments angle.
                            if(tracks_list.size() > 1)
                            {
                                m_Parent->GetParent()->DRC_AddMarker(tear, nullptr, tear->GetPosition(), DRCE_TJUNCTION_ANGLE);
                                ++num_marks;
                            }
                        }
                    }
                }
                else //Only via teardrops.
                {
                    if(!dynamic_cast<TEARDROP_PAD*>(tear))
                    {
                        if((m_type_todo == ALL_TYPES_T) || (m_type_todo == ONLY_TEARDROPS_T))
                        {
                            //Too big to via.
                            if(tear->GetParams().width_ratio > 100)
                            {
                                m_Parent->GetParent()->DRC_AddMarker(tear, nullptr, tear->GetPosition(), DRCE_TEARDROP_BIG);
                                ++num_marks;
                            }
                        }
                    }
                    
                }
                
                //If same track segments teardros are inside each other.
                wxPoint opp_seg_pos = tears_track_seg->GetEnd();
                if(tears_track_seg->GetEnd() == tear->GetEnd())
                    opp_seg_pos = tears_track_seg->GetStart();
                TEARDROP* opp_tear = dynamic_cast<TEARDROP*>(m_Parent->Get(tears_track_seg, opp_seg_pos));
                if(opp_tear)
                {
                    if(opp_tear->GetShape() != TEARDROP::ZERO_T)
                    {
                        int tears_lengths = tear->GetRealLength();
                        if(tear->GetShape() == TEARDROP::TEARDROP_T)
                            tears_lengths -= tears_track_seg->GetWidth()>>1;
                        tears_lengths += opp_tear->GetRealLength();
                        if(opp_tear->GetShape() == TEARDROP::TEARDROP_T)
                            tears_lengths -= tears_track_seg->GetWidth()>>1;
                        if(tears_track_seg->GetLength() < tears_lengths)
                        {
                            int warning_code = 0;
                            if(dynamic_cast<TEARDROP_JUNCTIONS*>(tear))
                            {
                                bool isT = dynamic_cast<TEARDROP_JUNCTIONS*>(tear)->Is_T_Junction();
                                if((m_type_todo == ALL_TYPES_T) || (isT && (m_type_todo == ONLY_TJUNCTIONS_T)) || (!isT && (m_type_todo == ONLY_JUNCTIONS_T)))
                                    warning_code = DRCE_TEARDROPS_INSIDE;
                            }
                            else
                                if(!dynamic_cast<TEARDROP_PAD*>(tear))
                                {
                                    if((m_type_todo == ALL_TYPES_T) || (m_type_todo == ONLY_TEARDROPS_T))
                                        warning_code = DRCE_TEARDROPS_INSIDE;
                                }
                            if(warning_code)
                            {
                                m_Parent->GetParent()->DRC_AddMarker(tear, opp_tear, tear->GetPosition(), warning_code);
                                ++num_marks;
                            }
                        }
                    }
                }   
            }
            
        }
    
        //Segment is trace.
        if(track_seg->Type() == PCB_TRACE_T)
        {
            for(int n = 0; n < 2; ++n) 
            {
                wxPoint track_pos;
                (n)? track_pos = track_seg->GetStart() : track_pos = track_seg->GetEnd();
                VIA* via = m_Parent->GetParent()->GetVia(track_seg, track_pos);
                D_PAD* pad = m_Parent->GetParent()->GetPad(track_seg, track_pos);

                //Missing Teardrop at via.
                if(via)
                {
                    if((m_type_todo == ALL_TYPES_T) || (m_type_todo == ONLY_TEARDROPS_T))
                        if(!m_Parent->Get(track_seg, via->GetEnd()))
                        {
                            m_Parent->GetParent()->DRC_AddMarker(track_seg, static_cast<BOARD_CONNECTED_ITEM*>(via), track_pos, DRCE_TEARDROP_MISSING);
                            ++num_marks;
                        }
                }
                else
                {
                    if(!pad)
                    {
                        if(!m_Parent->Get(track_seg, track_pos))
                        {
                            TRACK* track_next = nullptr;
                            TRACK* track_back = nullptr;
                            bool t_tracks = Find_T_Tracks(track_seg, track_pos, track_next, track_back);

                            //Missing T- Junction.
                            if(t_tracks)
                            {
                                if((m_type_todo == ALL_TYPES_T) || (m_type_todo == ONLY_TJUNCTIONS_T))
                                {
                                    TRACK* t_track_next = nullptr;
                                    TRACK* t_track_back = nullptr;
                                    bool t_next = Find_T_Tracks(track_next, track_pos, t_track_next, t_track_back);
                                    bool t_back = Find_T_Tracks(track_back, track_pos, t_track_next, t_track_back);
                                    if(t_next && t_back)
                                    {
                                        if(!m_Parent->Get(track_next, track_pos))
                                            t_next = false;
                                        if(!m_Parent->Get(track_back, track_pos))
                                            t_back = false;
                                    }
                                    if(!(t_next && t_back))
                                    {
                                        m_Parent->GetParent()->DRC_AddMarker(track_seg, static_cast<BOARD_CONNECTED_ITEM*>(track_next), track_pos, DRCE_TJUNCTION_MISSING);
                                        ++num_marks;
                                    }
                                }
                            }
                            else
                            {
                                //Missing Junction.
                                if((m_type_todo == ALL_TYPES_T) || (m_type_todo == ONLY_JUNCTIONS_T))
                                {
                                    Tracks_Container tracks_list;
                                    Collect(track_seg, track_pos, tracks_list);
                                    if(tracks_list.size())
                                    {
                                        if(GetMaxWidth(tracks_list) > track_seg->GetWidth())
                                        {
                                            m_Parent->GetParent()->DRC_AddMarker(track_seg, static_cast<BOARD_CONNECTED_ITEM*>(GetMaxWidthTrack(tracks_list)), track_pos, DRCE_JUNCTION_MISSING);
                                            ++num_marks;
                                        }
                                    }
                                }
                            }
                        
                            //If track to via connection is not at center.
                            if((m_type_todo == ALL_TYPES_T) || (m_type_todo == ONLY_TEARDROPS_T))
                            {
                                Tracks_Container result_list;
                                m_Parent->GetBoard()->TrackItems()->GetBadConnectedVia(track_seg, track_pos, &result_list);
                                for(TRACK* track_n : result_list)
                                {
                                    m_Parent->GetParent()->DRC_AddMarker(track_seg, track_n, track_pos, DRCE_TEARDROP_IMPOSSIBLE);
                                    ++num_marks;
                                }
                            }
                            
                        }
                    }
                }
            }
        }   
    }
    return num_marks;
}
