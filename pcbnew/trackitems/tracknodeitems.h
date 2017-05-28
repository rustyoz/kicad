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
 * @file tracknodeitems.h
 * @brief Definitions for tracknodes base class.
 */

#ifndef TRACKNODEITEMS_H
#define TRACKNODEITEMS_H

#include "tracknodeitem.h"
#include "teardrop.h"

#include <class_board.h>
#include <wxPcbStruct.h>
#include <pcb_parser.h>
#include <cstddef>
#include <drag.h>
#include <class_board_item.h>
#include <class_pcb_text.h>
#include <wx/progdlg.h>
#include <pcbnew_id.h>
#include <menus_helpers.h>
#include <drc_stuff.h>

namespace TrackNodeItems
{
//-----------------------------------------------------------------------------------------------------------   
// PROGRESS OPERATIONS 
//-----------------------------------------------------------------------------------------------------------   
    class ITEMS_PROGRESS_BASE
    {

    public:
        virtual ~ITEMS_PROGRESS_BASE();
        uint Execute(void);

    protected:
        ITEMS_PROGRESS_BASE() {};
        ITEMS_PROGRESS_BASE(const PCB_EDIT_FRAME* aFrame, const BOARD_ITEM* aListFirstItem, PICKED_ITEMS_LIST* aUndoRedo);

        virtual uint ExecuteItem(const BOARD_ITEM* aItemAt)=0; 
        virtual void ExecuteEnd(void) {};

        PICKED_ITEMS_LIST* m_undoredo_items;

        wxString m_progress_title;
        bool m_can_cancel{true};
        int m_progress_style {0};
        bool m_cancelled {false};

    private:
        int m_items_to_count {0};
        int m_progress_to_count {0};

        wxProgressDialog* m_progress {nullptr};
        inline bool UpdateProgress(const uint aProgress, const uint aOperations);

        const BOARD_ITEM* m_list_first_item{nullptr};
        PCB_EDIT_FRAME* m_frame{nullptr};
    };


//-----------------------------------------------------------------------------------------------------------   
// MODULES_PROGRESS (modules PADs) 
//-----------------------------------------------------------------------------------------------------------   
    class MODULES_PROGRESS : public ITEMS_PROGRESS_BASE
    {
    public:
        MODULES_PROGRESS(const PCB_EDIT_FRAME* aFrame, const DLIST<MODULE>* aModules, PICKED_ITEMS_LIST* aUndoRedo);

    protected:
        MODULES_PROGRESS(){;}

        uint ExecuteItem(const BOARD_ITEM* aItemAt) override;
        virtual uint DoAtPad(const D_PAD* aPadAt) { return 0; };
    };

//-----------------------------------------------------------------------------------------------------------   
// TRACKS_PROGRESS 
//-----------------------------------------------------------------------------------------------------------   
    class TRACKS_PROGRESS : public ITEMS_PROGRESS_BASE
    {
    public:
        TRACKS_PROGRESS(const PCB_EDIT_FRAME* aFrame, const DLIST<TRACK>* aTracks, PICKED_ITEMS_LIST* aUndoRedo) : ITEMS_PROGRESS_BASE(aFrame, aTracks->GetFirst(), aUndoRedo) {;}
        
    protected:
        TRACKS_PROGRESS(){;}

    };


//-----------------------------------------------------------------------------------------------------------   
// TRACKNODEITEMS Base 
//-----------------------------------------------------------------------------------------------------------   
    class TRACKNODEITEMS
    {
    public:
        TRACKNODEITEMS(const TRACKITEMS* aParent, const BOARD* aBoard);
        virtual ~TRACKNODEITEMS();
        void SetEditFrame(const PCB_EDIT_FRAME* aEditFrame);
        
        BOARD* GetBoard(void) const { return m_Board; }
        PCB_EDIT_FRAME* GetEditFrame(void) const { return m_EditFrame; }
        TRACKITEMS* GetParent(void) const { return m_Parent; }

        virtual void Plot(const TrackNodeItem::TRACKNODEITEM* aTrackNodeItem, PLOTTER* aPlotter, const EDA_DRAW_MODE_T* aPlotMode, void* aData ) = 0;

    protected:
        TRACKNODEITEMS(){};
        
        BOARD* m_Board;
        PCB_EDIT_FRAME* m_EditFrame;
        TRACKITEMS* m_Parent;
        
    //-----------------------------------------------------------------------------------------------------------   
    //Menus
    //-----------------------------------------------------------------------------------------------------------   
    public:
        wxMenu* GetMenu(void) const { return m_menu; }
        void RecreateMenu(void);
        
    protected:
        wxMenu* m_menu;
        virtual void CreateMenu(wxMenu* aMenu) const = 0;
    //-----------------------------------------------------------------------------------------------------------   
        
    //-----------------------------------------------------------------------------------------------------------   
    // Get item.
    //-----------------------------------------------------------------------------------------------------------
    public:
        using TrackNodeItem_Container = std::set<TrackNodeItem::TRACKNODEITEM*>;
        TrackNodeItem::TRACKNODEITEM* Get(const TRACK* aTrackSegAt, const wxPoint& aPosAt) const;
        TrackNodeItem::TRACKNODEITEM* Get(const TRACK* aTrackSegAt, const wxPoint& aPosAt, const bool aExcactPos) const;
        virtual TrackNodeItem::TRACKNODEITEM* Next(const TRACK* aTrackSegAt) const = 0;
        virtual TrackNodeItem::TRACKNODEITEM* Back(const TRACK* aTrackSegAt) const = 0;
        
    protected:
        TrackNodeItem_Container* m_get_list;
        void AddGetList(const TRACK* aTrackSegFrom);
    //-----------------------------------------------------------------------------------------------------------   

    //-----------------------------------------------------------------------------------------------------------   
    // DRC
    //-----------------------------------------------------------------------------------------------------------   
    protected:
        STATUS_FLAGS DRC_Flags(const STATUS_FLAGS aStatus);
        bool TestSegment(const wxPoint aStartPoint, const wxPoint aEndPoint, const wxPoint aTestPoint, const int aMinDist);
    //-----------------------------------------------------------------------------------------------------------   

    //-----------------------------------------------------------------------------------------------------------   
    // Save, Load
    //-----------------------------------------------------------------------------------------------------------   
    public:
        virtual void Format(OUTPUTFORMATTER* aOut, const int aNestLevel ) const throw( IO_ERROR ) = 0;
        virtual TrackNodeItem::TRACKNODEITEM* Parse(PCB_PARSER* aParser) throw( IO_ERROR, PARSE_ERROR ) = 0;
        
    protected:
        TRACK* GetTrackSegment(const wxPoint aStart, const wxPoint aEnd, const int aLayer, const int aNetCode) const;
    //-----------------------------------------------------------------------------------------------------------   
    
    //-----------------------------------------------------------------------------------------------------------   
    // Net scan common
    //-----------------------------------------------------------------------------------------------------------   
    protected:
        class NET_SCAN_BASE : public TrackNodeItem::SCAN_NET_BASE
        {
        public:
            ~NET_SCAN_BASE() {};

        protected:
            NET_SCAN_BASE() {};
            NET_SCAN_BASE(const TRACK* aTrackSeg, const TRACKNODEITEMS* aParent) : SCAN_NET_BASE(aTrackSeg) {
                m_Parent = const_cast<TRACKNODEITEMS*>(aParent);
            }

            TRACKNODEITEMS* m_Parent {nullptr};
        };
    //-----------------------------------------------------------------------------------------------------------   

    };

} //namespace TrackNodeItems

#endif //TRACKNODEITEMS_H


