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

using namespace TrackNodeItem;


//-----------------------------------------------------------------------------------------------------/
// PROGRESS ( ALL CORNERS ) OPERATIONS 
//-----------------------------------------------------------------------------------------------------/
// Add all teracks
void ROUNDEDTRACKSCORNERS::Add(const DLIST<TRACK>* aTracksAt)
{
    if(IsOn())
    {
        PICKED_ITEMS_LIST undoredo_list;
        //Convert all tracks, witch are not been converted, to rounded tracks.
        ConvertTracksInList(aTracksAt, &undoredo_list);
        
        std::unique_ptr<ROUNDEDTRACKSCORNERS_PROGRESS_ADD_CORNERS> corners_add(new ROUNDEDTRACKSCORNERS_PROGRESS_ADD_CORNERS(this, aTracksAt, &undoredo_list));
        if(corners_add)
        {
            corners_add->Execute();
            if(undoredo_list.GetCount())
                m_EditFrame->SaveCopyInUndoList(undoredo_list, UR_CHANGED);
        }
    }
}

ROUNDEDTRACKSCORNERS::ROUNDEDTRACKSCORNERS_PROGRESS_ADD_CORNERS::ROUNDEDTRACKSCORNERS_PROGRESS_ADD_CORNERS(const ROUNDEDTRACKSCORNERS* aParent, const DLIST<TRACK>* aTracks, PICKED_ITEMS_LIST* aUndoRedoList) : 
    ROUNDEDTRACKSCORNERS_PROGRESS(aParent, aTracks, aUndoRedoList)
{
    m_progress_title.Printf(_("Adding Rounded Corners"));
}

uint ROUNDEDTRACKSCORNERS::ROUNDEDTRACKSCORNERS_PROGRESS_ADD_CORNERS::ExecuteItem(const BOARD_ITEM* aItemAt)
{
    uint num_added = 0;
    TRACK* track_seg = dynamic_cast<TRACK*>(const_cast<BOARD_ITEM*>(aItemAt));
    if(track_seg)
    {
        if(track_seg->Type() == PCB_TRACE_T)
        {
            num_added = m_undoredo_items->GetCount();
            m_parent->Add(static_cast<TRACK*>(track_seg), m_undoredo_items);
            num_added = m_undoredo_items->GetCount() - num_added;
        }
    }
    return num_added;
}
    
//Convert all TRACKs to ROUNDEDCORNERTRACKs
void ROUNDEDTRACKSCORNERS::ConvertTracksInList(const DLIST<TRACK>* aTracksAt, PICKED_ITEMS_LIST* aUndoRedoList)
{
    bool created_undoredo_list = false;
    if(!aUndoRedoList)
    {
        aUndoRedoList = new PICKED_ITEMS_LIST();
        created_undoredo_list = true;
    }
    
    std::unique_ptr<ROUNDEDTRACKSCORNERS_PROGRESS_CONVERT_TRACKS> tracks_convert(new ROUNDEDTRACKSCORNERS_PROGRESS_CONVERT_TRACKS(this, aTracksAt, aUndoRedoList));
    if(tracks_convert)
    {
        tracks_convert->Execute();
        m_EditFrame->Compile_Ratsnest( nullptr, false );
        
        if(created_undoredo_list && aUndoRedoList->GetCount())
            m_EditFrame->SaveCopyInUndoList(*aUndoRedoList, UR_CHANGED);
    }
    if(created_undoredo_list && aUndoRedoList)
    {
        delete aUndoRedoList;
        aUndoRedoList = nullptr;
    }
}

ROUNDEDTRACKSCORNERS::ROUNDEDTRACKSCORNERS_PROGRESS_CONVERT_TRACKS::ROUNDEDTRACKSCORNERS_PROGRESS_CONVERT_TRACKS(const ROUNDEDTRACKSCORNERS* aParent, const DLIST<TRACK>* aTracks, PICKED_ITEMS_LIST* aUndoRedoList) :
    ROUNDEDTRACKSCORNERS_PROGRESS(aParent, aTracks, aUndoRedoList)
{
    m_can_cancel = false;
    m_progress_title.Printf(_("Convert Tracks to Rounded Corner Tracks"));
    m_progress_style = wxPD_AUTO_HIDE;
}

uint ROUNDEDTRACKSCORNERS::ROUNDEDTRACKSCORNERS_PROGRESS_CONVERT_TRACKS::ExecuteItem(const BOARD_ITEM* aItemAt)
{
    if(aItemAt->Type() == PCB_TRACE_T)
    {
        TRACK* track = static_cast<TRACK*>(const_cast<BOARD_ITEM*>(aItemAt));
        TRACK* track_in = track;
        track = m_parent->ConvertTrackInList(track, m_undoredo_items);
        if(track != track_in)
            return 1;
    }
    return 0;
}

void ROUNDEDTRACKSCORNERS::Remove(DLIST<TRACK>* aTracksAt)
{
    PICKED_ITEMS_LIST undoredo_list;
    std::unique_ptr<ROUNDEDTRACKSCORNERS_PROGRESS_REMOVE_CORNERS> corners_remove(new ROUNDEDTRACKSCORNERS_PROGRESS_REMOVE_CORNERS(this, aTracksAt, &undoredo_list));
        
    if(corners_remove)
    {
        corners_remove->Execute();
        if(undoredo_list.GetCount())
            m_EditFrame->SaveCopyInUndoList(undoredo_list, UR_DELETED);
    }
}

ROUNDEDTRACKSCORNERS::ROUNDEDTRACKSCORNERS_PROGRESS_REMOVE_CORNERS::ROUNDEDTRACKSCORNERS_PROGRESS_REMOVE_CORNERS(const ROUNDEDTRACKSCORNERS* aParent, const DLIST<TRACK>* aTracks, PICKED_ITEMS_LIST* aUndoRedoList) : 
    ROUNDEDTRACKSCORNERS_PROGRESS(aParent, aTracks, aUndoRedoList)
{
    m_tracks = const_cast<DLIST<TRACK>*>(aTracks);
    
    m_remove_corners = new RoundedTracksCorner_Container;
    m_remove_corners->clear();
    m_progress_title.Printf(_("Removing Track Corners"));
}

ROUNDEDTRACKSCORNERS::ROUNDEDTRACKSCORNERS_PROGRESS_REMOVE_CORNERS::~ROUNDEDTRACKSCORNERS_PROGRESS_REMOVE_CORNERS()
{
    if(m_remove_corners)
    {
        delete m_remove_corners;
        m_remove_corners = nullptr;
    }
}

void ROUNDEDTRACKSCORNERS::ROUNDEDTRACKSCORNERS_PROGRESS_REMOVE_CORNERS::ExecuteEnd(void)
{
    for(ROUNDEDTRACKSCORNER* corner : *m_remove_corners)
    {
        m_parent->Remove(corner, m_undoredo_items, false);
    }
}
        
uint ROUNDEDTRACKSCORNERS::ROUNDEDTRACKSCORNERS_PROGRESS_REMOVE_CORNERS::ExecuteItem(const BOARD_ITEM* aItemAt)
{
    ROUNDEDTRACKSCORNER* corner = dynamic_cast<ROUNDEDTRACKSCORNER*>(const_cast<BOARD_ITEM*>(aItemAt));
    if(corner)
    {
        if(!corner->IsLocked())
        {
            m_remove_corners->insert(corner);
            return 1;
        }
    }
    return 0;
}

//-----------------------------------------------------------------------------------------------------/
// Clean
//-----------------------------------------------------------------------------------------------------/
ROUNDEDTRACKSCORNERS::ROUNDEDTRACKSCORNERS_PROGRESS_CLEAN::ROUNDEDTRACKSCORNERS_PROGRESS_CLEAN(const ROUNDEDTRACKSCORNERS* aParent, const DLIST<TRACK>* aTracks, PICKED_ITEMS_LIST* aUndoRedoList) : 
    ROUNDEDTRACKSCORNERS_PROGRESS(aParent, aTracks, aUndoRedoList), 
    ROUNDEDTRACKSCORNERS_PROGRESS_REMOVE_CORNERS(aParent, aTracks, aUndoRedoList)
{
    m_can_cancel = false;
    m_progress_title.Printf(_("Cleaning: Rounded Corners"));
    m_progress_style = wxPD_AUTO_HIDE;
}

uint ROUNDEDTRACKSCORNERS::ROUNDEDTRACKSCORNERS_PROGRESS_CLEAN::ExecuteItem(const BOARD_ITEM* aItemAt)
{
    ROUNDEDTRACKSCORNER* corner = dynamic_cast<ROUNDEDTRACKSCORNER*>(const_cast<BOARD_ITEM*>(aItemAt));
    if(corner)
    {
        TRACK* first_track = corner->GetTrackSeg();
        TRACK* second_track = corner->GetTrackSegSecond();
        wxPoint pos = corner->GetEnd();
        if(!corner->IsSetOK() && (AngleBtwTracks(first_track, pos, second_track, pos) == M_PI))
            m_remove_corners->insert(corner);
    }
    return 0;
}

bool ROUNDEDTRACKSCORNERS::Clean(const DLIST<TRACK>* aTracksAt, BOARD_COMMIT& aCommit)
{
    PICKED_ITEMS_LIST undoredo_list; //Do not need to udoredo, but can if wanted to.
    std::unique_ptr<ROUNDEDTRACKSCORNERS_PROGRESS_CLEAN> corners_clean(new ROUNDEDTRACKSCORNERS_PROGRESS_CLEAN(this, aTracksAt, &undoredo_list));
    if(corners_clean)
    {
        uint cleaned = corners_clean->Execute();
        
        uint num_cleaned_corners = undoredo_list.GetCount();
        if(num_cleaned_corners)
            for(uint n = 0; n < num_cleaned_corners; ++n)
                aCommit.Removed(undoredo_list.GetPickedItem(n));
            
        return (bool)cleaned;
    }
    return false;
}
//-----------------------------------------------------------------------------------------------------/


