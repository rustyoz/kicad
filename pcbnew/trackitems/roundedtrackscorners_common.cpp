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
#include "teardrops.h"
#include "trackitems.h"

#include <ratsnest_data.h>
#include <view/view.h> //Gal canvas

using namespace TrackNodeItem;

const wxString ROUNDEDTRACKSCORNERS::TXT_ROUNDEDTRACKSCORNERS = _("Rounded Tracks Corners");

ROUNDEDTRACKSCORNERS::ROUNDEDTRACKSCORNERS(const TRACKITEMS* aParent, const BOARD* aBoard) : TRACKNODEITEMS(aParent, aBoard)
{
    m_Board = const_cast<BOARD*>(aBoard);
    m_EditFrame = nullptr;
    m_menu = nullptr;

    m_update_list = new RoundedTracksCorner_Container;
    m_update_list->clear(); 

    m_update_tracks_list = new RoundedCornerTrack_Container;;
    m_update_tracks_list->clear(); 

    m_recreate_list = new RoundedTracksCorner_Container;
    m_recreate_list->clear();

    m_gal_removed_list = new RoundedTracksCorner_Container;
    m_gal_removed_list->clear(); 

    m_to_edit = EDIT_NULL_T;
    m_can_edit = false;
    m_track_edit_corner = nullptr;

    LoadDefaultParams();
}

ROUNDEDTRACKSCORNERS::~ROUNDEDTRACKSCORNERS()
{
    delete m_update_list;
    m_update_list = nullptr;
    
    delete m_update_tracks_list;
    m_update_tracks_list = nullptr;
    
    delete m_recreate_list;
    m_recreate_list = nullptr;

    delete m_gal_removed_list;
    m_gal_removed_list = nullptr;    
}

ROUNDEDTRACKSCORNER* ROUNDEDTRACKSCORNERS::Create(const TRACK* aTrackSegTo, const TRACK* aTrackSegSecond, const wxPoint aPosition, const bool aNullTrackCheck)
{
    ROUNDEDTRACKSCORNER* corner = nullptr;

    if(aTrackSegTo && aTrackSegSecond)
    {
        if(dynamic_cast<ROUNDEDCORNERTRACK*>(const_cast<TRACK*>(aTrackSegTo)) && dynamic_cast<ROUNDEDCORNERTRACK*>(const_cast<TRACK*>(aTrackSegSecond)))
        {
            //Not in pad or via:
            if(const_cast<TRACK*>(aTrackSegTo)->GetVia( aPosition, aTrackSegTo->GetLayer()))
                return nullptr;
                    
            std::vector<D_PAD*> connected_pads = aTrackSegTo->m_PadsConnected;
                for(auto pad : connected_pads)
                    if(pad->HitTest(aPosition))
                        return nullptr;

            connected_pads = aTrackSegSecond->m_PadsConnected;
                for(auto pad : connected_pads)
                    if(pad->HitTest(aPosition))
                        return nullptr;

            ROUNDEDTRACKSCORNER* trackseg_corner_atpos = nullptr;
            if(aPosition == dynamic_cast<ROUNDEDCORNERTRACK*>(const_cast<TRACK*>(aTrackSegTo))->GetStart())
                trackseg_corner_atpos = dynamic_cast<ROUNDEDCORNERTRACK*>(const_cast<TRACK*>(aTrackSegTo))->GetStartPointCorner();
            if(aPosition == dynamic_cast<ROUNDEDCORNERTRACK*>(const_cast<TRACK*>(aTrackSegTo))->GetEnd())
                trackseg_corner_atpos = dynamic_cast<ROUNDEDCORNERTRACK*>(const_cast<TRACK*>(aTrackSegTo))->GetEndPointCorner();
            
            ROUNDEDTRACKSCORNER* tracksegsecond_corner_atpos = nullptr;
            if(aPosition == dynamic_cast<ROUNDEDCORNERTRACK*>(const_cast<TRACK*>(aTrackSegSecond))->GetStart())
                tracksegsecond_corner_atpos = dynamic_cast<ROUNDEDCORNERTRACK*>(const_cast<TRACK*>(aTrackSegSecond))->GetStartPointCorner();
            if(aPosition == dynamic_cast<ROUNDEDCORNERTRACK*>(const_cast<TRACK*>(aTrackSegSecond))->GetEnd())
                trackseg_corner_atpos = dynamic_cast<ROUNDEDCORNERTRACK*>(const_cast<TRACK*>(aTrackSegSecond))->GetEndPointCorner();
            
            if(!trackseg_corner_atpos && !tracksegsecond_corner_atpos && 
                !m_Parent->Teardrops()->Get(aTrackSegTo, aPosition, true) && 
                !m_Parent->Teardrops()->Get(aTrackSegSecond, aPosition, true))
            {

                ROUNDEDTRACKSCORNER::PARAMS params = GetParams();
                corner = new ROUNDEDTRACKSCORNER(m_Board, aTrackSegTo, aTrackSegSecond, aPosition, params, aNullTrackCheck);

                if(corner)
                {
                    if(!corner->IsCreatedOK())
                    {
                        delete corner;
                        corner = nullptr;
                    }
                    else
                    {
                        m_Parent->Teardrops()->Update(corner->GetTrackSeg());
                        m_Parent->Teardrops()->Update(corner->GetTrackSegSecond());
                        
                        //GAL View.
                        if(m_EditFrame && m_EditFrame->IsGalCanvasActive() )
                        {
                            m_EditFrame->GetGalCanvas()->GetView()->Add(corner);
                            m_EditFrame->GetGalCanvas()->GetView()->Update(corner->GetTrackSeg(), KIGFX::GEOMETRY);
                            m_EditFrame->GetGalCanvas()->GetView()->Update(corner->GetTrackSegSecond(), KIGFX::GEOMETRY);
                            //Update teardrops in GAL
                            std::set<TRACK*> commit_container;
                            m_Parent->Teardrops()->CollectCommit(corner->GetTrackSeg(), &commit_container, true);
                            m_Parent->Teardrops()->CollectCommit(corner->GetTrackSegSecond(), &commit_container, true);
                            for(auto tear : commit_container)
                                if(tear)
                                    m_EditFrame->GetGalCanvas()->GetView()->Update(tear, KIGFX::GEOMETRY);
                        }
                    }
                }
            }
        }
    }
    return corner;
}

void ROUNDEDTRACKSCORNERS::Delete(ROUNDEDTRACKSCORNER* aCorner, DLIST<TRACK>*aTrackListAt, PICKED_ITEMS_LIST* aUndoRedoList)
{
    if(aCorner && aTrackListAt)
    {
        if(aCorner->GetList() == aTrackListAt) 
        {
            ITEM_PICKER picker(nullptr, UR_DELETED);

            TRACK* first_track = aCorner->GetTrackSeg();
            TRACK* second_track = aCorner->GetTrackSegSecond();
            
            //GAL View removing.
            if(m_EditFrame && m_EditFrame->IsGalCanvasActive() )
            {
               GalRemovedListAdd(aCorner);
               m_EditFrame->GetGalCanvas()->GetView()->Remove(aCorner);
            }

            aCorner->ResetVisibleEndpoints();
            aCorner->ReleaseTrackSegs();
            Update(first_track);
            Update(second_track);

            aTrackListAt->Remove(aCorner);
            picker.SetItem(aCorner);
            aUndoRedoList->PushItem(picker);

            m_Parent->Teardrops()->Update(first_track);
            m_Parent->Teardrops()->Update(second_track);
            
            if(m_EditFrame)
            {
                m_EditFrame->GetCanvas()->RefreshDrawingRect(aCorner->GetBoundingBox());
                if( m_EditFrame->IsGalCanvasActive() )
                {
                    m_EditFrame->GetGalCanvas()->GetView()->Update(first_track, KIGFX::GEOMETRY);
                    m_EditFrame->GetGalCanvas()->GetView()->Update(second_track, KIGFX::GEOMETRY);
                    std::set<TRACK*> commit_container;
                    m_Parent->Teardrops()->CollectCommit(first_track, &commit_container, true);
                    m_Parent->Teardrops()->CollectCommit(second_track, &commit_container, true);
                    for(auto tear : commit_container)
                        if(tear)
                            m_EditFrame->GetGalCanvas()->GetView()->Update(tear, KIGFX::GEOMETRY);
                }
            }
        }
    }
}

TRACK* ROUNDEDTRACKSCORNERS::FindSecondTrack(const TRACK* aTrackSegTo, wxPoint aPosition)
{
    if(aTrackSegTo)
    {
        Tracks_Container tracks;
        Collect(aTrackSegTo, aPosition, tracks);
        if(tracks.size() == 1)
        {
            TRACK* track_second = *tracks.begin();
            if(track_second->Type() == PCB_TRACE_T)
                return track_second;
        }
    }
    return nullptr;
}


ROUNDEDCORNERTRACK* ROUNDEDTRACKSCORNERS::ConvertTrackInList(TRACK* aTrack, PICKED_ITEMS_LIST* aUndoRedoList)
{
    ROUNDEDCORNERTRACK* rounded_track = nullptr;
    if(aTrack && aTrack->Type() == PCB_TRACE_T)
    {
        ITEM_PICKER picker_new(nullptr, UR_NEW);
        ITEM_PICKER picker_deleted(nullptr, UR_DELETED);
        ROUNDEDCORNERTRACK* created_track = nullptr;

        if(!dynamic_cast<ROUNDEDCORNERTRACK*>(const_cast<TRACK*>(aTrack)))
        {
            created_track = new ROUNDEDCORNERTRACK(m_Board, const_cast<TRACK*>(aTrack));
            rounded_track = created_track;
        }
        else
            rounded_track = static_cast<ROUNDEDCORNERTRACK*>(const_cast<TRACK*>(aTrack));

        if(created_track)
        {
            //If teardrops save them.
            m_Parent->Teardrops()->ToMemory(aTrack);
            m_Parent->Teardrops()->Remove(aTrack, aUndoRedoList, true);            
            
            //Add created.
            picker_new.SetItem(created_track);
            aUndoRedoList->PushItem(picker_new);
            m_Board->GetRatsnest()->Add(created_track);
            m_Board->m_Track.Insert(created_track, aTrack);
            
            //Remove old.
            picker_deleted.SetItem(aTrack);
            aUndoRedoList->PushItem(picker_deleted);
            m_Board->GetRatsnest()->Remove(aTrack);
            m_Board->Remove(aTrack);

            //If teardrops add them.
            m_Parent->Teardrops()->FromMemory(created_track, aUndoRedoList);
            m_Parent->Teardrops()->Update( created_track->GetNetCode(), created_track);            

            //GAL remove and add
            if(m_EditFrame)
            {
                if( m_EditFrame->IsGalCanvasActive() )
                {
                    m_EditFrame->GetGalCanvas()->GetView()->Remove(aTrack);
                    m_EditFrame->GetGalCanvas()->GetView()->Add(created_track);
                }
            }
        }
        //aTrack = rounded_track;
    }
    return rounded_track;
}

ROUNDEDTRACKSCORNER* ROUNDEDTRACKSCORNERS::Add(TRACK* aTrackSegTo, const wxPoint aPosition, PICKED_ITEMS_LIST* aUndoRedoList)
{
    ROUNDEDTRACKSCORNER* corner = nullptr;
    ITEM_PICKER picker_new(nullptr, UR_NEW);
    if(aTrackSegTo)
    {
        TRACK* track_second = FindSecondTrack(aTrackSegTo, aPosition);
        if(track_second)
        {
            track_second = ConvertTrackInList(track_second, aUndoRedoList);
            aTrackSegTo = ConvertTrackInList(aTrackSegTo, aUndoRedoList);
            
            corner = Create(aTrackSegTo, track_second, aPosition, false);

            if(corner)
            {
                DLIST<TRACK>* tracks_list = static_cast<DLIST<TRACK>*>(aTrackSegTo->GetList());
                tracks_list->Insert(corner, corner->GetTrackSeg());

                picker_new.SetItem(corner);
                aUndoRedoList->PushItem(picker_new);
                
                m_Parent->Teardrops()->Update(corner->GetTrackSeg());
                m_Parent->Teardrops()->Update(corner->GetTrackSegSecond());
                
                if(m_EditFrame)
                {
                    m_EditFrame->GetCanvas()->RefreshDrawingRect(corner->GetBoundingBox());
                }
            }
        }
    }
    return corner;
}

void ROUNDEDTRACKSCORNERS::Add(TRACK* aTrackSegTo, const wxPoint& aCurPosAt)
{
    if(aTrackSegTo && (aTrackSegTo->Type() == PCB_TRACE_T))
    {
        PICKED_ITEMS_LIST undoredo_items;
        
        uint min_dist = std::numeric_limits<uint>::max();

        uint dist_start = hypot(abs(aTrackSegTo->GetStart().y - aCurPosAt.y) , abs(aTrackSegTo->GetStart().x - aCurPosAt.x));
        if(dist_start < min_dist)
            min_dist = dist_start;
        
        uint dist_end = hypot(abs(aTrackSegTo->GetEnd().y - aCurPosAt.y) , abs(aTrackSegTo->GetEnd().x - aCurPosAt.x));
        wxPoint pos;
        (dist_start < dist_end)? pos = aTrackSegTo->GetStart() : pos = aTrackSegTo->GetEnd();
        
        Add(aTrackSegTo, pos, &undoredo_items);
        if(undoredo_items.GetCount())
            m_EditFrame->SaveCopyInUndoList(undoredo_items, UR_NEW);
    }
}

void ROUNDEDTRACKSCORNERS::Add(TRACK* aTrackSegTo, PICKED_ITEMS_LIST* aUndoRedoList)
{
    if(aTrackSegTo && (aTrackSegTo->Type() == PCB_TRACE_T) && !aTrackSegTo->IsNull())
    {
        for(int n = 0; n < 2; ++n)
        {
            wxPoint pos = aTrackSegTo->GetStart();
            if(n)
                pos = aTrackSegTo->GetEnd();
            
            Add(aTrackSegTo, pos, aUndoRedoList);
        }
    }
}

void ROUNDEDTRACKSCORNERS::Add(TRACK* aTrackSegTo)
{
    PICKED_ITEMS_LIST null_undo_redo_list;
    Add(aTrackSegTo, &null_undo_redo_list);
}

ROUNDEDTRACKSCORNERS::NET_SCAN_NET_ADD::NET_SCAN_NET_ADD(const int aNet, const ROUNDEDTRACKSCORNERS* aParent, PICKED_ITEMS_LIST* aUndoRedoList) : NET_SCAN_BASE(nullptr, aParent)
{
    m_picked_items = aUndoRedoList;
    DLIST<TRACK>* tracks_list = &m_Parent->GetBoard()->m_Track; 
    m_net_start_seg = tracks_list->GetFirst()->GetStartNetCode(aNet);
}

bool ROUNDEDTRACKSCORNERS::NET_SCAN_NET_ADD::ExecuteAt(const TRACK* aTrackSeg)
{
    if(aTrackSeg->Type() == PCB_TRACE_T)
        dynamic_cast<ROUNDEDTRACKSCORNERS*>(m_Parent)->Add(const_cast<TRACK*>(aTrackSeg), m_picked_items);
    return false;
}

void ROUNDEDTRACKSCORNERS::Add(const int aNetCodeTo, PICKED_ITEMS_LIST* aUndoRedoList)
{
    std::unique_ptr<NET_SCAN_NET_ADD> net(new NET_SCAN_NET_ADD(aNetCodeTo, this, aUndoRedoList));
    if(net)
    {
        net->Execute();
    }
}

void ROUNDEDTRACKSCORNERS::Remove(const TRACK* aTrackItemFrom, const bool aUndo, const bool aLockedToo)
{
    if(aTrackItemFrom)
    {
        PICKED_ITEMS_LIST undoredo_items;
        Remove(aTrackItemFrom, &undoredo_items, aLockedToo);
        if(m_EditFrame && aUndo && undoredo_items.GetCount() )
            m_EditFrame->SaveCopyInUndoList(undoredo_items, UR_DELETED);
    }
}

void ROUNDEDTRACKSCORNERS::Remove(const TRACK* aTrackItemFrom, PICKED_ITEMS_LIST* aUndoRedoList, const bool aLockedToo)
{
    if(aTrackItemFrom)
    {
        if(aTrackItemFrom->Type() == PCB_ROUNDEDTRACKSCORNER_T)
            Remove(static_cast<ROUNDEDTRACKSCORNER*>(const_cast<TRACK*>(aTrackItemFrom)), aUndoRedoList, !CAN_RECREATE, aLockedToo);
        else
            if(aTrackItemFrom->Type() == PCB_TRACE_T)
            {
                ROUNDEDTRACKSCORNER* corner = nullptr;
                for(uint n = 0; n < 2; ++n)
                {
                    n? corner = dynamic_cast<ROUNDEDTRACKSCORNER*>(Next(const_cast<TRACK*>(aTrackItemFrom))) : corner = dynamic_cast<ROUNDEDTRACKSCORNER*>(Back(const_cast<TRACK*>(aTrackItemFrom)));

                    if(corner)
                    {
                        if(aLockedToo || (!aLockedToo && !corner->IsLocked()))
                        {
                            if((corner->GetTrackSeg() == const_cast<TRACK*>(aTrackItemFrom)) || corner->GetTrackSegSecond() == const_cast<TRACK*>(aTrackItemFrom))
                            {
                                DLIST<TRACK>* tracks_list = static_cast<DLIST<TRACK>*>(aTrackItemFrom->GetList());
                                if(tracks_list)
                                {
                                    Delete(corner, tracks_list, aUndoRedoList);
                                }
                            }
                        }
                    }
                }
            }
    }
}

void ROUNDEDTRACKSCORNERS::Remove(ROUNDEDTRACKSCORNER* aCorner, PICKED_ITEMS_LIST* aUndoRedoList, const bool aSaveRemoved, const bool aLockedToo)
{
    if(aCorner)
    {
        if(aLockedToo || (!aLockedToo && !aCorner->IsLocked()))
        {
            DLIST<TRACK>* tracks_list = static_cast<DLIST<TRACK>*>(aCorner->GetTrackSeg()->GetList());
            if(tracks_list)
            {
                if(aSaveRemoved)
                    m_recreate_list->insert(aCorner);

                Delete(aCorner, tracks_list, aUndoRedoList);
            }
        }
    }
}

void ROUNDEDTRACKSCORNERS::Remove(const TRACK* aTrackItemFrom, BOARD_COMMIT& aCommit, const bool aLockedToo)
{
    if(aTrackItemFrom)
    {
        PICKED_ITEMS_LIST undoredo_items;
        Remove(aTrackItemFrom, &undoredo_items, aLockedToo);
        
        uint num_removed_corners = undoredo_items.GetCount();
        if(num_removed_corners)
            for(uint n = 0; n < num_removed_corners; ++n)
                aCommit.Removed(undoredo_items.GetPickedItem(n));
    }
}

ROUNDEDTRACKSCORNERS::NET_SCAN_NET_REMOVE::NET_SCAN_NET_REMOVE(const int aNet, const ROUNDEDTRACKSCORNERS* aParent, PICKED_ITEMS_LIST* aUndoRedoList, RoundedTracksCorner_Container* aRecreateList, const bool aLockedToo) : NET_SCAN_BASE(nullptr, aParent)
{
    m_picked_items = aUndoRedoList;
    m_recreate_list = aRecreateList;   
    m_locked_too = aLockedToo;
    
    DLIST<TRACK>* tracks_list = &m_Parent->GetBoard()->m_Track; 
    m_net_start_seg = tracks_list->GetFirst()->GetStartNetCode(aNet);
}

bool ROUNDEDTRACKSCORNERS::NET_SCAN_NET_REMOVE::ExecuteAt(const TRACK* aTrackSeg)
{
    if(aTrackSeg->Type() == PCB_ROUNDEDTRACKSCORNER_T)
    {
        if(m_locked_too || (!m_locked_too && !aTrackSeg->IsLocked()))
        {
            m_recreate_list->insert(static_cast<ROUNDEDTRACKSCORNER*>(const_cast<TRACK*>(aTrackSeg)));
        }
    }
    return false;
}

//Save removed corners in list, recreating for.
void ROUNDEDTRACKSCORNERS::Remove(const int aNetCodeFrom, PICKED_ITEMS_LIST* aUndoRedoList, const bool aLockedToo)
{
    DLIST<TRACK>* tracks_list = &m_Board->m_Track;
    m_recreate_list->clear();

    std::unique_ptr<NET_SCAN_NET_REMOVE> net(new NET_SCAN_NET_REMOVE(aNetCodeFrom, this, aUndoRedoList, m_recreate_list, aLockedToo));
    if(net)
        net->Execute();
    
    for(ROUNDEDTRACKSCORNER* corner : *m_recreate_list)
    {
        if(corner)
        {
            Delete(corner, tracks_list, aUndoRedoList);
        }
    }
}

void ROUNDEDTRACKSCORNERS::Change(const TRACK* aTrackItemFrom, const bool aUndo, const bool aLockedToo)
{
    if(aTrackItemFrom && aTrackItemFrom->Type() == PCB_ROUNDEDTRACKSCORNER_T)
    {
        PICKED_ITEMS_LIST undoredo_items;
        ROUNDEDTRACKSCORNER* corner = static_cast<ROUNDEDTRACKSCORNER*>(const_cast<TRACK*>(aTrackItemFrom));
        wxPoint pos = corner->GetEnd();
        TRACK* track = corner->GetTrackSeg();

        if(track)
        {
            Remove(corner, &undoredo_items, aLockedToo);
            Add(track, pos, &undoredo_items);
        }
        
        if(m_EditFrame && aUndo && undoredo_items.GetCount() )
            m_EditFrame->SaveCopyInUndoList(undoredo_items, UR_DELETED);
}
}
    
ROUNDEDTRACKSCORNERS::NET_SCAN_NET_RECREATE::NET_SCAN_NET_RECREATE(const int aNet, const ROUNDEDTRACKSCORNERS* aParent, PICKED_ITEMS_LIST* aUndoRedoList, RoundedTracksCorner_Container* aRecreateList) : NET_SCAN_NET_REMOVE(aNet, aParent, aUndoRedoList, aRecreateList, false)
{
    m_current_params = dynamic_cast<ROUNDEDTRACKSCORNERS*>(m_Parent)->GetParams();
}

ROUNDEDTRACKSCORNERS::NET_SCAN_NET_RECREATE::~NET_SCAN_NET_RECREATE()
{
    dynamic_cast<ROUNDEDTRACKSCORNERS*>(m_Parent)->SetParams(m_current_params);
    dynamic_cast<ROUNDEDTRACKSCORNERS*>(m_Parent)->RecreateMenu();
}

bool ROUNDEDTRACKSCORNERS::NET_SCAN_NET_RECREATE::ExecuteAt(const TRACK* aTrackSeg)
{
    if(aTrackSeg->Type() == PCB_TRACE_T)
    {
        for(ROUNDEDTRACKSCORNER* corner : *m_recreate_list)
        {
            if(corner)
            {
                dynamic_cast<ROUNDEDTRACKSCORNERS*>(m_Parent)->SetParams(corner->GetParams());
                wxPoint pos = corner->GetEnd();
                if((aTrackSeg->GetStart() == pos) || (aTrackSeg->GetEnd() == pos))
                {
                    if(aTrackSeg->IsOnLayer(corner->GetLayer()))
                    {
                        dynamic_cast<ROUNDEDTRACKSCORNERS*>(m_Parent)->Add(const_cast<TRACK*>(aTrackSeg), pos, m_picked_items);
                    }
                }
            }
        }
    }
    return false;
}

void ROUNDEDTRACKSCORNERS::Recreate(const int aNetCodeTo, PICKED_ITEMS_LIST* aUndoRedoList)
{
    std::unique_ptr<NET_SCAN_NET_RECREATE> net(new NET_SCAN_NET_RECREATE(aNetCodeTo, this, aUndoRedoList, m_recreate_list));
    if(net)
        net->Execute();
}

void ROUNDEDTRACKSCORNERS::Update(const BOARD_ITEM* aItemAt)
{
    if(aItemAt)    
    {
        if(dynamic_cast<TRACK*> (const_cast<BOARD_ITEM*> (aItemAt)))
            Update(static_cast<TRACK*>(const_cast<BOARD_ITEM*>(aItemAt)));
    }
}

void ROUNDEDTRACKSCORNERS::SetParams(const ROUNDEDTRACKSCORNER::PARAMS aParams)
{
    m_params = aParams;
    if(m_params.num_segments < ROUNDEDTRACKSCORNER::SEGMENTS_MIN)
        m_params.num_segments = ROUNDEDTRACKSCORNER::SEGMENTS_MIN;
    if(m_params.num_segments > ROUNDEDTRACKSCORNER::SEGMENTS_MAX)
        m_params.num_segments = ROUNDEDTRACKSCORNER::SEGMENTS_MAX;
}

void ROUNDEDTRACKSCORNERS::LoadDefaultParams(void)
{
    m_params = GetDefaultParams();
    RecreateMenu();
}

ROUNDEDTRACKSCORNER::PARAMS ROUNDEDTRACKSCORNERS::GetDefaultParams(void) const
{
    ROUNDEDTRACKSCORNER::PARAMS params = {0, ROUNDEDTRACKSCORNER::DEFAULT_LENGTH_RATIO, ROUNDEDTRACKSCORNER::DEFAULT_NUM_SEGMENTS};
    return params;
}

ROUNDEDTRACKSCORNER::PARAMS ROUNDEDTRACKSCORNERS::CopyCurrentParams(const TRACK* aTrackSegAt, const wxPoint& aCurPosAt)
{
    ROUNDEDTRACKSCORNER::PARAMS corner_params = {0,0,10};
    wxPoint track_pos = TrackSegNearestEndpoint(aTrackSegAt, aCurPosAt);
    ROUNDEDTRACKSCORNER* corner = dynamic_cast<ROUNDEDTRACKSCORNER*>(Get(aTrackSegAt, track_pos));
    if(corner)
    {
        corner_params = corner->GetParams();
        SetParams(corner_params);
        RecreateMenu();
    }
    return corner_params;
}

TRACKNODEITEM* ROUNDEDTRACKSCORNERS::Next(const TRACK* aTrackSegAt) const
{
    ROUNDEDTRACKSCORNER* result = nullptr;
    if(dynamic_cast<ROUNDEDCORNERTRACK*>(const_cast<TRACK*>(aTrackSegAt)))
        result = dynamic_cast<ROUNDEDCORNERTRACK*>(const_cast<TRACK*>(aTrackSegAt))->GetEndPointCorner();
    return result;
}

TRACKNODEITEM* ROUNDEDTRACKSCORNERS::Back(const TRACK* aTrackSegAt) const
{
    ROUNDEDTRACKSCORNER* result = nullptr;
    if(dynamic_cast<ROUNDEDCORNERTRACK*>(const_cast<TRACK*>(aTrackSegAt)))
        result = dynamic_cast<ROUNDEDCORNERTRACK*>(const_cast<TRACK*>(aTrackSegAt))->GetStartPointCorner();
    return result;
}

// Update without draw.
ROUNDEDTRACKSCORNERS::NET_SCAN_TRACK_UPDATE::NET_SCAN_TRACK_UPDATE(const TRACK* aTrackSeg, const ROUNDEDTRACKSCORNERS* aParent) : NET_SCAN_BASE(aTrackSeg, aParent)
{
}

bool ROUNDEDTRACKSCORNERS::NET_SCAN_TRACK_UPDATE::ExecuteAt(const TRACK* aTrackSeg)
{
    if(dynamic_cast<ROUNDEDTRACKSCORNER*>(const_cast<TRACK*>(aTrackSeg)))
    {
        if(dynamic_cast<ROUNDEDTRACKSCORNER*>(const_cast<TRACK*>(aTrackSeg))->GetTrackSeg() == m_net_start_seg)
            dynamic_cast<ROUNDEDTRACKSCORNER*>(const_cast<TRACK*>(aTrackSeg))->Update();

        if(dynamic_cast<ROUNDEDTRACKSCORNER*>(const_cast<TRACK*>(aTrackSeg))->GetTrackSegSecond() == m_net_start_seg)
            dynamic_cast<ROUNDEDTRACKSCORNER*>(const_cast<TRACK*>(aTrackSeg))->Update();
    }   
    return false;
}

void ROUNDEDTRACKSCORNERS::Update(const TRACK* aTrackSegAt)
{
    if(aTrackSegAt)
    {
        if(aTrackSegAt->Type() == PCB_ROUNDEDTRACKSCORNER_T)
            dynamic_cast<ROUNDEDTRACKSCORNER*>(const_cast<TRACK*>(aTrackSegAt))->Update();

        if(aTrackSegAt->Type() == PCB_TRACE_T)
        {
            if(dynamic_cast<ROUNDEDCORNERTRACK*>(const_cast<TRACK*>(aTrackSegAt)))
                dynamic_cast<ROUNDEDCORNERTRACK*>(const_cast<TRACK*>(aTrackSegAt))->ResetVisibleEndpoints();
            
            std::unique_ptr<NET_SCAN_TRACK_UPDATE> track(new NET_SCAN_TRACK_UPDATE(aTrackSegAt, this));
            if(track)
                track->Execute();
        }
    }
} 

void ROUNDEDTRACKSCORNERS::Update(TrackNodeItem::ROUNDEDTRACKSCORNER* aCorner, EDA_DRAW_PANEL* aPanel, wxDC* aDC, GR_DRAWMODE aDrawMode, bool aErase)
{
    if(aCorner)
    {
        RoundedCornerTrack_Container r_tracks;
        r_tracks.insert(static_cast<ROUNDEDCORNERTRACK*>(aCorner->GetTrackSeg()));
        r_tracks.insert(static_cast<ROUNDEDCORNERTRACK*>(aCorner->GetTrackSegSecond()));
        if(aErase)
        {
            for(auto r_t: r_tracks)
                if(r_t)
                    r_t->Draw( aPanel, aDC, aDrawMode);
            aCorner->Draw(aPanel, aDC, aDrawMode);
        }
        aCorner->Update();        
        aCorner->Draw(aPanel, aDC, aDrawMode);
        for(auto r_t: r_tracks)
            if(r_t)
                r_t->Draw( aPanel, aDC, aDrawMode);
    }
}

void ROUNDEDTRACKSCORNERS::Update(TRACK* aTrackSegAt, EDA_DRAW_PANEL* aPanel, wxDC* aDC, GR_DRAWMODE aDrawMode, bool aErase)
{
    if(aTrackSegAt)
    {
        if(aTrackSegAt->Type() == PCB_ROUNDEDTRACKSCORNER_T)
            Update(dynamic_cast<ROUNDEDTRACKSCORNER*> (const_cast<TRACK*> (aTrackSegAt)), aPanel, aDC, aDrawMode, aErase);
        
        if(aTrackSegAt->Type() == PCB_TRACE_T && dynamic_cast<ROUNDEDCORNERTRACK*>(aTrackSegAt))
        {
            dynamic_cast<ROUNDEDCORNERTRACK*>(const_cast<TRACK*>(aTrackSegAt))->ResetVisibleEndpoints();
            ROUNDEDTRACKSCORNER* corner = dynamic_cast<ROUNDEDCORNERTRACK*>(aTrackSegAt)->GetStartPointCorner();
            if(corner)
                Update(corner, aPanel, aDC, aDrawMode, aErase);
                
            corner = dynamic_cast<ROUNDEDCORNERTRACK*>(aTrackSegAt)->GetEndPointCorner();
            if(corner)
                Update(corner, aPanel, aDC, aDrawMode, aErase);
        }
    }
}    

void ROUNDEDTRACKSCORNERS::ToMemory(const TRACK* aTrackSegFrom)
{
    m_next_corner_in_memory = nullptr;
    m_back_corner_in_memory = nullptr;
    m_next_corner_in_memory = dynamic_cast<ROUNDEDTRACKSCORNER*>(Next(aTrackSegFrom));
    m_back_corner_in_memory = dynamic_cast<ROUNDEDTRACKSCORNER*>(Back(aTrackSegFrom));
}

void ROUNDEDTRACKSCORNERS::FromMemory(const TRACK* aTrackSegTo, PICKED_ITEMS_LIST* aItemsListPicker)
{
    ROUNDEDTRACKSCORNER::PARAMS corner_params = GetParams();
    if(m_next_corner_in_memory) //Recreate next corner.
    {   
        ROUNDEDTRACKSCORNER::PARAMS params = m_next_corner_in_memory->GetParams();
        SetParams(params);
        ROUNDEDTRACKSCORNER* added_corner = Add(const_cast<TRACK*>(aTrackSegTo), m_next_corner_in_memory->GetEnd(), aItemsListPicker);
        if(added_corner)
            added_corner->SetLocked(m_next_corner_in_memory->IsLocked());
    }
    if(m_back_corner_in_memory) //Recreate back corner.
    {   
        ROUNDEDTRACKSCORNER::PARAMS params = m_back_corner_in_memory->GetParams();
        SetParams(params);
        ROUNDEDTRACKSCORNER* added_corner = Add(const_cast<TRACK*>(aTrackSegTo), m_back_corner_in_memory->GetEnd(), aItemsListPicker);
        if(added_corner)
            added_corner->SetLocked(m_back_corner_in_memory->IsLocked());
    }
    SetParams(corner_params);
    RecreateMenu();
}

void ROUNDEDTRACKSCORNERS::FromMemory(const TRACK* aTrackSegTo)
{
    PICKED_ITEMS_LIST itemsListPicker;
    FromMemory(aTrackSegTo, &itemsListPicker);
}

void ROUNDEDTRACKSCORNERS::FromMemory(const TRACK* aTrackSegTo, BOARD_COMMIT& aCommit)
{
    PICKED_ITEMS_LIST undoredo_items;
    FromMemory(aTrackSegTo, &undoredo_items);
    
    uint num_added_corners = undoredo_items.GetCount();
    if(num_added_corners)
        for(uint n = 0; n < num_added_corners; ++n)
            aCommit.Added(undoredo_items.GetPickedItem(n));
}
