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
 * @file roundedtrackscorner.h
 * @brief Definitions for rounded track nodes.
 */

#ifndef ROUNDEDTRACKSCORNER_H
#define ROUNDEDTRACKSCORNER_H

#include "tracknodeitem.h" 


namespace TrackNodeItem
{
    class ROUNDEDTRACKSCORNER : public TrackNodeItem::TRACKNODEITEM
    {
    public:
        static const int SEGMENTS_MAX = 100;     //max num segments.
        static const int SEGMENTS_MIN = 10;      //min num segments.
        static const int DEFAULT_MIN_LENGTH_SET = 254000; //0,254mm = 10 mils
        static const int DEFAULT_LENGTH_RATIO = 100;
        static const int DEFAULT_NUM_SEGMENTS = 10;
        
        class PARAMS
        {
        public:

            uint length_set; //if 0 m_length_set value depends on m_Width;
            int length_ratio;
            int num_segments;
            bool operator==(const PARAMS& aComp) const 
            {
                if((length_ratio == aComp.length_ratio) && (num_segments == aComp.num_segments) && (length_set == aComp.length_set))
                    return true;
                return false;
            }
            bool operator!=(const PARAMS& aComp) const 
            {
                return !(*this == aComp);
            }
        };

        ROUNDEDTRACKSCORNER(const BOARD_ITEM* aParent, const TRACK* aTrackSeg, const TRACK* aTrackSegSecond, const wxPoint aPosition, const PARAMS aParams, const bool aCheckNullTrack);
        ~ROUNDEDTRACKSCORNER(){;}

        wxString GetClass() const override { return wxT( "ROUNDEDTRACKSCORNER" ); } //override TRACK.
        EDA_ITEM* Clone() const override; //override TRACK.
        const EDA_RECT GetBoundingBox() const override; //override TRACK.
        const wxPoint& GetPosition() const override { return m_mid_pos; } 
        
        void TransformShapeWithClearanceToPolygon( SHAPE_POLY_SET& aCornerBuffer,
                                                int             aClearanceValue,
                                                int             aCircleToSegmentsCount,
                                                double          aCorrectionFactor ) const override; //override TRACK.
        wxString GetSelectMenuText() const override; //override TRACK.
        void AddTo3DContainer(CBVHCONTAINER2D* aContainer, const double aBiuTo3Dunits) override;        
        
        void SetParams(const PARAMS aParams);
        PARAMS GetParams(void) const;

        const wxPoint& GetStartVisible(void) const {return m_pos_start;}
        const wxPoint& GetEndVisible(void) const {return m_pos_end;}
        
        double GetLengthVisible(void) const;

        bool Update(void) override;

        bool IsSetOK(void) const {return m_set_ok;}
        uint GetBoundingRad(void) const override;
        
        void SetTrackSecondEndpoint(void);
        void ReleaseTrackSegs(void);
        void ConnectTrackSegs(void);
        void ResetVisibleEndpoints(void);
        TRACK* GetTrackSegSecond(void) const { return m_trackseg_second; }
        wxPoint GetPointInArc(const wxPoint aFromPos, const double aLength) const;

    protected:
        bool CallConstructor(const TRACK* aTrackSeg, const PARAMS aParams, const bool aCheckNullTrack);

        void DrawItem(EDA_DRAW_PANEL* aPanel, wxDC* aDC, const COLOR4D aColor, const wxPoint& aOffset, const DISPLAY_OPTIONS* aDisplOpts) override;
        void DrawItem(KIGFX::GAL* aGal, const bool aIsSketch ) override;

        void SetTracksSegSecond(const TRACK* aNewTrackSecond) {m_trackseg_second = const_cast<TRACK*>(aNewTrackSecond);}
    
    private:
        static constexpr uint DEFAULT_SEGMENTS_NUM = DEFAULT_NUM_SEGMENTS;
        static constexpr uint DEFAULT_OUTER_POLY_POINTS_NUM = DEFAULT_NUM_SEGMENTS * 2 + 2;
        
        bool m_on{true}; //On / Off
        
        int m_length_ratio;
        int m_num_arc_segs;
        uint m_length_set; 
        
        wxPoint m_pos{0,0};
        wxPoint m_pos_start{0,0};   // = m_seg_points[0]. Arc starting point : m_trackseg side.
        wxPoint m_pos_end{0,0};     // = m_seg_points[m_num_arc_segs]. Arc last point : m_trackseg_second side.
        wxPoint m_mid_pos;
        
        TRACK* m_trackseg_second;
        uint m_trackseg_second_length;
        bool m_trackseg_second_startpos_is_pos;

        //Own drawing modes. 
        bool m_draw_mode_unfill{false};     //Graphic Edit.
        
        bool m_set_ok{true};
        void SetArc(void);
        void SetPoints(void);
        void SetTracksVisibleEndpoints(void);         //Set ROUNDEDCORNERTRACK Visible points:
        int m_num_arc_outer_points;

        uint m_trackseg_arced_length;       //segments arced length 
        double m_angle_btw_tracks;          //Angle between tracks;
        double m_angle_inner_btw_tracks;    //Inner side between tracks angle.
        double m_angle_inner_half;          //Inner half angle;
        wxPoint m_arc_center_pos{0,0};         
        uint m_arc_radius;                  //Arc radius
        
        void SetNotOKValues(void);
    };

//-----------------------------------------------------------------------------------------------------/
// Track routing edit corner.
//-----------------------------------------------------------------------------------------------------/
    class ROUNDEDTRACKSCORNER_ROUTE_EDIT : public ROUNDEDTRACKSCORNER
    {
    public:
        ROUNDEDTRACKSCORNER_ROUTE_EDIT(const BOARD_ITEM* aParent, const TRACK* aTrackSeg, const TRACK* aTrackSegSecond, const PARAMS aParams, const bool aCheckNullTrack);
        ~ROUNDEDTRACKSCORNER_ROUTE_EDIT(){;}
    
        wxString GetClass() const override { return wxT( "ROUNDEDTRACKSCORNER_ROUTE_EDIT" ); }

        bool ChangeTracks(const TRACK* aNewTrack, const TRACK* aNewTrackSecond);    
        
    protected:
        void DrawItem(EDA_DRAW_PANEL* aPanel, wxDC* aDC, const COLOR4D aColor, const wxPoint& aOffset, const DISPLAY_OPTIONS* aDisplOpts) override;
    };

} //namespace TrackNodeItem


#endif //ROUNDEDTRACKSCORNER_H

