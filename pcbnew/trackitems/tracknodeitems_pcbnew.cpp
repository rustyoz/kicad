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

#include"tracknodeitems.h"
#include "teardrops.h"


using namespace TrackNodeItem;
using namespace TrackNodeItems;

//-----------------------------------------------------------------------------------------------------/
// PROGRESS CLASSES 
//-----------------------------------------------------------------------------------------------------/
// BASE item class 
ITEMS_PROGRESS_BASE::ITEMS_PROGRESS_BASE(const PCB_EDIT_FRAME* aFrame, const BOARD_ITEM* aListFirstItem, PICKED_ITEMS_LIST* aUndoRedo)
{
    m_frame = const_cast<PCB_EDIT_FRAME*>(aFrame);
    
    m_undoredo_items = aUndoRedo;

    m_progress_to_count = 0;
    m_items_to_count = 0;
    m_list_first_item = const_cast<BOARD_ITEM*>(aListFirstItem);
    if(m_list_first_item)
    {
        m_progress_to_count = m_list_first_item->GetList()->GetCount();
        m_items_to_count = m_progress_to_count - 1;
    }

    m_can_cancel = true;
    
    m_progress_title.Printf(_("Progressing..."));
}

ITEMS_PROGRESS_BASE::~ITEMS_PROGRESS_BASE()
{
    if(m_frame && m_undoredo_items && m_undoredo_items->GetCount())
    {
        if(m_cancelled)
            m_frame->PutDataInPreviousState(m_undoredo_items, false, false);
    }
    if(m_progress)
        m_progress->Destroy();
    
}

bool ITEMS_PROGRESS_BASE::UpdateProgress(const uint aProgress, const uint aOperations)
{
    wxString msg;
    msg.Printf(_( "Number of: %d" ), aOperations);
    bool skip = false;
    return m_progress->Update(aProgress, msg, &skip);
}

uint ITEMS_PROGRESS_BASE::Execute(void)
{
    if(m_list_first_item)    
    {
        if(m_can_cancel)
        {
            m_progress_style |= wxPD_CAN_ABORT;
        }
        m_progress = new wxProgressDialog(m_progress_title, _("******************************************************"), m_progress_to_count, m_frame, m_progress_style);
        
        if(m_progress)
        {
            BOARD_ITEM* item = const_cast<BOARD_ITEM*>(m_list_first_item);
            uint operations_count = 0;
            int progress_count = 0;
            uint update_val = m_list_first_item->GetList()->GetCount() / 100;
            uint update_count = 0;
            while(item)
            {
                BOARD_ITEM* next_item = item->Next();
                operations_count += ExecuteItem(item);
                if(++update_count > update_val)
                {
                    update_count = 0;
                    if(!UpdateProgress(progress_count, operations_count))
                    {
                        if(m_can_cancel)
                        {
                            m_cancelled = true;
                            return 0;
                        }
                    }
                }
                if(++progress_count >= m_items_to_count)
                    progress_count = m_items_to_count;
                item = next_item;
            }
            UpdateProgress(m_progress_to_count, operations_count);
            ExecuteEnd();
            return operations_count;
        }
    }
    return 0;
}


//-----------------------------------------------------------------------------------------------------/
// MODULES_PROGRESS 
//-----------------------------------------------------------------------------------------------------/
// Base modules 
MODULES_PROGRESS::MODULES_PROGRESS(const PCB_EDIT_FRAME* aFrame, const DLIST<MODULE>* aModules, PICKED_ITEMS_LIST* aUndoRedo) : ITEMS_PROGRESS_BASE(aFrame, aModules->GetFirst(), aUndoRedo)
{
}

uint MODULES_PROGRESS::ExecuteItem(const BOARD_ITEM* aItemAt)
{
    uint operations_count = 0;
    MODULE* module = dynamic_cast<MODULE*>(const_cast<BOARD_ITEM*>(aItemAt));
    if(module)
    {
        D_PAD* pad = module->Pads();
        while(pad)
        {
            operations_count += DoAtPad(pad);
            pad = pad->Next();
        }
    }
    return operations_count;
}


//-----------------------------------------------------------------------------------------------------/
// DRC 
//-----------------------------------------------------------------------------------------------------/
bool TRACKNODEITEMS::TestSegment(const wxPoint aStartPoint, const wxPoint aEndPoint, const wxPoint aTestPoint, const int aMinDist)
{
    //Copyed from drc_clearance_test_functions.cpp
    wxPoint delta = aEndPoint - aStartPoint;
    wxPoint seg_start_point = aTestPoint - aStartPoint;
    int angle = ArcTangente(delta.y, delta.x );
    RotatePoint( &delta, angle );
    RotatePoint( &seg_start_point, angle );
    
    if(!m_EditFrame->GetDrcController()->checkMarginToCircle(seg_start_point, aMinDist, delta.x ))
        return false;
    
    return true;
}
