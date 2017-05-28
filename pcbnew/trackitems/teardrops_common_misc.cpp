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

using namespace TrackNodeItem;


//-----------------------------------------------------------------------------------------------------/
// PLOT
//-----------------------------------------------------------------------------------------------------/
void TEARDROPS::Plot(const TRACKNODEITEM* aTrackNodeItem, PLOTTER* aPlotter, const EDA_DRAW_MODE_T* aPlotMode, void* aData)
{
    if(aTrackNodeItem && dynamic_cast<TEARDROP*>(const_cast<TRACKNODEITEM*>(aTrackNodeItem)))
    {
        TEARDROP* tear = static_cast<TEARDROP*>(const_cast<TRACKNODEITEM*>(aTrackNodeItem));
        if(tear->IsSetOK())
        {
            std::vector<wxPoint> corners;
            corners.clear();
            switch(tear->GetShape())
            {
                case TEARDROP::FILLET_T:
                case TEARDROP::TEARDROP_T:
                    for(uint n = 0; n < tear->GetPolyPointsNum() - 1; n++)
                    {
                        corners.push_back(tear->GetPolyPoint(n));
                        aPlotter->ThickSegment(tear->GetPolyPoint(n), tear->GetPolyPoint(n+1), tear->GetPolySegmentWidth(), *aPlotMode, aData );
                    }
                    corners.push_back(tear->GetPolyPoint(tear->GetPolyPointsNum()-1));
                    aPlotter->PlotPoly(corners, FILLED_SHAPE);
                    break;
                case TEARDROP::SUBLAND_T:
                    aPlotter->FlashPadCircle(tear->GetPosition(), tear->GetWidth(), *aPlotMode, aData);
            }
        }
    }
}


