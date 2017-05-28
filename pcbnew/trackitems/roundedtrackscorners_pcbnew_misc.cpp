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
#include <drag.h>

using namespace TrackNodeItem;


void ROUNDEDTRACKSCORNERS::AddToDragList(const TRACK* aTrackFrom, std::vector<DRAG_SEGM_PICKER>& aDragSegList)
{
    if(aTrackFrom->Type() == PCB_TRACE_T)
    {
        for(int n = 0; n < 2; ++n)
        {
            ROUNDEDTRACKSCORNER* corner = nullptr;
            n? corner = dynamic_cast<ROUNDEDTRACKSCORNER*>(Back(aTrackFrom)) : corner = dynamic_cast<ROUNDEDTRACKSCORNER*>(Next(aTrackFrom));

            if(corner)
            {
                DRAG_SEGM_PICKER wrapper(corner);
                aDragSegList.push_back(wrapper);
            }
        }
    }
}



