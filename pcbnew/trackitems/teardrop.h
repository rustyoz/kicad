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

/**
 * @file teardrop.h
 * @brief Definitions for teardrop.
 */

#ifndef TEARDROP_H
#define TEARDROP_H

#include "tracknodeitem.h"

namespace TrackNodeItem
{

    class TEARDROP : public TrackNodeItem::TRACKNODEITEM
    {
    public:
        
        static const int TEARDROP_SEGS_MAX = 10; //Shape Teardrop max num segments.
        static const int TEARDROP_SEGS_MIN = 2; //Shape Teardrop min num segments.
        static const int SUBLAND_SIZE_MAX = 100; //Shape Subland max size.
        static const int SUBLAND_POS_MAX = 500; //Shape Subland max position. Teardrop & Fillet has no max.
        static const int TEARDROPS_WIDTH_MAX = 1000; //common shapes width max.
        static const int TEARDROPS_LENGTH_MAX = 5000; //common shapes length max.
        static const int TEARDROPS_SIZE_MIN = 1; //common shapes size min.
        static const int TEARDROPS_POS_MIN = 1; //common shapes position min.
        
        enum SHAPES_T 
        {
            NULL_T = 0,
            TEARDROP_T,
            FILLET_T,
            SUBLAND_T,
            ZERO_T,
        };
        
        class PARAMS
        {
        public:
            SHAPES_T shape;
            int length_ratio;   //At shape SUBLAND_T max value is 100. Others may have bigger values.
            int width_ratio;   
            int num_segments;
            bool operator==(const PARAMS& aComp) const 
            {
                if((shape == aComp.shape) && (length_ratio == aComp.length_ratio) && (width_ratio == aComp.width_ratio) && (num_segments == aComp.num_segments))
                    return true;
                return false;
            }
            bool operator!=(const PARAMS& aComp) const 
            {
                return !(*this == aComp);
            }
        };

        ~TEARDROP(){};
        
        //Base overrides, overloads, hides ...
        wxString GetClass() const override { return wxT( "TEARDROP" ); }
        EDA_ITEM* Clone() const override; 
        const EDA_RECT GetBoundingBox() const override;
        
        void TransformShapeWithClearanceToPolygon( SHAPE_POLY_SET& aCornerBuffer,
                                                int             aClearanceValue,
                                                int             aCircleToSegmentsCount,
                                                double          aCorrectionFactor ) const override;
        wxString GetSelectMenuText() const override;
        void AddTo3DContainer(CBVHCONTAINER2D* aContainer, const double aBiuTo3Dunits) override;        
        
        const wxPoint& GetPosition() const override { return m_mid_pos; } // Center point of teardrop    
        void SetPosition( const wxPoint& aPoint ) override {}; 
        void SetEnd( const wxPoint& aEnd ) {}; 
        void SetStart( const wxPoint& aStart ); //Use this, when change position. Same as connected item position.
        
        int GetShape() const { return m_shape; }
        
        wxString GetShapeName(void) const; 

        bool IsSetOK(void) const { return m_set_ok; }
        
        uint GetBoundingRad(void) const override; //Max radius from mid. Depend on shape
        uint GetCalcLength(void) const; //Max teardrop calculated lengt from item pos to teardrop pos, depend on shape. How long it should have be.
        uint GetRealLength(void) const; //Max real length from item. How long it is.
        uint GetSizeLength(void) const; //Only teardrops length
        
        BOARD_CONNECTED_ITEM* GetConnectedItem(void) const { return m_connected_item; }
        virtual wxString GetConnectedItemName(void) const;

        void SetDrawMode_Unfill(const bool aSetValue) { m_draw_mode_unfill = aSetValue; }

        void SetParams(const PARAMS aParams);
        PARAMS GetParams(void) const;
        
        bool Update(void) override;
        
        bool operator==(const TEARDROP& aComp) const
        {
            if((this != &aComp) && (GetEnd() == aComp.GetEnd()) && (GetLayer() == aComp.GetLayer()) && (TrackNodeItem::Rad2MilsInt(GetTrackSegAngle()) == TrackNodeItem::Rad2MilsInt(aComp.GetTrackSegAngle())))
                return true;
            return false;
        }
        bool operator!=(const TEARDROP& aComp) const 
        {
            return !(*this == aComp);
        }
        
    protected:

        //Derived classes must call this.
        TEARDROP(const BOARD_ITEM* aParent, const TRACK* aTrackSeg);
        //CallConstruct must be call at the end from derived class constructor.
        bool CallConstructor(const TRACK* aTrackSeg, const PARAMS aParams, const bool aCheckNullTrack);

        inline uint CalcSublandPosDist(const int aLengthRatio, const uint aConnectedItemRad, const uint aSublandRad);
        inline uint CalcFilletPosDist(const int aLengthRatio, const uint aConnectedItemRad);
        inline uint CalcTeardropPosDist(const int aLengthRatio, const uint aConnectedItemRad);
        
        inline uint CalcSublandRad(const int aWidthRatio, const uint aConnectedItemRad, const uint aTrackSegRad);
        inline uint CalcFilletWidthRad(const int aWidthRatio, const uint aConnectedItemRad, const uint aTrackSegRad);
        inline uint CalcTeardropWidthRad(const int aWidthRatio, const uint aConnectedItemRad, const uint aTrackSegRad);

        inline uint CalcFilletLength(const int aLengthRatio);
        inline uint CalcTeardropLength(const int aLengthRatio);

        virtual void CalcSubland(const int aWidthRatio, const int aLengthRatio);
        virtual void CalcFillet(const int aWidthRatio, const int aLengthRatio);
        virtual void CalcTeardrop(const int aWidthRatio, const int aLengthRatio);

        virtual void SetFilletPoints(void);
        virtual void SetTeardropPoints(void);

        inline void CalcFilletSegs(void);
        inline void CalcTeardropArcs(void);
        
        virtual void SetConnectedItem(void){};
        BOARD_CONNECTED_ITEM* m_connected_item;
        uint m_connected_item_rad; //Connected item radius. Via, Pad or something else.
        wxPoint m_connected_pos_delta; //Teardrop delta pos from item.
        int m_connected_pos_length_delta; //Distance item to teardrop delta length.
        int m_length_width_corr; //??
	
        wxPoint m_pos;
        uint m_width_rad;
        uint m_length;

        int m_length_ratio;
        int m_width_ratio;

        uint m_fillet_seg_length;
        double m_fillet_seg_A_angle;
        double m_fillet_seg_B_angle;

        double m_teardrop_segs_angle_add;
        ulong m_teardrop_segs_arc_rad;
        wxPoint m_teardrop_arc_A_center_pos;
        wxPoint m_teardrop_arc_B_center_pos;
        
        int m_num_arc_segs;
        
        bool m_can_draw_clearance;
        
        virtual void TestTrackLength(void);
        virtual void TestTrackWidth(void);
        virtual void TestSize(void);
        virtual void SetNotOKValues(void);
        bool m_set_ok{true};
        
        void DrawItem(EDA_DRAW_PANEL* aPanel, wxDC* aDC, const COLOR4D aColor, const wxPoint& aOffset, const DISPLAY_OPTIONS* aDisplOpts) override;
        void DrawItem(KIGFX::GAL* aGal, const bool aIsSketch ) override;

    private:
        static const uint FILLET_POLY_POINTS_NUM = 4;
        static const uint FILLET_OUTER_POLY_POINTS_NUM = 6;
        static const uint FILLET_CLEARANCE_POLY_POINTS_NUM = FILLET_OUTER_POLY_POINTS_NUM;
        
        SHAPES_T m_shape;
        wxPoint m_mid_pos;

        //Own drawing modes. 
        bool m_draw_mode_unfill; //Graphic Edit.
        
        void SetSize(void);
        void SetPoints(void);
        void SetSegments(void);
        void SetRoundedCornerTrack(void);
    };

//-----------------------------------------------------------------------------------------------------/
// VIA Teardrop
//-----------------------------------------------------------------------------------------------------/
    class TEARDROP_VIA : public TEARDROP
    {
    public:
        TEARDROP_VIA(const BOARD_ITEM* aParent, const VIA* aVia, const TRACK* aTrackSeg, const PARAMS aParams, const bool aCheckNullTrack);
        ~TEARDROP_VIA(){};
    
        wxString GetClass() const override { return wxT( "TEARDROP_VIA" ); }

        VIA* GetConnectedVia(void) const { return m_connected_via; }
        void SetViaRad(const uint aNewViaRad);
        uint GetViaRad(void) const { return m_connected_item_rad; }
        
    protected:
        void SetConnectedItem(void) override;
        VIA* m_connected_via;
    };

//-----------------------------------------------------------------------------------------------------/
// Routing edit teardrop.
//-----------------------------------------------------------------------------------------------------/
    class TEARDROP_EDIT_VIA : public TEARDROP_VIA
    {
    public:
        TEARDROP_EDIT_VIA(const BOARD_ITEM* aParent, const uint aViaRadius, const TRACK* aTrackSeg, const PARAMS aParams, const bool aCheckNullTrack);
        ~TEARDROP_EDIT_VIA(){;}
    
        wxString GetClass() const override { return wxT( "TEARDROP_EDIT_VIA" ); }

        bool ChangeTrack(const TRACK* aNewTrack);    
    };

//-----------------------------------------------------------------------------------------------------/
// Rectangle shape items teardrop.
//-----------------------------------------------------------------------------------------------------/
    class TEARDROP_RECT : public TEARDROP
    {
    public:
        ~TEARDROP_RECT(){;}

        
    protected:
        TEARDROP_RECT(const BOARD_ITEM* aParent, const TRACK* aTrackSeg);
        void SetConnectedItem(void) override;

        void SetNotOKValues(void) override;
        
        void CalcFillet(const int aWidthRatio, const int aLengthRatio) override;
        void SetFilletPoints(void) override;
        
        void CalcTeardrop(const int aWidthRatio, const int aLengthRatio) override;
        void SetTeardropPoints(void) override;
        
        //void TestTrackLength(void) override;
        
        bool m_rect_type = false;
        int m_rect_edge_dist_x;

        void CalcSegment(void);
    };

//-----------------------------------------------------------------------------------------------------/
// PAD Teardrop
//-----------------------------------------------------------------------------------------------------/
    class TEARDROP_PAD : public TEARDROP_RECT
    {
    public:
        TEARDROP_PAD(const BOARD_ITEM* aParent, const D_PAD* aPad, const TRACK* aTrackSeg, const PARAMS aParams, const bool aCheckNullTrack);
        ~TEARDROP_PAD(){;}

        wxString GetClass() const override { return wxT( "TEARDROP_PAD" ); }

        void ChangePad(const D_PAD* aPad);
        D_PAD* GetConnectedPad(void) const {  return m_connected_pad; }
        
        bool IsAngleOK(void) const; //Is teardrop in right angle various pad shapes.
        
    protected:
        void SetConnectedItem(void) override;

    private:
        D_PAD* m_connected_pad;
    };
      
//-----------------------------------------------------------------------------------------------------/
// Junction and T-Junction.
//-----------------------------------------------------------------------------------------------------/
    class TEARDROP_JUNCTIONS : public TEARDROP_RECT
    {
    public:
        TEARDROP_JUNCTIONS(const BOARD_ITEM* aParent, const TRACK* aTrackSeg, const wxPoint aPosition, const PARAMS aParams, const bool aCheckNullTrack);
        ~TEARDROP_JUNCTIONS(){;}
    
        wxString GetClass() const override { return wxT("TEARDROP_JUNCTIONS"); }
        wxString GetConnectedItemName(void) const override { return wxT("JUNCTION"); }
        
        TRACK* Get_T_SegNext(void) const { return m_T_track_next; }
        TRACK* Get_T_SegBack(void) const { return m_T_track_back; }
        
        bool Is_T_Junction(void) const { return m_T_tracks; }
        
    protected:
        void SetConnectedItem(void) override;

        void CalcSubland(const int aWidthRatio, const int aLengthRatio) override;
        void CalcFillet(const int aWidthRatio, const int aLengthRatio) override;
        void CalcTeardrop(const int aWidthRatio, const int aLengthRatio) override;
        
        void TestTrackLength(void) override;

    private:
        TrackNodeItem::Tracks_Container m_connected_tracks;
        void CollectTracks(const TRACK* aTrackSeg, const wxPoint aPos);
        bool Find_T_FromTracks(void);
        int MaxWidthFromTracks(void);
        TRACK* m_T_track_next = nullptr;
        TRACK* m_T_track_back = nullptr;
        bool m_T_tracks = false;
    };

}//Namespace TrackNodeItem

#endif /* TEARDROP_H */
