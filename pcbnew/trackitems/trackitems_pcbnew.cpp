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

#include"trackitems.h"


using namespace TrackNodeItem;
using namespace TrackNodeItems;


//Eliminate tracknode items when collect tracks to drag.
TRACK* TRACKITEMS::DragKeepSlopeSegmentTypeCheck(TRACK* aTrackToPoint, const TRACK* aTrackSeg, const TRACK* aTrace, const ENDPOINT_T aEndpoint)
{
    TRACK* cur_track = aTrackToPoint;
    if(cur_track && aTrackSeg && aTrace)
    {
        if(dynamic_cast<TrackNodeItem::TRACKNODEITEM*>(cur_track))
        {
            cur_track->SetState( BUSY, true );
            TRACK* track_at_point = const_cast<TRACK*>(aTrackSeg)->GetTrack(const_cast<TRACK*>(aTrace), NULL, aEndpoint, true, false);
            if(track_at_point) 
            {
                if(dynamic_cast<TrackNodeItem::TRACKNODEITEM*>(track_at_point))
                {
                    track_at_point->SetState( BUSY, true );
                    TRACK* last_check_track_at_point = const_cast<TRACK*>(aTrackSeg)->GetTrack(const_cast<TRACK*>(aTrace), NULL, aEndpoint, true, false);
                    if(last_check_track_at_point)
                    {
                        cur_track->SetState( BUSY, false );
                        cur_track = last_check_track_at_point;
                    }
                    track_at_point->SetState( BUSY, false );
                }
                else
                {
                    cur_track->SetState( BUSY, false );
                    cur_track = track_at_point;
                }
            }
            cur_track->SetState( BUSY, false );
        }
    }
    return cur_track;
}

bool TRACKITEMS::DragKeepSlopeSegmentsNumCheck(const bool aCurrentError, TRACK* aTrackToPoint, const TRACK *aTrackSeg, const TRACK *aTrace, const ENDPOINT_T aEndpoint)
{
    bool return_error = aCurrentError;
    bool error = false;
    if(aTrackToPoint && aTrackSeg && aTrace)
    {
        if((aTrackToPoint->Type() == PCB_VIA_T))//|| aTrackSeg->m_PadsConnected.size())
            error = true;
        else
        {
            aTrackToPoint->SetState( BUSY, true );
            TRACK* track_at_point = const_cast<TRACK*>(aTrackSeg)->GetTrack(const_cast<TRACK*>(aTrace), NULL, aEndpoint, true, false);
            if(track_at_point)
            {
                if(dynamic_cast<TrackNodeItem::TRACKNODEITEM*>(track_at_point))
                {
                    track_at_point->SetState(BUSY, true);
                    TRACK* another_track_at_point = const_cast<TRACK*>(aTrackSeg)->GetTrack(const_cast<TRACK*>(aTrace), NULL, aEndpoint, true, false);
                    if(another_track_at_point) 
                    {
                        if(dynamic_cast<TrackNodeItem::TRACKNODEITEM*>(another_track_at_point))
                        {
                            another_track_at_point->SetState( BUSY, true );
                            TRACK* last_check_track_at_point = const_cast<TRACK*>(aTrackSeg)->GetTrack(const_cast<TRACK*>(aTrace), NULL, aEndpoint, true, false);
                            if(last_check_track_at_point)
                                error = true;
                            another_track_at_point->SetState(BUSY, false);
                        }
                        else
                            error = true;
                    }
                    track_at_point->SetState(BUSY, false);
                }
                else
                    error = true;
            }
            aTrackToPoint->SetState(BUSY, false);
        }
    }
    if(error)
        return_error = true;
    return return_error;
}

//------------------------------------------------------------------------------------------------------------------


//-----------------------------------------------------------------------------------------------------/
// SHARP ANGLES 
//-----------------------------------------------------------------------------------------------------/
void TRACKITEMS::MarkSharpAngles(const DLIST<TRACK>* aTracksAt, DRC* aDRC)
{
    std::unique_ptr<TRACKS_PROGRESS_MARK_SHARP_ANGLES> sharp_marks(new TRACKS_PROGRESS_MARK_SHARP_ANGLES(this, aTracksAt, aDRC));
    if(sharp_marks)
        sharp_marks->Execute();
}

TRACKITEMS::TRACKS_PROGRESS_MARK_SHARP_ANGLES::TRACKS_PROGRESS_MARK_SHARP_ANGLES(const TRACKITEMS* aParent, const DLIST<TRACK>* aTracks, DRC* aDRC) : TRACKITEMS_TRACKS_PROGRESS(aParent, aTracks, nullptr)
{
    marked_tracks_sharp_angle.clear();
    aParent->GetBoard()->DeleteMARKERs();
    m_progress_title.Printf(_("Searching Sharp Angles of Tracks"));
}

uint TRACKITEMS::TRACKS_PROGRESS_MARK_SHARP_ANGLES::ExecuteItem(const BOARD_ITEM* aItemAt)
{
    uint num_marks = 0;
    TRACK* track_seg = dynamic_cast<TRACK*>(const_cast<BOARD_ITEM*>(aItemAt));
    if(track_seg && (track_seg->Type() == PCB_TRACE_T))
    {
        for(int n = 0; n < 2; ++n)
        {
            VIA* via = nullptr;
            D_PAD* pad = nullptr;
            (n)? via = m_Parent->BackVia(track_seg) : 
                 via = m_Parent->NextVia(track_seg);
            (n)? pad = m_Parent->BackPad(track_seg) : 
                 pad = m_Parent->NextPad(track_seg);
            if(!via && !pad)
            {
                //Do not mark T tracs
                TRACK* track_next = nullptr;
                TRACK* track_back = nullptr;
                wxPoint track_pos;
                (n)? track_pos = track_seg->GetStart() : track_pos = track_seg->GetEnd();
                
                //Do not check inside pad.
                BOARD_CONNECTED_ITEM* in_item = m_Parent->m_Board->GetLockPoint(track_pos, track_seg->GetLayerSet());
                bool item_not_pad = false;
                if(in_item)
                    if(in_item->Type() != PCB_PAD_T)
                        item_not_pad = true;
                
                if(!in_item || item_not_pad)
                {
                    //T tracks.
                    bool t_tracks = Find_T_Tracks(track_seg, track_pos, track_next, track_back);
                    if(t_tracks)
                    {
                        if(m_Parent->Teardrops()->Get(track_seg, track_pos) || (m_Parent->Teardrops()->Get(track_next, track_pos) && m_Parent->Teardrops()->Get(track_back, track_pos)))
                            break;
                    }
                    if(!t_tracks && (track_back || track_next))
                    {
                        TRACK* track_90 = nullptr;
                        (track_back)? track_90 = track_back : track_90 = track_next;
                        if(Find_T_Tracks(track_90, track_pos, track_next, track_back))
                            if(m_Parent->Teardrops()->Get(track_90, track_pos))
                                break;
                    }
                    
                    //do not care if locked junctions in multiple connections
                    bool synapse = false;
                    Tracks_Container tracks_list;
                    Collect(track_seg, track_pos, tracks_list);
                    tracks_list.insert(track_seg);
                    if(tracks_list.size() > 2)
                    {
                        synapse = true;
                        for(TRACK* trck_sg : tracks_list)
                        {
                            TEARDROP* tear = dynamic_cast<TEARDROP*>(m_Parent->Teardrops()->Get(trck_sg, track_pos));
                            if(!tear)
                                synapse = false;
                            else
                                if(!tear->IsLocked())
                                    synapse = false;
                        }
                        if(synapse)
                            break;
                    }

                    //Do not mark if already marked.
                    bool already_marked = false;
                    for(wxPoint pos : marked_tracks_sharp_angle)
                        if(pos == track_pos)
                        {
                            already_marked = true;
                            break;
                        }
                    if(!already_marked)
                    {
                        TRACK * sharp_track = Find_Tracks_Sharp_Angle(track_seg, track_pos);
                        if(sharp_track)
                        {
                            m_Parent->DRC_AddMarker(track_seg, sharp_track, track_pos, DRCE_TRACKNODEITEM_UNSPECIFIED);                       
                            ++num_marks;
                            marked_tracks_sharp_angle.push_back(track_pos);
                        }
                    }
                }
            }
        }
    }   
    return num_marks;
}

TRACKITEMS::NET_SCAN_SHARP_ANGLES::NET_SCAN_SHARP_ANGLES(const TRACK* aTrackSeg, const TRACKITEMS* aParent, const wxPoint aPos) : NET_SCAN_BASE(aTrackSeg, aParent)
{
    m_layer_id = m_net_start_seg->GetLayer();
    m_pos = aPos;
    m_result_track = nullptr;
}

bool TRACKITEMS::NET_SCAN_SHARP_ANGLES::ExecuteAt(const TRACK* aTrackSeg)
{
    if((aTrackSeg != m_net_start_seg) && (aTrackSeg->Type() == PCB_TRACE_T) && aTrackSeg->IsOnLayer(m_layer_id))
    {
        if((aTrackSeg->GetStart() == m_pos) || (aTrackSeg->GetEnd() == m_pos))
        {
            if(IsSharpAngle(aTrackSeg, m_net_start_seg, m_pos, m_Parent->RoundedTracksCorners()->Get(aTrackSeg, m_pos, true)))
            {
                m_result_track = const_cast<TRACK*>(aTrackSeg);
                return true;
            }
        }
    }
    return false;
}

//Return first founded track.
TRACK* TRACKITEMS::TRACKS_PROGRESS_MARK_SHARP_ANGLES::Find_Tracks_Sharp_Angle(const TRACK* aTrackSegAt, const wxPoint aPosAt) const
{
    TRACK* result_track = nullptr;
    if(aTrackSegAt && (aTrackSegAt->Type() == PCB_TRACE_T))
    {
        std::unique_ptr<NET_SCAN_SHARP_ANGLES> track(new NET_SCAN_SHARP_ANGLES(aTrackSegAt, m_Parent, aPosAt));
        if(track)
        {
            track->Execute();
            result_track = track->GetResult();
        }
    }
    return result_track;
}

//-----------------------------------------------------------------------------------------------------/
// Adjust track connections in center of item.
//-----------------------------------------------------------------------------------------------------/
// and mark them with markers to see what have been done.
void TRACKITEMS::FixTrackConnectionsInCenter(const DLIST<TRACK>* aTracksAt)
{
    PICKED_ITEMS_LIST undoredo_list;
    std::unique_ptr<TRACKS_PROGRESS_FIX_ITEM_CONNECTION> fix_connection(new TRACKS_PROGRESS_FIX_ITEM_CONNECTION(this, aTracksAt, &undoredo_list));
    if(fix_connection)
    {
        fix_connection->Execute();
        if(undoredo_list.GetCount())
            m_EditFrame->SaveCopyInUndoList(undoredo_list, UR_CHANGED);
    }
}

TRACKITEMS::TRACKS_PROGRESS_FIX_ITEM_CONNECTION::TRACKS_PROGRESS_FIX_ITEM_CONNECTION(const TRACKITEMS* aParent, 
                                                                                    const DLIST<TRACK>* aTracks, 
                                                                                    PICKED_ITEMS_LIST* aUndoRedoList) : 
    TRACKITEMS_TRACKS_PROGRESS(aParent, aTracks, aUndoRedoList)
{
    m_progress_title.Printf(_("Fixing Pad / Via Connections of Tracks"));
    
    m_current_shape = m_Parent->Teardrops()->GetCurrentShape();
    m_teardrop_params = m_Parent->Teardrops()->GetShapeParams(TEARDROP::TEARDROP_T);
    m_fillet_params = m_Parent->Teardrops()->GetShapeParams(TEARDROP::FILLET_T);
    m_subland_params = m_Parent->Teardrops()->GetShapeParams(TEARDROP::SUBLAND_T);
    
    aParent->GetBoard()->DeleteMARKERs();
    m_fixed_tracks.clear();
}

TRACKITEMS::TRACKS_PROGRESS_FIX_ITEM_CONNECTION::~TRACKS_PROGRESS_FIX_ITEM_CONNECTION()
{
    m_Parent->Teardrops()->SetShapeParams(m_teardrop_params);
    m_Parent->Teardrops()->SetShapeParams(m_fillet_params);
    m_Parent->Teardrops()->SetShapeParams(m_subland_params);
    m_Parent->Teardrops()->SetCurrentShape(m_current_shape);
    m_Parent->Teardrops()->RecreateMenu();
    for(auto track_seg : m_fixed_tracks)
        if(dynamic_cast<ROUNDEDCORNERTRACK*>(track_seg))
            m_Parent->RoundedTracksCorners()->Update(track_seg);
}

uint TRACKITEMS::TRACKS_PROGRESS_FIX_ITEM_CONNECTION::ExecuteItem(const BOARD_ITEM* aItemAt)
{
    uint exec_count = 0;
    TRACK* track_seg = dynamic_cast<TRACK*>(const_cast<BOARD_ITEM*>(aItemAt));
    if(track_seg && (track_seg->Type() == PCB_TRACE_T))
    {
        ITEM_PICKER picker(track_seg, UR_CHANGED);
        for(int n = 0; n < 2; ++n)
        {
            VIA* via = nullptr;
            D_PAD* pad = nullptr;
            (n)? via = m_Parent->BackVia(track_seg) : via = m_Parent->NextVia(track_seg);
            (n)? pad = m_Parent->BackPad(track_seg) : pad = m_Parent->NextPad(track_seg);
            if(!via && !pad) //If not connected.
            {
                wxPoint track_pos;
                (n)? track_pos = track_seg->GetStart() : track_pos = track_seg->GetEnd();
                
                TEARDROP* junc = dynamic_cast<TEARDROP*>(m_Parent->Teardrops()->Get(track_seg, track_pos));
                
                //Check inside Pad.
                BOARD_CONNECTED_ITEM* in_item = m_Parent->m_Board->GetLockPoint(track_pos, track_seg->GetLayerSet());
                if(in_item)
                {
                    if(in_item->Type() == PCB_PAD_T)
                    {
                        if(junc)
                            m_Parent->Teardrops()->Remove(junc, m_undoredo_items, true);
                        
                        picker.SetLink(track_seg->Clone());
                        m_undoredo_items->PushItem(picker);
                        
                        wxPoint item_pos = in_item->GetPosition();
                        (n)? track_seg->SetStart(item_pos) : track_seg->SetEnd(item_pos);
                        
                        if(junc)
                        {
                            TEARDROP::PARAMS junc_params = junc->GetParams();
                            m_Parent->Teardrops()->SetShapeParams(junc_params);
                            m_Parent->Teardrops()->SetCurrentShape(junc_params.shape);
                            m_Parent->Teardrops()->Add(track_seg, in_item, m_undoredo_items, item_pos);
                        }
                        
                        m_Parent->DRC_AddMarker(track_seg, nullptr, track_pos, DRCE_TRACKNODEITEM_UNSPECIFIED);                       
                        m_fixed_tracks.push_back(track_seg);
                        ++exec_count;
                    }
                }
                //Check inside Via.
                Tracks_Container result_list;
                VIA* via_at = m_Parent->GetBadConnectedVia(track_seg, track_pos, &result_list);
                if(via_at)
                {
                    wxPoint via_pos = via_at->GetEnd();
                    for(TRACK* track_n : result_list)
                    {
                        if(junc)
                            m_Parent->Teardrops()->Remove(junc, m_undoredo_items, true);

                        picker.SetLink(track_seg->Clone());
                        m_undoredo_items->PushItem(picker);
                        
                        (n)? track_seg->SetStart(via_pos) : track_seg->SetEnd(via_pos);

                        if(junc)
                        {
                            TEARDROP::PARAMS junc_params = junc->GetParams();
                            m_Parent->Teardrops()->SetShapeParams(junc_params);
                            m_Parent->Teardrops()->SetCurrentShape(junc_params.shape);
                            m_Parent->Teardrops()->Add(track_seg, via_at, m_undoredo_items, via_pos);
                        }
                        
                        m_Parent->DRC_AddMarker(track_seg, nullptr, track_pos, DRCE_TRACKNODEITEM_UNSPECIFIED);                       
                        m_fixed_tracks.push_back(track_seg);
                        ++exec_count;
                    }
                }
            }
        }
    }
    return exec_count;
}


//This adds track and via menus to copy pointed segemnts and vias values to current.
//When select, values are changed in edit.cpp.
void TRACKITEMS::Popup_PickTrackOrViaWidth(wxMenu* aMenu, const TRACK* aTrackSeg) const
{
    if(aMenu && aTrackSeg)    
    {
        wxString msg;
        if(aTrackSeg->Type() == PCB_VIA_T)
        {
            int via_size = aTrackSeg->GetWidth();
            int via_drill = static_cast<VIA*>(const_cast<TRACK*>(aTrackSeg))->GetDrillValue();
            if((via_size != m_Board->GetDesignSettings().GetCurrentViaSize()) || (via_drill != m_Board->GetDesignSettings().GetCurrentViaDrill()))
            {
                wxString txt_width = StringFromValue(g_UserUnit, via_size, true);
                wxString txt_drill = StringFromValue(g_UserUnit, via_drill, true);
                msg.Printf( _("Copy Via Settings [%s / %s] to Current"), GetChars(txt_width), GetChars(txt_drill));
                AddMenuItem(aMenu, ID_POPUP_PCB_VIA_PICK_SIZE, msg, KiBitmap(width_segment_xpm));
            }
        }
        else
        {
            int track_width = aTrackSeg->GetWidth();
            if(track_width != m_Board->GetDesignSettings().GetCurrentTrackWidth())
            {
                wxString txt_width = StringFromValue(g_UserUnit, track_width, true);
                msg.Printf( _( "Copy Track Width [%s] to Current" ), GetChars(txt_width));
                AddMenuItem(aMenu, ID_POPUP_PCB_TRACK_PICK_WIDTH,msg, KiBitmap(width_track_xpm));
            }
        }
    }
}


