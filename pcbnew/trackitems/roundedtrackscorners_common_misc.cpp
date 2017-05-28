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
// PLOT
//-----------------------------------------------------------------------------------------------------/
void ROUNDEDTRACKSCORNERS::Plot(const TRACKNODEITEM* aTrackNodeItem, PLOTTER* aPlotter, const EDA_DRAW_MODE_T* aPlotMode, void* aData)
{
    if(aTrackNodeItem && dynamic_cast<ROUNDEDTRACKSCORNER*>(const_cast<TRACKNODEITEM*>(aTrackNodeItem)))
    {
        ROUNDEDTRACKSCORNER* corner = static_cast<ROUNDEDTRACKSCORNER*>(const_cast<TRACKNODEITEM*>(aTrackNodeItem));
        if(corner->IsSetOK())
        {
            std::vector<wxPoint> corners;
            corners.clear();
            for(uint n = 0; n < corner->GetPolyPointsNum() - 1; n++)
            {
                corners.push_back(corner->GetPolyPoint(n));
                aPlotter->ThickSegment(corner->GetPolyPoint(n), corner->GetPolyPoint(n+1), corner->GetWidth(), *aPlotMode, aData );
            }
        }
    }
}

