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
// PROGRESS ( ALL TEARDROPS ) OPERATIONS
//-----------------------------------------------------------------------------------------------------/
void TEARDROPS::Add(const DLIST<TRACK>* aTracksAt, const TEARDROPS::TEARDROPS_TYPE_TODO aTypeToDo)
{
    PICKED_ITEMS_LIST undoredo_list;
    std::unique_ptr<TRACKS_PROGRESS_ADD_TEARS_VIAS> tears_add(new TRACKS_PROGRESS_ADD_TEARS_VIAS(this, aTracksAt, &undoredo_list));
    if(aTypeToDo == ONLY_TJUNCTIONS_T)
        tears_add.reset(new TRACKS_PROGRESS_ADD_TJUNCTIONS(this, aTracksAt, &undoredo_list));
    else
        if(aTypeToDo == ONLY_JUNCTIONS_T)
            tears_add.reset(new TRACKS_PROGRESS_ADD_JUNCTIONS(this, aTracksAt, &undoredo_list));
        
    if(tears_add)
    {
        tears_add->Execute();
        if(undoredo_list.GetCount())
            m_EditFrame->SaveCopyInUndoList(undoredo_list, UR_NEW);
    }
}

void TEARDROPS::Add(const DLIST<MODULE>* aModulesTo)
{
    PICKED_ITEMS_LIST undoredo_list;
    std::unique_ptr<MODULES_PROGRESS_ADD_TEARS> modules_add(new MODULES_PROGRESS_ADD_TEARS(this, aModulesTo, &undoredo_list));
    if(modules_add)
    {
        modules_add->Execute();
        if(undoredo_list.GetCount())
            m_EditFrame->SaveCopyInUndoList(undoredo_list, UR_NEW);
    }
}

void TEARDROPS::Change(const DLIST<TRACK>* aTracksAt, const TEARDROPS::TEARDROPS_TYPE_TODO aTypeToDo)
{
    PICKED_ITEMS_LIST undoredo_list;
    std::unique_ptr<TRACKS_PROGRESS_CHANGE_TEARS_VIAS> tears_change(new TRACKS_PROGRESS_CHANGE_TEARS_VIAS(this, aTracksAt, &undoredo_list));
    if(aTypeToDo == ONLY_TJUNCTIONS_T)
        tears_change.reset(new TRACKS_PROGRESS_CHANGE_TJUNCTIONS(this, aTracksAt, &undoredo_list));
    else
        if(aTypeToDo == ONLY_JUNCTIONS_T)
            tears_change.reset(new TRACKS_PROGRESS_CHANGE_JUNCTIONS(this, aTracksAt, &undoredo_list));
        
    if(tears_change)
    {
        tears_change->Execute();
        if(undoredo_list.GetCount())
            m_EditFrame->SaveCopyInUndoList(undoredo_list, UR_CHANGED);
    }
}

void TEARDROPS::Change(const DLIST<MODULE>* aModulesAt)
{
    PICKED_ITEMS_LIST undoredo_list;
    std::unique_ptr<MODULES_PROGRESS_CHANGE_TEARS> modules_change(new MODULES_PROGRESS_CHANGE_TEARS(this, aModulesAt, 
                                                                                                    m_recreate_list, &undoredo_list));
    if(modules_change)
    {
        modules_change->Execute();
        if(undoredo_list.GetCount())
            m_EditFrame->SaveCopyInUndoList(undoredo_list, UR_CHANGED);
    }
}

void TEARDROPS::Remove(DLIST<TRACK>* aTracksAt, const TEARDROPS::TEARDROPS_TYPE_TODO aTypeToDo)
{
    PICKED_ITEMS_LIST undoredo_list;
    std::unique_ptr<TRACKS_PROGRESS_REMOVE_TEARS_VIAS> tears_remove(new TRACKS_PROGRESS_REMOVE_TEARS_VIAS(this, aTracksAt, &undoredo_list));
    if(aTypeToDo == ONLY_TJUNCTIONS_T)
        tears_remove.reset(new TRACKS_PROGRESS_REMOVE_TJUNCTIONS(this, aTracksAt, &undoredo_list));
    else
        if(aTypeToDo == ONLY_JUNCTIONS_T)
            tears_remove.reset(new TRACKS_PROGRESS_REMOVE_JUNCTIONS(this, aTracksAt, &undoredo_list));
        
    if(tears_remove)
    {
        tears_remove->Execute();
        if(undoredo_list.GetCount())
            m_EditFrame->SaveCopyInUndoList(undoredo_list, UR_DELETED);
    }
}


void TEARDROPS::Remove(const DLIST<MODULE>* aModulesFrom)
{
    PICKED_ITEMS_LIST undoredo_list;
    std::unique_ptr<MODULES_PROGRESS_REMOVE_TEARS> modules_remove(new MODULES_PROGRESS_REMOVE_TEARS(this, aModulesFrom, &undoredo_list));
    if(modules_remove)
    {
        modules_remove->Execute();
        if(undoredo_list.GetCount())
            m_EditFrame->SaveCopyInUndoList(undoredo_list, UR_DELETED);
    }
}

//-----------------------------------------------------------------------------------------------------/
// Clean
//-----------------------------------------------------------------------------------------------------/
TEARDROPS::TRACKS_PROGRESS_CLEAN::TRACKS_PROGRESS_CLEAN(const TEARDROPS* aParent, const DLIST<TRACK>* aTracks, PICKED_ITEMS_LIST* aUndoRedoList) : 
    TEARDROPS_TRACKS_PROGRESS(aParent, aTracks, aUndoRedoList),
    TRACKS_PROGRESS_REMOVE_TEARS_VIAS(aParent, aTracks, aUndoRedoList)
{
    m_can_cancel = false;
    m_progress_title.Printf(_("Cleaning: Teardrops, Junctions, T-Junctions"));
    m_progress_style = wxPD_AUTO_HIDE;
}

uint TEARDROPS::TRACKS_PROGRESS_CLEAN::ExecuteItem(const BOARD_ITEM* aItemAt)
{
    TEARDROP* tear = dynamic_cast<TEARDROP*>(const_cast<BOARD_ITEM*>(aItemAt));
    if(tear)
    {
        TRACK* track_seg = tear->GetTrackSeg();
        if(track_seg)
        {
            if(!((tear->GetEnd() == track_seg->GetStart()) || (tear->GetEnd() == track_seg->GetEnd())))
            {
                m_remove_tears->insert(tear);
                return 1;
            }
        }
        else
        {
            m_remove_tears->insert(tear);
            return 1;
        }
        /*
        if(!tear->IsSetOK())
        {
            m_remove_tears->push_back(tear);
            return 1;
        }
        */
    }
    return 0;
}

bool TEARDROPS::Clean(const DLIST<TRACK>* aTracksAt)
{
    PICKED_ITEMS_LIST undoredo_list; //Do not need to udoredo, but can if wanted to.
    std::unique_ptr<TRACKS_PROGRESS_CLEAN> tears_clean(new TRACKS_PROGRESS_CLEAN(this, aTracksAt, &undoredo_list));
    if(tears_clean)
    {
        uint cleaned = tears_clean->Execute();
        return (bool)cleaned;
    }
    return false;
}
//-----------------------------------------------------------------------------------------------------/

void TEARDROPS::Lock(const TRACK* aTrackSegAt, const wxPoint& aCurPosAt, const DLIST<TRACK>* aTracksAt)
{
    wxPoint track_pos = TrackSegNearestEndpoint(aTrackSegAt, aCurPosAt);
    TEARDROP* tear = dynamic_cast<TEARDROP*>(Get(aTrackSegAt, track_pos));
    if(tear)
    {
        std::unique_ptr<TRACKS_PROGRESS_LOCK_SAME> to_lock(new TRACKS_PROGRESS_LOCK_SAME(this, aTracksAt, tear->GetParams()));
        if(to_lock)
            to_lock->Execute();
    }
}

void TEARDROPS::Unlock(const TRACK* aTrackSegAt, const wxPoint& aCurPosAt, const DLIST<TRACK>* aTracksAt)
{
    wxPoint track_pos = TrackSegNearestEndpoint(aTrackSegAt, aCurPosAt);
    TEARDROP* tear = dynamic_cast<TEARDROP*>(Get(aTrackSegAt, track_pos));
    if(tear)
    {
        std::unique_ptr<TRACKS_PROGRESS_UNLOCK_SAME> to_unlock(new TRACKS_PROGRESS_UNLOCK_SAME(this, aTracksAt, tear->GetParams()));
        if(to_unlock)
            to_unlock->Execute();
    }
}

void TEARDROPS::Lock(const DLIST<TRACK>* aTracksAt, const TEARDROPS::TEARDROPS_TYPE_TODO aTypeToDo)
{
    std::unique_ptr<TRACKS_PROGRESS_LOCK_TEARS_VIAS> to_lock(new TRACKS_PROGRESS_LOCK_TEARS_VIAS(this, aTracksAt));
    if(aTypeToDo == ONLY_TJUNCTIONS_T)
        to_lock.reset(new TRACKS_PROGRESS_LOCK_TJUNCTIONS(this, aTracksAt));
    else
        if(aTypeToDo == ONLY_JUNCTIONS_T)
            to_lock.reset(new TRACKS_PROGRESS_LOCK_JUNCTIONS(this, aTracksAt));

    if(to_lock)
        to_lock->Execute();
}

void TEARDROPS::Unlock(const DLIST<TRACK>* aTracksAt, const TEARDROPS::TEARDROPS_TYPE_TODO aTypeToDo)
{
    std::unique_ptr<TRACKS_PROGRESS_UNLOCK_TEARS_VIAS> to_unlock(new TRACKS_PROGRESS_UNLOCK_TEARS_VIAS(this, aTracksAt));
    if(aTypeToDo == ONLY_TJUNCTIONS_T)
        to_unlock.reset(new TRACKS_PROGRESS_UNLOCK_TJUNCTIONS(this, aTracksAt));
    else
        if(aTypeToDo == ONLY_JUNCTIONS_T)
            to_unlock.reset(new TRACKS_PROGRESS_UNLOCK_JUNCTIONS(this, aTracksAt));

    if(to_unlock)
        to_unlock->Execute();
}

void TEARDROPS::Lock(const DLIST<MODULE>* aModulesAt)
{
    std::unique_ptr<MODULES_PROGRESS_LOCK_TEARS> to_lock(new MODULES_PROGRESS_LOCK_TEARS(this, aModulesAt));
    if(to_lock)
        to_lock->Execute();
}

void TEARDROPS::Unlock(const DLIST<MODULE>* aModulesAt)
{
    std::unique_ptr<MODULES_PROGRESS_UNLOCK_TEARS> to_unlock(new MODULES_PROGRESS_UNLOCK_TEARS(this, aModulesAt));
    if(to_unlock)
        to_unlock->Execute();
}



//-----------------------------------------------------------------------------------------------------/
// Teardrops modules (PADs)
//-----------------------------------------------------------------------------------------------------/
TEARDROPS::MODULES_PROGRESS_ADD_TEARS::MODULES_PROGRESS_ADD_TEARS(const TEARDROPS* aParent, const DLIST<MODULE>* aModules, PICKED_ITEMS_LIST* aUndoRedoList) : 
    TEARDROPS_MODULES_PROGRESS(aParent, aModules, aUndoRedoList)
{
    m_progress_title.Printf(_("Adding Teardrops to Footprints"));
}

uint TEARDROPS::MODULES_PROGRESS_ADD_TEARS::DoAtPad(const D_PAD* aPadAt)
{
    uint num_added = m_undoredo_items->GetCount();
    m_Parent->Add(aPadAt, m_undoredo_items);
    num_added = m_undoredo_items->GetCount() - num_added;
    return num_added;
}

TEARDROPS::MODULES_PROGRESS_REMOVE_TEARS::MODULES_PROGRESS_REMOVE_TEARS(const TEARDROPS* aParent, const DLIST<MODULE>* aModules, PICKED_ITEMS_LIST* aUndoRedoList) : 
    TEARDROPS_MODULES_PROGRESS(aParent, aModules, aUndoRedoList)
{
    m_progress_title.Printf(_("Removing Teardrops from Footprints"));
}

uint TEARDROPS::MODULES_PROGRESS_REMOVE_TEARS::DoAtPad(const D_PAD* aPadAt)
{
    uint num_removed = m_undoredo_items->GetCount();
    m_Parent->Remove(aPadAt, m_undoredo_items, false);
    num_removed = m_undoredo_items->GetCount() - num_removed;
    return num_removed;
}

TEARDROPS::MODULES_PROGRESS_CHANGE_TEARS::MODULES_PROGRESS_CHANGE_TEARS(const TEARDROPS* aParent, const DLIST<MODULE>* aModules, const Teardrop_Container* aRemovedList, PICKED_ITEMS_LIST* aUndoRedoList) : 
    MODULES_PROGRESS_REMOVE_TEARS(aParent, aModules, aUndoRedoList)
{
    m_removed_tears = const_cast<Teardrop_Container*>(aRemovedList);
    m_tears_list = new Teardrop_Container;
    m_tears_list->clear();
    m_progress_title.Printf(_("Changing Teardrops of Footprints"));
}

TEARDROPS::MODULES_PROGRESS_CHANGE_TEARS::~MODULES_PROGRESS_CHANGE_TEARS()
{
    if(m_tears_list)
    {
        delete m_tears_list;
        m_tears_list = nullptr;
    }
}

uint TEARDROPS::MODULES_PROGRESS_CHANGE_TEARS::DoAtPad(const D_PAD* aPadAt)
{
    uint rv = MODULES_PROGRESS_REMOVE_TEARS::DoAtPad(aPadAt);
    for(TEARDROP* tear : *m_removed_tears)
        m_tears_list->insert(tear);
    return rv;
}

void TEARDROPS::MODULES_PROGRESS_CHANGE_TEARS::ExecuteEnd(void)
{
    for(TEARDROP* tear : *m_tears_list)
        m_Parent->Add(tear->GetTrackSeg(), tear->GetConnectedItem(), m_undoredo_items);
}

TEARDROPS::MODULES_PROGRESS_LOCK_TEARS::MODULES_PROGRESS_LOCK_TEARS(const TEARDROPS* aParent, const DLIST<MODULE>* aModules) :
    TEARDROPS_MODULES_PROGRESS(aParent, aModules, nullptr)
{
    m_progress_title.Printf(_("Locking Teardrops of Footprints"));
}

uint TEARDROPS::MODULES_PROGRESS_LOCK_TEARS::DoAtPad(const D_PAD* aPadAt)
{
    int num_locked = 0;
    int num_tears = m_Parent->Contains(aPadAt, num_locked);
    if(num_tears)
        m_Parent->Lock(aPadAt);
    return num_tears;
}

TEARDROPS::MODULES_PROGRESS_UNLOCK_TEARS::MODULES_PROGRESS_UNLOCK_TEARS(const TEARDROPS* aParent, const DLIST<MODULE>* aModules) : 
    TEARDROPS_MODULES_PROGRESS(aParent, aModules, nullptr)
{
    m_progress_title.Printf(_("Unlocking Teardrops of Footprints"));
}

uint TEARDROPS::MODULES_PROGRESS_UNLOCK_TEARS::DoAtPad(const D_PAD* aPadAt)
{
    int num_locked = 0;
    int num_tears = m_Parent->Contains(aPadAt, num_locked);
    if(num_tears)
        m_Parent->Unlock(aPadAt);
    return num_tears;
}


//-----------------------------------------------------------------------------------------------------/
// TRACKS_PROGRESS 
//-----------------------------------------------------------------------------------------------------/
TEARDROPS::TRACKS_PROGRESS_ADD_TEARS_VIAS::TRACKS_PROGRESS_ADD_TEARS_VIAS(const TEARDROPS* aParent, const DLIST<TRACK>* aTracks, PICKED_ITEMS_LIST* aUndoRedoList) : 
    TEARDROPS_TRACKS_PROGRESS(aParent, aTracks, aUndoRedoList)
{
    m_progress_title.Printf(_("Adding Teardrops to Vias"));
}

uint TEARDROPS::TRACKS_PROGRESS_ADD_TEARS_VIAS::ExecuteItem(const BOARD_ITEM* aItemAt)
{
    uint num_added = 0;
    TRACK* track_seg = dynamic_cast<TRACK*>(const_cast<BOARD_ITEM*>(aItemAt));
    if(track_seg)
    {
        if(track_seg->Type() == PCB_VIA_T)
        {
            num_added = m_undoredo_items->GetCount();
            m_Parent->Add(static_cast<VIA*>(track_seg), m_undoredo_items);
            num_added = m_undoredo_items->GetCount() - num_added;
        }
    }
    return num_added;
}
    
TEARDROPS::TRACKS_PROGRESS_REMOVE_TEARS_VIAS::TRACKS_PROGRESS_REMOVE_TEARS_VIAS(const TEARDROPS* aParent, const DLIST<TRACK>* aTracks, PICKED_ITEMS_LIST* aUndoRedoList) : 
    TEARDROPS_TRACKS_PROGRESS(aParent, aTracks, aUndoRedoList)
{
    m_tracks = const_cast<DLIST<TRACK>*>(aTracks);
    
    m_remove_tears = new Teardrop_Container;
    m_remove_tears->clear();
    m_progress_title.Printf(_("Removing Teardrops from Vias"));
}

TEARDROPS::TRACKS_PROGRESS_REMOVE_TEARS_VIAS::~TRACKS_PROGRESS_REMOVE_TEARS_VIAS()
{
    if(m_remove_tears)
    {
        delete m_remove_tears;
        m_remove_tears = nullptr;
    }
}

void TEARDROPS::TRACKS_PROGRESS_REMOVE_TEARS_VIAS::ExecuteEnd(void)
{
    for(TEARDROP* tear : *m_remove_tears)
    {
        m_Parent->Remove(tear, m_undoredo_items, false);
    }
}
        
uint TEARDROPS::TRACKS_PROGRESS_REMOVE_TEARS_VIAS::ExecuteItem(const BOARD_ITEM* aItemAt)
{
    TEARDROP* tear = dynamic_cast<TEARDROP_VIA*>(const_cast<BOARD_ITEM*>(aItemAt));
    if(tear)
    {
        if(!tear->IsLocked())
        {
            m_remove_tears->insert(tear);
            return 1;
        }
    }
    return 0;
}

TEARDROPS::TRACKS_PROGRESS_CHANGE_TEARS_VIAS::TRACKS_PROGRESS_CHANGE_TEARS_VIAS(const TEARDROPS* aParent, const DLIST<TRACK>* aTracks, PICKED_ITEMS_LIST* aUndoRedoList) : 
    TEARDROPS_TRACKS_PROGRESS(aParent, aTracks, aUndoRedoList),  
    TRACKS_PROGRESS_REMOVE_TEARS_VIAS(aParent, aTracks, aUndoRedoList)
{
    m_progress_title.Printf(_("Changing Teardrops of Vias"));
}
        
void TEARDROPS::TRACKS_PROGRESS_CHANGE_TEARS_VIAS::ExecuteEnd(void)
{
    TRACKS_PROGRESS_REMOVE_TEARS_VIAS::ExecuteEnd();
    
    for(TEARDROP* tear : *m_remove_tears)
        m_Parent->Add(tear->GetTrackSeg(), tear->GetConnectedItem(), m_undoredo_items, tear->GetEnd());
}
   
TEARDROPS::TRACKS_PROGRESS_ADD_TJUNCTIONS::TRACKS_PROGRESS_ADD_TJUNCTIONS(const TEARDROPS* aParent, const DLIST<TRACK>* aTracks, PICKED_ITEMS_LIST* aUndoRedoList) :
    TRACKS_PROGRESS_ADD_TEARS_VIAS(aParent, aTracks, aUndoRedoList)
{
    m_progress_title.Printf(_("Adding T-Junctions"));
}

uint TEARDROPS::TRACKS_PROGRESS_ADD_TJUNCTIONS::ExecuteItem(const BOARD_ITEM* aItemAt)
{
    uint num_added = m_undoredo_items->GetCount();
    TRACK* track_seg = dynamic_cast<TRACK*>(const_cast<BOARD_ITEM*>(aItemAt));
    if(track_seg)
        m_Parent->Add(track_seg, TEARDROPS::TJUNCTION_TO_ADD, m_undoredo_items);
    num_added = m_undoredo_items->GetCount() - num_added;
    return num_added;
}
    
TEARDROPS::TRACKS_PROGRESS_REMOVE_TJUNCTIONS::TRACKS_PROGRESS_REMOVE_TJUNCTIONS(const TEARDROPS* aParent, const DLIST<TRACK>* aTracks, PICKED_ITEMS_LIST* aUndoRedoList) : 
    TEARDROPS_TRACKS_PROGRESS(aParent, aTracks, aUndoRedoList), 
    TRACKS_PROGRESS_REMOVE_TEARS_VIAS(aParent, aTracks, aUndoRedoList)
{
    m_progress_title.Printf(_("Removing T-Junctions"));
}

uint TEARDROPS::TRACKS_PROGRESS_REMOVE_TJUNCTIONS::ExecuteItem(const BOARD_ITEM* aItemAt)
{
    TEARDROP* tear = dynamic_cast<TEARDROP_JUNCTIONS*>(const_cast<BOARD_ITEM*>(aItemAt));
    if(tear)
    {
        if(!tear->IsLocked())
        {
            if(dynamic_cast<TEARDROP_JUNCTIONS*>(tear)->Get_T_SegBack() && dynamic_cast<TEARDROP_JUNCTIONS*>(tear)->Get_T_SegNext())
            {
                m_remove_tears->insert(tear);
                return 1;
            }
        }
    }
    return 0;
}

TEARDROPS::TRACKS_PROGRESS_CHANGE_TJUNCTIONS::TRACKS_PROGRESS_CHANGE_TJUNCTIONS(const TEARDROPS* aParent, const DLIST<TRACK>* aTracks, PICKED_ITEMS_LIST* aUndoRedoList) : 
    TEARDROPS_TRACKS_PROGRESS(aParent, aTracks, aUndoRedoList),
    TRACKS_PROGRESS_REMOVE_TEARS_VIAS(aParent, aTracks, aUndoRedoList),
    TRACKS_PROGRESS_CHANGE_TEARS_VIAS(aParent, aTracks, aUndoRedoList),
    TRACKS_PROGRESS_REMOVE_TJUNCTIONS(aParent, aTracks, aUndoRedoList)
{
    m_progress_title.Printf(_("Changing T-Junctions"));
}

uint TEARDROPS::TRACKS_PROGRESS_CHANGE_TJUNCTIONS::ExecuteItem(const BOARD_ITEM* aItemAt)
{
    return TRACKS_PROGRESS_REMOVE_TJUNCTIONS::ExecuteItem(aItemAt);
}

void TEARDROPS::TRACKS_PROGRESS_CHANGE_TJUNCTIONS::ExecuteEnd(void)
{
    TRACKS_PROGRESS_CHANGE_TEARS_VIAS::ExecuteEnd();
}

TEARDROPS::TRACKS_PROGRESS_ADD_JUNCTIONS::TRACKS_PROGRESS_ADD_JUNCTIONS(const TEARDROPS* aParent, const DLIST<TRACK>* aTracks, PICKED_ITEMS_LIST* aUndoRedoList) : 
    TRACKS_PROGRESS_ADD_TJUNCTIONS(aParent, aTracks, aUndoRedoList)
{
    m_progress_title.Printf(_("Adding Junctions"));
}

uint TEARDROPS::TRACKS_PROGRESS_ADD_JUNCTIONS::ExecuteItem(const BOARD_ITEM* aItemAt)
{
    uint num_added = m_undoredo_items->GetCount();
    TRACK* track_seg = dynamic_cast<TRACK*>(const_cast<BOARD_ITEM*>(aItemAt));
    if(track_seg)
        m_Parent->Add(track_seg, TEARDROPS::JUNCTION_TO_ADD, m_undoredo_items);
    num_added = m_undoredo_items->GetCount() - num_added;
    return num_added;
}
    
TEARDROPS::TRACKS_PROGRESS_REMOVE_JUNCTIONS::TRACKS_PROGRESS_REMOVE_JUNCTIONS(const TEARDROPS* aParent, const DLIST<TRACK>* aTracks, PICKED_ITEMS_LIST* aUndoRedoList) :
TEARDROPS_TRACKS_PROGRESS(aParent, aTracks, aUndoRedoList),
    TRACKS_PROGRESS_REMOVE_TEARS_VIAS(aParent, aTracks, aUndoRedoList),
    TRACKS_PROGRESS_REMOVE_TJUNCTIONS(aParent, aTracks, aUndoRedoList)
{
    m_progress_title.Printf(_("Removing Junctions"));
}

uint TEARDROPS::TRACKS_PROGRESS_REMOVE_JUNCTIONS::ExecuteItem(const BOARD_ITEM* aItemAt)
{
    TEARDROP* tear = dynamic_cast<TEARDROP_JUNCTIONS*>(const_cast<BOARD_ITEM*>(aItemAt));
    if(tear)
    {
        if(!tear->IsLocked())
        {
            if(!(dynamic_cast<TEARDROP_JUNCTIONS*>(tear)->Get_T_SegBack() && dynamic_cast<TEARDROP_JUNCTIONS*>(tear)->Get_T_SegNext()))
            {
                m_remove_tears->insert(tear);
                return 1;
            }
        }
    }
    return 0;
}

TEARDROPS::TRACKS_PROGRESS_CHANGE_JUNCTIONS::TRACKS_PROGRESS_CHANGE_JUNCTIONS(const TEARDROPS* aParent, const DLIST<TRACK>* aTracks, PICKED_ITEMS_LIST* aUndoRedoList) :
TEARDROPS_TRACKS_PROGRESS(aParent, aTracks, aUndoRedoList),
    TRACKS_PROGRESS_REMOVE_TEARS_VIAS(aParent, aTracks, aUndoRedoList),
    TRACKS_PROGRESS_CHANGE_TEARS_VIAS(aParent, aTracks, aUndoRedoList),
    TRACKS_PROGRESS_REMOVE_JUNCTIONS(aParent, aTracks, aUndoRedoList)
{
    m_progress_title.Printf(_("Changing Junctions"));
}

uint TEARDROPS::TRACKS_PROGRESS_CHANGE_JUNCTIONS::ExecuteItem(const BOARD_ITEM* aItemAt)
{
    return TRACKS_PROGRESS_REMOVE_JUNCTIONS::ExecuteItem(aItemAt);
}

void TEARDROPS::TRACKS_PROGRESS_CHANGE_JUNCTIONS::ExecuteEnd(void)
{
    TRACKS_PROGRESS_CHANGE_TEARS_VIAS::ExecuteEnd();
}

TEARDROPS::TRACKS_PROGRESS_LOCK_SAME::TRACKS_PROGRESS_LOCK_SAME(const TEARDROPS* aParent, const DLIST<TRACK>* aTracks, const TEARDROP::PARAMS aParams) : 
    TEARDROPS_TRACKS_PROGRESS(aParent, aTracks, nullptr)
{
    m_params = aParams;
    m_progress_title.Printf(_("Locking All [%s]"), GetChars(aParent->ParamsTxtToMenu(m_params)));
}

uint TEARDROPS::TRACKS_PROGRESS_LOCK_SAME::ExecuteItem(const BOARD_ITEM* aItemAt)
{
    TEARDROP* tear = dynamic_cast<TEARDROP*>(const_cast<BOARD_ITEM*>(aItemAt));
    if(tear)
    {
        if(tear->GetParams() == m_params)
        {
            tear->SetLocked(true);
            return 1;
        }
    }
    return 0;
}
    
TEARDROPS::TRACKS_PROGRESS_UNLOCK_SAME::TRACKS_PROGRESS_UNLOCK_SAME(const TEARDROPS* aParent, const DLIST<TRACK>* aTracks, const TEARDROP::PARAMS aParams) : 
    TRACKS_PROGRESS_LOCK_SAME(aParent, aTracks, aParams)
{
    m_progress_title.Printf(_("Unlocking All [%s]"), GetChars(aParent->ParamsTxtToMenu(aParams)));
}

uint TEARDROPS::TRACKS_PROGRESS_UNLOCK_SAME::ExecuteItem(const BOARD_ITEM* aItemAt)
{
    TEARDROP* tear = dynamic_cast<TEARDROP*>(const_cast<BOARD_ITEM*>(aItemAt));
    if(tear)
    {
        if(tear->GetParams() == m_params)
        {
            tear->SetLocked(false);
            return 1;
        }
    }
    return 0;
}

TEARDROPS::TRACKS_PROGRESS_LOCK_TEARS_VIAS::TRACKS_PROGRESS_LOCK_TEARS_VIAS(const TEARDROPS* aParent, const DLIST<TRACK>* aTracks) :
    TEARDROPS_TRACKS_PROGRESS(aParent, aTracks, nullptr)
{
    m_progress_title.Printf(_("Locking Teardrops of Vias"));
}

uint TEARDROPS::TRACKS_PROGRESS_LOCK_TEARS_VIAS::ExecuteItem(const BOARD_ITEM* aItemAt)
{
    TEARDROP* tear = dynamic_cast<TEARDROP_VIA*>(const_cast<BOARD_ITEM*>(aItemAt));
    if(tear)
    {
        tear->SetLocked(true);
        return 1;
    }
    return 0;
}

TEARDROPS::TRACKS_PROGRESS_UNLOCK_TEARS_VIAS::TRACKS_PROGRESS_UNLOCK_TEARS_VIAS(const TEARDROPS* aParent, const DLIST<TRACK>* aTracks) :
    TEARDROPS_TRACKS_PROGRESS(aParent, aTracks, nullptr)
{
    m_progress_title.Printf(_("Unlocking Teardrops of Vias"));
}

uint TEARDROPS::TRACKS_PROGRESS_UNLOCK_TEARS_VIAS::ExecuteItem(const BOARD_ITEM* aItemAt)
{
    TEARDROP* tear = dynamic_cast<TEARDROP_VIA*>(const_cast<BOARD_ITEM*>(aItemAt));
    if(tear)
    {
        tear->SetLocked(false);
        return 1;
    }
    return 0;
}

TEARDROPS::TRACKS_PROGRESS_LOCK_TJUNCTIONS::TRACKS_PROGRESS_LOCK_TJUNCTIONS(const TEARDROPS* aParent, const DLIST<TRACK>* aTracks) :
    TRACKS_PROGRESS_LOCK_TEARS_VIAS(aParent, aTracks)
{
    m_progress_title.Printf(_("Locking T-Junctions"));
}

uint TEARDROPS::TRACKS_PROGRESS_LOCK_TJUNCTIONS::ExecuteItem(const BOARD_ITEM* aItemAt)
{
    TEARDROP* tear = dynamic_cast<TEARDROP_JUNCTIONS*>(const_cast<BOARD_ITEM*>(aItemAt));
    if(tear)
    {
        if(dynamic_cast<TEARDROP_JUNCTIONS*>(tear)->Get_T_SegBack() && dynamic_cast<TEARDROP_JUNCTIONS*>(tear)->Get_T_SegNext())
        {
            tear->SetLocked(true);
            return 1;
        }
    }
    return 0;
}

TEARDROPS::TRACKS_PROGRESS_UNLOCK_TJUNCTIONS::TRACKS_PROGRESS_UNLOCK_TJUNCTIONS(const TEARDROPS* aParent, const DLIST<TRACK>* aTracks) :
    TRACKS_PROGRESS_UNLOCK_TEARS_VIAS(aParent, aTracks)
{
    m_progress_title.Printf(_("Unlocking T-Junctions"));
}

uint TEARDROPS::TRACKS_PROGRESS_UNLOCK_TJUNCTIONS::ExecuteItem(const BOARD_ITEM* aItemAt)
{
    TEARDROP* tear = dynamic_cast<TEARDROP_JUNCTIONS*>(const_cast<BOARD_ITEM*>(aItemAt));
    if(tear)
    {
        if(dynamic_cast<TEARDROP_JUNCTIONS*>(tear)->Get_T_SegBack() && dynamic_cast<TEARDROP_JUNCTIONS*>(tear)->Get_T_SegNext())
        {
            tear->SetLocked(false);
            return 1;
        }
    }
    return 0;
}

TEARDROPS::TRACKS_PROGRESS_LOCK_JUNCTIONS::TRACKS_PROGRESS_LOCK_JUNCTIONS(const TEARDROPS* aParent, const DLIST<TRACK>* aTracks) :
    TRACKS_PROGRESS_LOCK_TEARS_VIAS(aParent, aTracks)
{
    m_progress_title.Printf(_("Locking Junctions"));
}

uint TEARDROPS::TRACKS_PROGRESS_LOCK_JUNCTIONS::ExecuteItem(const BOARD_ITEM* aItemAt)
{
    TEARDROP* tear = dynamic_cast<TEARDROP_JUNCTIONS*>(const_cast<BOARD_ITEM*>(aItemAt));
    if(tear)
    {
        if(!(dynamic_cast<TEARDROP_JUNCTIONS*>(tear)->Get_T_SegBack() && dynamic_cast<TEARDROP_JUNCTIONS*>(tear)->Get_T_SegNext()))
        {
            tear->SetLocked(true);
            return 1;
        }
    }
    return 0;
}

TEARDROPS::TRACKS_PROGRESS_UNLOCK_JUNCTIONS::TRACKS_PROGRESS_UNLOCK_JUNCTIONS(const TEARDROPS* aParent, const DLIST<TRACK>* aTracks) :
    TRACKS_PROGRESS_UNLOCK_TEARS_VIAS(aParent, aTracks)
{
    m_progress_title.Printf(_("Unlocking Junctions"));
}

uint TEARDROPS::TRACKS_PROGRESS_UNLOCK_JUNCTIONS::ExecuteItem(const BOARD_ITEM* aItemAt)
{
    TEARDROP* tear = dynamic_cast<TEARDROP_JUNCTIONS*>(const_cast<BOARD_ITEM*>(aItemAt));
    if(tear)
    {
        if(!(dynamic_cast<TEARDROP_JUNCTIONS*>(tear)->Get_T_SegBack() && dynamic_cast<TEARDROP_JUNCTIONS*>(tear)->Get_T_SegNext()))
        {
            tear->SetLocked(false);
            return 1;
        }
    }
    return 0;
}

//Mark different teardrops of all pads, vias, juctions or t-junctions.
void TEARDROPS::MarkDifferent(const DLIST<TRACK>* aTracksAt, const TEARDROPS::TEARDROPS_TYPE_TODO aTypeToDo, DRC* aDRC)
{
    std::unique_ptr<TRACKS_PROGRESS_MARK_DIFF> mark_diff(new TRACKS_PROGRESS_MARK_DIFF(this, aTracksAt, aTypeToDo, aDRC));
    if(mark_diff)
        mark_diff->Execute();
}

TEARDROPS::TRACKS_PROGRESS_MARK_DIFF::TRACKS_PROGRESS_MARK_DIFF(const TEARDROPS* aParent, const DLIST<TRACK>* aTracks, const TEARDROPS::TEARDROPS_TYPE_TODO aTypeToDo, DRC* aDRC) : 
    TRACKS_PROGRESS_MARK_WARNINGS(aParent, aTracks, aTypeToDo, aDRC)
{
    m_collected_tears = new Teardrop_Container;
    m_collected_tears->clear();
    switch(aTypeToDo)
    {
        case ONLY_TEARDROPS_T:
            m_progress_title.Printf(_("Searching Different Teardrops of Vias"));
            break;
        case ONLY_TJUNCTIONS_T:
            m_progress_title.Printf(_("Searching Different T-Junctions"));
            break;
        case ONLY_JUNCTIONS_T:
            m_progress_title.Printf(_("Searching Different Junctions"));
            break;
        case ONLY_PAD_TEARDROPS_T:
            m_progress_title.Printf(_("Searching Different Teardrops of Footprints"));
            break;
        default:
            m_progress_title.Printf(_("Searching Different: Teardrops of Vias, T-Junctions, Junctions"));
    }
}

TEARDROPS::TRACKS_PROGRESS_MARK_DIFF::~TRACKS_PROGRESS_MARK_DIFF()
{
    if(m_collected_tears)
        delete m_collected_tears;
}

uint TEARDROPS::TRACKS_PROGRESS_MARK_DIFF::ExecuteItem(const BOARD_ITEM* aItemAt)
{
    uint num_marks = 0;
    TEARDROP* tear = dynamic_cast<TEARDROP*>(const_cast<BOARD_ITEM*>(aItemAt));
    if(tear)
    {
        bool hit = false;
        for(TEARDROP* tear_n : *m_collected_tears)
            if(tear_n->GetParams() == tear->GetParams())
            {
                hit = true;
                break;
            }
        
        if(!hit)
        {
            int warning_code = 0;
            if(dynamic_cast<TEARDROP_JUNCTIONS*>(tear))
            {
                bool isT = dynamic_cast<TEARDROP_JUNCTIONS*>(tear)->Is_T_Junction();
                if((m_type_todo == ALL_TYPES_T) || (isT && (m_type_todo == ONLY_TJUNCTIONS_T)) || (!isT && (m_type_todo == ONLY_JUNCTIONS_T)))
                    warning_code = DRCE_TRACKNODEITEM_UNSPECIFIED;
            }
            else
                if(!dynamic_cast<TEARDROP_PAD*>(tear))
                {
                    if((m_type_todo == ALL_TYPES_T) || (m_type_todo == ONLY_TEARDROPS_T))
                        warning_code = DRCE_TRACKNODEITEM_UNSPECIFIED;
                }
                else
                {
                    if((m_type_todo == ALL_TYPES_T) || (m_type_todo == ONLY_PAD_TEARDROPS_T))
                        warning_code = DRCE_TRACKNODEITEM_UNSPECIFIED;
                }
            if(warning_code)
            {
                m_collected_tears->insert(tear);
                m_Parent->GetParent()->DRC_AddMarker(tear, nullptr, tear->GetPosition(), warning_code);
                ++num_marks;
            }
        }
    }
    return num_marks;
}

//Mark all current type teardrops of all pads, vias, juctions or t-junctions.
void TEARDROPS::MarkCurrent(const DLIST<TRACK>* aTracksAt, const TEARDROPS::TEARDROPS_TYPE_TODO aTypeToDo, DRC* aDRC)
{
    std::unique_ptr<TRACKS_PROGRESS_MARK_CURR> mark_curr(new TRACKS_PROGRESS_MARK_CURR(this, aTracksAt, aTypeToDo, aDRC));
    if(mark_curr)
        mark_curr->Execute();
}

TEARDROPS::TRACKS_PROGRESS_MARK_CURR::TRACKS_PROGRESS_MARK_CURR(const TEARDROPS* aParent, const DLIST<TRACK>* aTracks, const TEARDROPS::TEARDROPS_TYPE_TODO aTypeToDo, DRC* aDRC) : 
    TRACKS_PROGRESS_MARK_WARNINGS(aParent, aTracks, aTypeToDo, aDRC)
{
    switch(aTypeToDo)
    {
        case ONLY_TEARDROPS_T:
            m_progress_title.Printf(_("Searching Current Teardrops of Vias"));
            break;
        case ONLY_TJUNCTIONS_T:
            m_progress_title.Printf(_("Searching Current T-Junctions"));
            break;
        case ONLY_JUNCTIONS_T:
            m_progress_title.Printf(_("Searching Current Junctions"));
            break;
        case ONLY_PAD_TEARDROPS_T:
            m_progress_title.Printf(_("Searching Current Teardrops of Footprints"));
            break;
        default:
            m_progress_title.Printf(_("Searching Current: Teardrops of Vias, T-Junctions, Junctions"));
    }
}

uint TEARDROPS::TRACKS_PROGRESS_MARK_CURR::ExecuteItem(const BOARD_ITEM* aItemAt)
{
    uint num_marks = 0;
    TEARDROP::PARAMS curr_params = m_Parent->GetShapeParams(m_Parent->GetCurrentShape());
    TEARDROP* tear = dynamic_cast<TEARDROP*>(const_cast<BOARD_ITEM*>(aItemAt));
    if(tear)
    {
        int warning_code = 0;
        if(curr_params == tear->GetParams())
        {
            if(dynamic_cast<TEARDROP_JUNCTIONS*>(tear))
            {
                bool isT = dynamic_cast<TEARDROP_JUNCTIONS*>(tear)->Is_T_Junction();
                if((m_type_todo == ALL_TYPES_T) || (isT && (m_type_todo == ONLY_TJUNCTIONS_T)) || (!isT && (m_type_todo == ONLY_JUNCTIONS_T)))
                    warning_code = DRCE_TRACKNODEITEM_UNSPECIFIED;
            }
            else
                if(!dynamic_cast<TEARDROP_PAD*>(tear))
                {
                    if((m_type_todo == ALL_TYPES_T) || (m_type_todo == ONLY_TEARDROPS_T))
                        warning_code = DRCE_TRACKNODEITEM_UNSPECIFIED;
                }
                else
                    if((m_type_todo == ALL_TYPES_T) || (m_type_todo == ONLY_PAD_TEARDROPS_T))
                        warning_code = DRCE_TRACKNODEITEM_UNSPECIFIED;
        }
        if(warning_code)
        {
            m_Parent->GetParent()->DRC_AddMarker(tear, nullptr, tear->GetPosition(), warning_code);
            ++num_marks;
        }
    }
    return num_marks;
}
