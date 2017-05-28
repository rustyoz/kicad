/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2017 Heikki Pulkkinen.
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

/**
 * @file roundedcornertrack.h
 * @brief Definitions for rounded track nodes.
 */

#ifndef ROUNDEDCORNERTRACK_H
#define ROUNDEDCORNERTRACK_H

#include <class_track.h>
#include <class_board.h>
#include "roundedtrackscorner.h"


class ROUNDEDCORNERTRACK : public TRACK
{
public:
    ROUNDEDCORNERTRACK(const ROUNDEDCORNERTRACK& aTrack);
    ROUNDEDCORNERTRACK(const BOARD_ITEM* aParent);
    ROUNDEDCORNERTRACK(const BOARD_ITEM* aParent, const TRACK* aTrack);
    ~ROUNDEDCORNERTRACK(){};
    
    EDA_ITEM* Clone() const override {
        return new ROUNDEDCORNERTRACK( *this );
    }

    double GetLengthVisible() const {
        return GetLineLength( m_StartVisible, m_EndVisible );
    }

    void ResetVisibleEndpoints(void);
    const wxPoint& GetStartVisible(void) const {return m_StartVisible;};
    const wxPoint& GetEndVisible(void) const {return m_EndVisible;};

    TrackNodeItem::ROUNDEDTRACKSCORNER* Contains(const wxPoint& aPos) const;
    TrackNodeItem::ROUNDEDTRACKSCORNER* GetStartPointCorner(void) const {return m_StartPointCorner;}
    TrackNodeItem::ROUNDEDTRACKSCORNER* GetEndPointCorner(void) const {return m_EndPointCorner;}

    void TransformShapeWithClearanceToPolygon( SHAPE_POLY_SET& aCornerBuffer,
                                            int             aClearanceValue,
                                            int             aCircleToSegmentsCount,
                                            double          aCorrectionFactor ) const override; //override TRACK.
                                            
    void Draw( EDA_DRAW_PANEL* aPanel, wxDC* DC, GR_DRAWMODE aDrawMode, const wxPoint& aOffset = ZeroOffset ) override;
    
private:
    friend class TrackNodeItem::ROUNDEDTRACKSCORNER; //Only, who can set values.
    
    wxPoint m_StartVisible{0,0};
    wxPoint m_EndVisible{0,0};
    
    TrackNodeItem::ROUNDEDTRACKSCORNER* m_StartPointCorner{nullptr};
    TrackNodeItem::ROUNDEDTRACKSCORNER* m_EndPointCorner{nullptr};
};

#endif //ROUNDEDCORNERTRACK_H
