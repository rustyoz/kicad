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

#include "teardrops.h"
#include "board_commit.h"

using namespace TrackNodeItem;


void TEARDROPS::CollectCommit(TEARDROP* aTeardrop, std::set<TRACK*>* aCommitContainer, const bool aLockedToo)
{
    if(aTeardrop && aCommitContainer)
    {
        if(aLockedToo || (!aLockedToo && !aTeardrop->IsLocked()))
        {
            aCommitContainer->insert(aTeardrop);
        }
    }
}

TEARDROPS::NET_SCAN_VIA_COLLECTCOMMIT::NET_SCAN_VIA_COLLECTCOMMIT(const VIA* aVia, const TEARDROPS* aParent, std::set<TRACK*>* aCommitContainer, const bool aLockedToo) : NET_SCAN_VIA_UPDATE(aVia, aParent)
{
    m_locked_too = aLockedToo;
    m_commit_container = aCommitContainer;
}

bool TEARDROPS::NET_SCAN_VIA_COLLECTCOMMIT::ExecuteAt(const TRACK* aTrackSeg)
{    
    if(aTrackSeg->Type() == PCB_TRACE_T)
    {
        TEARDROP* tear = dynamic_cast<TEARDROPS*>(m_Parent)->GetTeardrop(aTrackSeg, m_via);
        if(tear)
        {
            if(m_locked_too || (!m_locked_too && !tear->IsLocked()))
            {
                m_commit_container->insert(tear);
            }
        }
    }
    return false;
}

void TEARDROPS::CollectCommit(const VIA* aViaFrom, std::set<TRACK*>* aCommitContainer, const bool aLockedToo)
{
    if(aViaFrom && aCommitContainer)
    {
        std::unique_ptr<NET_SCAN_VIA_COLLECTCOMMIT> via(new NET_SCAN_VIA_COLLECTCOMMIT(aViaFrom, this, aCommitContainer, aLockedToo));
        if(via)
            via->Execute();
    }
}

TEARDROPS::NET_SCAN_PAD_COLLECTCOMMIT::NET_SCAN_PAD_COLLECTCOMMIT(const D_PAD* aPad, const TEARDROPS* aParent, std::set<TRACK*>* aCommitContainer, const bool aLockedToo) : NET_SCAN_PAD_BASE(aPad, aParent)
{
    m_locked_too = aLockedToo;
    m_commit_container = aCommitContainer;
}

bool TEARDROPS::NET_SCAN_PAD_COLLECTCOMMIT::ExecuteAt(const TRACK* aTrackSeg)
{
    TEARDROP* tear = m_Parent->GetTeardrop(aTrackSeg, m_pad);
    if(tear)
    {
        if(m_locked_too || (!m_locked_too && !tear->IsLocked()))
        {
            m_commit_container->insert(tear);
        }
    }
    return false;
}

void TEARDROPS::CollectCommit(const D_PAD* aPadFrom, std::set<TRACK*>* aCommitContainer, const bool aLockedToo)
{
    std::unique_ptr<NET_SCAN_PAD_COLLECTCOMMIT> pad(new NET_SCAN_PAD_COLLECTCOMMIT(aPadFrom, this, aCommitContainer, aLockedToo));
    if(pad)
        pad->Execute();
}

void TEARDROPS::CollectCommit(const TRACK* aTrackSegFrom, std::set<TRACK*>* aCommitContainer, const bool aLockedToo)
{
    if(aTrackSegFrom && aCommitContainer)
    {
        TEARDROP* tear = nullptr;
        for(uint n = 0; n < 2; ++n)
        {
            n? tear = dynamic_cast<TEARDROP*>(Next(aTrackSegFrom)) : tear = dynamic_cast<TEARDROP*>(Back(aTrackSegFrom));
            
            if(tear)
            {
                if(aLockedToo || (!aLockedToo && !tear->IsLocked()))
                {
                    if(tear->GetTrackSeg() == aTrackSegFrom)
                    {
                        aCommitContainer->insert(tear);
                    }
                }
            }
        }
    }
}

void TEARDROPS::GalRemovedListAdd(const TEARDROP* aTear)
{
    if(aTear)
        if(aTear->Type() == PCB_TEARDROP_T)
            m_gal_removed_list->insert(const_cast<TEARDROP*>(aTear)); 
}

