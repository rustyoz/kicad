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
 * @file tracknodeitem.h
 * @brief Definitions for tracknodeitem.
 */

#ifndef TRACKNODEITEM_H
#define TRACKNODEITEM_H

#include <math.h>
#include <class_track.h>
#include <view/view_item.h>
#include <pcb_painter.h>
#include <cstddef>
#include <class_pad.h>
#include <wxBasePcbFrame.h>
#include <wxPcbStruct.h>
#include <class_drawpanel.h>
#include <class_board.h>
#include <class_module.h>
#include <pcbnew.h>
#include <trigo.h>
#include <drc_stuff.h>
#include <wx/progdlg.h>
#include <limits.h>
#include <gal/graphics_abstraction_layer.h>
#include <3d_rendering/3d_render_raytracing/accelerators/ccontainer2d.h>

namespace TrackNodeItem
{
    constexpr double M_PIx2 = M_PI * 2.0;
    constexpr double M_PI_4x3 = M_PI_4 * 3.0;
    constexpr inline int Rad2MilsInt(const double aRadAngle);
    using Tracks_Container = std::set<TRACK*>;

    //Base class to make polygons to TRACK endpoints.
    class TRACKNODEITEM : public TRACK
    {
    public:
        ~TRACKNODEITEM(){};
        bool IsCreatedOK(void) const { return m_created_ok; }
        void Draw( EDA_DRAW_PANEL* aPanel, wxDC* aDC, GR_DRAWMODE aDrawMode, const wxPoint& aOffset = ZeroOffset ) override; //override TRACK.
        void Draw( KIGFX::GAL* aGal, const KIGFX::PCB_RENDER_SETTINGS* aPcbSettings, const float aOutlineWidth, int aLayer); //Gal draw.

        TRACK* GetTrackSeg(void) const { return m_trackseg; }
        double GetTrackSegAngle(void) const { return m_trackseg_angle; }
        
        void SetTrackEndpoint(void); //Sometimes when update need to set. (undo redo)
        bool IsTrackEndpointStart(void) const {return m_trackstartpos_is_pos;}
        bool IsTrackEndpointEnd(void) const {return !m_trackstartpos_is_pos;}

        wxPoint GetPolyPoint(const uint aPoint) const;
        uint GetPolyPointsNum(void) const { return m_seg_points.size(); }
        uint GetPolySegmentWidth(void) const {return m_poly_seg_width; }

        virtual uint GetBoundingRad(void) const = 0;
        virtual bool Update(void);

        virtual void AddTo3DContainer(CBVHCONTAINER2D* aContainer, const double aBiuTo3Dunits) = 0;    
        
    protected:
        TRACKNODEITEM(const BOARD_ITEM* aParent, KICAD_T aID_Type);
        //CallConstruct must be call from derived class constructor or overloaded CallConsttructor.
        bool CallConstructor(const TRACK* aTrackSeg, const bool aCheckNullTrack);
        virtual void DrawItem(EDA_DRAW_PANEL* aPanel, wxDC* aDC, const COLOR4D aColor, const wxPoint& aOffset, const DISPLAY_OPTIONS* aDisplOpts)=0;
        //Gal draw
        virtual void DrawItem(KIGFX::GAL* aGal, const bool aIsSketch )=0;
        
        bool SetTrackSeg(const TRACK* aTrackSeg, const bool aCheckNullTrack);
        TRACK* m_trackseg; //Track segment where polygon belongs to.
        bool m_trackstartpos_is_pos;
        double m_trackseg_angle;
        double m_trackseg_angle_inv;
        uint m_trackseg_rad;
        uint m_trackseg_length;
        //base class m_Width = m_trackseg width

        void SetConnectedPos(void);
        wxPoint m_connected_pos{0,0};   //Tracks connected position. Via, Pad, track endpoint or node.
        wxPoint m_opposite_pos{0,0};    //Tracks opposite pos of connected pos.
        
        static const uint MIN_POLYSEG_WIDTH = 2540; //0.1 mils 
        uint m_poly_seg_rad; 
        uint m_poly_seg_width;
        std::vector<wxPoint> m_seg_points;
        std::vector<wxPoint> m_seg_outer_points;

        uint m_clearance;
        std::vector<wxPoint> m_seg_clearance_points;

        bool m_created_ok{false};
    };


    inline double cathete(const unsigned long aHyp, const unsigned long aCath) 
    {
        return sqrt(pow(aHyp, 2) - pow(aCath, 2));
    }

    inline long double CosineAlpha(const unsigned long long a, const unsigned long long b, const unsigned long long c)
    {
        unsigned long long x = b * b + c * c - a * a;
        unsigned long long y = 2 * b * c;
        return acos((long double)x / (long double)y);
    }
    
    inline double AngleRad(const wxPoint aPos1, const wxPoint aPos2)
    {
        if(aPos1.x != aPos2.x)
        {
            double slope = double(abs(aPos1.y - aPos2.y)) / double(abs(aPos1.x - aPos2.x));
            double angle = atan(slope);
            if( aPos1.x > aPos2.x ) 
            {   
                if(aPos1.y < aPos2.y)
                    return M_PI + angle;
                return M_PI - angle;
            }
            else
                if(aPos1.y < aPos2.y)
                    return M_PIx2 - angle;
        
            return angle;
        }
        else
        {
            if(aPos1.y > aPos2.y)
                return M_PI_2;
            return M_PI + M_PI_2;        
        } 
    }

    inline double Rad2DeciDegRnd(const double aRadAngle)
    {
        return round(RAD2DECIDEG(aRadAngle) / 10.0) * 10.0;
    }
    
    constexpr int ANGLE_0 = 0;
    constexpr int ANGLE_45 = 450;
    constexpr int ANGLE_90 = 900;
    constexpr int ANGLE_135 = 1350;
    constexpr int ANGLE_180 = 1800;
    constexpr int ANGLE_225 = 2250;
    constexpr int ANGLE_270 = 2700;
    constexpr int ANGLE_315 = 3150;
    constexpr int ANGLE_360 = 3600;
            
    //Rounding angle rads to integer whit it's desimal places.
    //Accuracy using angle when compre angles with radius representation.
    constexpr inline int Rad2MilsInt(const double aRadAngle)
    {
        return round(1000.0 * aRadAngle);
    }
    
    constexpr int RAD_0_MILS_INT = 0;
    constexpr int RAD_45_MILS_INT = Rad2MilsInt(M_PI_4);
    constexpr int RAD_90_MILS_INT = Rad2MilsInt(M_PI_2);
    constexpr int RAD_135_MILS_INT = Rad2MilsInt((M_PI_2 + M_PI_4));
    constexpr int RAD_180_MILS_INT = Rad2MilsInt(M_PI);
    constexpr int RAD_225_MILS_INT = Rad2MilsInt(M_PI + M_PI_4);
    constexpr int RAD_270_MILS_INT = Rad2MilsInt(M_PI + M_PI_2);
    constexpr int RAD_315_MILS_INT = Rad2MilsInt(M_PI + M_PI_2 + M_PI_4);
    constexpr int RAD_360_MILS_INT = Rad2MilsInt(M_PIx2);

    inline wxPoint GetPoint(const wxPoint aStartPoint, const double aAngle, const long aLengthFromStart)
    {
        long x = aStartPoint.x + aLengthFromStart * cos(aAngle);
        long y = aStartPoint.y - aLengthFromStart * sin(aAngle);
        return wxPoint(x, y); 
    }


    template <typename T> inline T NormAngle(const T aAngle, const T aMinNorm, const T aMaxNorm, const T aNorm)
    {
        T angle = aAngle;
        while(angle < aMinNorm)
            angle += aNorm;
        while(angle >= aMaxNorm)
            angle -= aNorm;
        return angle;
    }

    inline double TrackSegAngle(const TRACK* aTrackSeg, const wxPoint aPointWith)
    {
        double angle;
        (aTrackSeg->GetStart() == aPointWith)? angle = AngleRad(aTrackSeg->GetStart(), aTrackSeg->GetEnd()) : angle = AngleRad(aTrackSeg->GetEnd(), aTrackSeg->GetStart());
        return angle;
    }
    
    inline double AngleBtwTracks(const TRACK* aTrackSegA, const wxPoint aPosSegA, const TRACK* aTrackSegB, const wxPoint aPosSegB)
    {
        double angle_ret = M_PIx2 + M_PI_4; //Return value over 360 degerees if not ok.
        if(((aPosSegA == aTrackSegA->GetStart()) || (aPosSegA == aTrackSegA->GetEnd())) && ((aPosSegB == aTrackSegB->GetStart()) || (aPosSegB == aTrackSegB->GetEnd())))
        {
            double angle_cmp = TrackSegAngle(aTrackSegA, aPosSegA) - TrackSegAngle(aTrackSegB, aPosSegB);
            angle_ret = NormAngle(angle_cmp, 0.0, M_PIx2, M_PIx2);
        }
        return angle_ret;
    }
    
    inline bool IsSharpAngle(const double aAngle, const bool aRoundedCorner)
    {
        int angle_cmp = Rad2MilsInt(aAngle);
        if(aRoundedCorner)
        {
            constexpr int angle_min_rounded = Rad2MilsInt(M_PI_2);
            constexpr int angle_max_rounded = Rad2MilsInt(M_PI + M_PI_2);
            if((angle_cmp < angle_min_rounded) || (angle_cmp > angle_max_rounded))
                return true;
        }
        else
        {
            constexpr int angle_min = Rad2MilsInt(M_PI_2 + M_PI_4);
            constexpr int angle_max = Rad2MilsInt(M_PI + M_PI_4);
            if((angle_cmp < angle_min) || (angle_cmp > angle_max))
                return true;
        }
        return false;
    }
    
    inline bool IsSharpAngle(const TRACK* aTrackSegA, const TRACK* aTrackSegB, const wxPoint aCommonPos, const bool aRoundedCorner)
    {
        return IsSharpAngle(AngleBtwTracks(aTrackSegA, aCommonPos, aTrackSegB, aCommonPos), aRoundedCorner);
    }
    
    void Collect(const TRACK* aTrackSeg, const wxPoint aPosAt, Tracks_Container& aTracksList);
    int GetMaxWidth(Tracks_Container& aTracksList);
    TRACK* GetMaxWidthTrack(Tracks_Container& aTracksList);
    //Tracks addresses T connected with atTrackSeg are returned aTrackSegNext and aTrackSegBack.
    bool Find_T_Tracks(const TRACK* aTrackSegAt, const wxPoint aPosAt, TRACK* &aTrackSegNext, TRACK* &aTrackSegBack);

    constexpr double ALPHA_INC = M_PIx2 / 64;
    void DrawArc(KIGFX::GAL* aGal, const wxPoint& aCenterPoint, const double aStartAngle, const double aEndAngle, const double aRad);
    void DrawArc(KIGFX::GAL* aGal, const wxPoint& aCenterPoint, const wxPoint& aStartPoint, const wxPoint& aEndPoint, const double aRad);

    class SCAN_NET_BASE
    {
    public:
        virtual ~SCAN_NET_BASE(){};
        
        void Execute(void);
        
    protected:
        SCAN_NET_BASE(){};
        SCAN_NET_BASE(const TRACK* aNetStartSeg);
        virtual bool ExecuteAt(const TRACK* aTrackSeg)=0;
        
        TRACK* m_net_start_seg{nullptr};
        
        bool m_reverse{false};
    };

    class SCAN_NET_COLLECT : public SCAN_NET_BASE
    {
    public:
        SCAN_NET_COLLECT(const TRACK* aTrackSeg, const wxPoint aPosAt, Tracks_Container* aTracksList);
        ~SCAN_NET_COLLECT(){};
        
    protected:
        bool ExecuteAt(const TRACK* aTrackSeg) override;
        wxPoint m_pos{0,0};
        PCB_LAYER_ID m_layer;
        Tracks_Container* m_tracks_list{nullptr};
    };
    
    class SCAN_NET_FIND_T_TRACKS : public SCAN_NET_COLLECT
    {
    public:
        SCAN_NET_FIND_T_TRACKS(const TRACK* aTrackSeg, const wxPoint aPosAt, Tracks_Container* aTracksList);
        ~SCAN_NET_FIND_T_TRACKS(){};
        
        TRACK* GetResultTrack(bool aFirstSecond) const;
        
    protected:
        bool ExecuteAt(const TRACK* aTrackSeg) override;
    private:
        double m_angle_next;
        double m_angle_back;
        int m_angle_next_rnd;
        int m_angle_back_rnd;
        TRACK* m_result_track_0;
        TRACK* m_result_track_1;
    };
    
    bool EndPosNearest(const TRACK* aTrackSegAt, const wxPoint aPos);
    bool StartPosNearest(const TRACK* aTrackSegAt, const wxPoint aPos);
    wxPoint TrackSegNearestEndpoint(const TRACK* aTrackSegAt, const wxPoint aPos);
    wxPoint TracksCommonPos(const TRACK* aTrackSegFirst, const TRACK* aTrackSegSecond);
}//namespace TrackNodeItem

#endif //TRACKNODEITEM_H
