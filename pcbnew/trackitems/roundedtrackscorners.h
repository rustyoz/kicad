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
 * @file rounded_trackscorners.h
 * @brief Definitions for teardrop using class.
 */

#ifndef ROUNDEDTRACKSCORNERS_H
#define ROUNDEDTRACKSCORNERS_H

//Interface of ROUNDEDTRACKSCORNERS.

#include "tracknodeitem.h"
#include "tracknodeitems.h"
#include "roundedtrackscorner.h"
#include "roundedcornertrack.h"

#include <drag.h>
#include <board_commit.h>


class ROUNDEDTRACKSCORNERS : public TrackNodeItems::TRACKNODEITEMS
{
public:
    static const wxString TXT_ROUNDEDTRACKSCORNERS;

    ROUNDEDTRACKSCORNERS(const TRACKITEMS* aParent, const BOARD* aBoard);
    ~ROUNDEDTRACKSCORNERS();

    inline bool IsOn(void) const { return m_params.length_ratio; }
    
    //One aCorner
    void Add(TRACK* aTrackSegTo, const wxPoint& aCurPosAt); 
    TrackNodeItem::ROUNDEDTRACKSCORNER* Add(TRACK* aTrackSegTo, const wxPoint aPosition, PICKED_ITEMS_LIST* aUndoRedoList);
    void Add(TRACK* aTrackSegTo, PICKED_ITEMS_LIST* aUndoRedoList);
    void Add(TRACK* aTrackSegTo); //Track routing trace.
    //All
    void Add(const DLIST<TRACK>* aTracksAt);
    void Add(const int aNetCodeTo, PICKED_ITEMS_LIST* aUndoRedoList);

    void Remove(const TRACK* aTrackItemFrom, const bool aUndo, const bool aLockedToo);
    void Remove(const TRACK* aTrackItemFrom, PICKED_ITEMS_LIST* aUndoRedoList, const bool aLockedToo);
    void Remove(const TRACK* aTrackItemFrom, BOARD_COMMIT& aCommit, const bool aLockedToo);
    void Remove(DLIST<TRACK>* aTracksAt); //All
    void Remove(const int aNetCodeFrom, PICKED_ITEMS_LIST* aUndoRedoList, const bool aLockedToo);
    
    void Change(const TRACK* aTrackItemFrom, const bool aUndo, const bool aLockedToo);

    void Recreate(const int aNetCodeTo, PICKED_ITEMS_LIST* aUndoRedoList);

    void Plot(const TrackNodeItem::TRACKNODEITEM* aTrackItem, PLOTTER* aPlotter, const EDA_DRAW_MODE_T* aPlotMode, void* aData ) override;

    //Sace, Load
    void Format(OUTPUTFORMATTER* aOut, const int aNestLevel ) const throw( IO_ERROR ) override;
    TrackNodeItem::TRACKNODEITEM* Parse(PCB_PARSER* aParser) throw( IO_ERROR, PARSE_ERROR ) override;
    
    void LoadDefaultParams(void);
    TrackNodeItem::ROUNDEDTRACKSCORNER::PARAMS CopyCurrentParams(const TRACK* aTrackSegAt, const wxPoint& aCurPosAt);
    
    TrackNodeItem::TRACKNODEITEM* Next(const TRACK* aTrackSegAt) const override;
    TrackNodeItem::TRACKNODEITEM* Back(const TRACK* aTrackSegAt) const override;
    
protected:
    ROUNDEDTRACKSCORNERS(){};
    void CreateMenu(wxMenu* aMenu) const override;
    
private:
    //Creation
    TrackNodeItem::ROUNDEDTRACKSCORNER* Create(const TRACK* aTrackSegTo, const TRACK* aTrackSegSecond, const wxPoint aPosition, const bool aNullTrackCheck);
    //Removing
    void Delete(TrackNodeItem::ROUNDEDTRACKSCORNER* aCorner, DLIST<TRACK>*aTrackListAt, PICKED_ITEMS_LIST* aUndoRedoList);

    //Private remove funcs.
    static const bool CAN_RECREATE = true;
    void Remove(TrackNodeItem::ROUNDEDTRACKSCORNER* aCorner, PICKED_ITEMS_LIST* aUndoRedoList, const bool aSaveRemoved, const bool aLockedToo);

    TRACK* FindSecondTrack(const TRACK* aTrackSegTo, wxPoint aPosition);
    
    //aTrack is converted to ROUNDEDCORNERTRACK in m_board->m_Track
    ROUNDEDCORNERTRACK* ConvertTrackInList(TRACK* aTrack, PICKED_ITEMS_LIST* aUndoRedoList); //One Track
    void ConvertTracksInList(const DLIST<TRACK>* aTracksAt, PICKED_ITEMS_LIST* aUndoRedoList); //All TRACKs in m_Tracks list.
    
    TrackNodeItem::ROUNDEDTRACKSCORNER::PARAMS GetParams(void) const { return m_params; }
    void SetParams(const TrackNodeItem::ROUNDEDTRACKSCORNER::PARAMS aParams);
    TrackNodeItem::ROUNDEDTRACKSCORNER::PARAMS GetDefaultParams(void) const;
    TrackNodeItem::ROUNDEDTRACKSCORNER::PARAMS m_params;
        
//-----------------------------------------------------------------------------------------------------/
// One track segment corner(s) memory 
//-----------------------------------------------------------------------------------------------------/
public:
    //Save 
    void ToMemory(const TRACK* aTrackSegFrom);
    void FromMemory(const TRACK* aTrackSegTo, PICKED_ITEMS_LIST* aItemsListPicker);
    void FromMemory(const TRACK* aTrackSegTo);
    void FromMemory(const TRACK* aTrackSegTo, BOARD_COMMIT& aCommit);

private:
    TrackNodeItem::ROUNDEDTRACKSCORNER* m_next_corner_in_memory;
    TrackNodeItem::ROUNDEDTRACKSCORNER* m_back_corner_in_memory;
//-----------------------------------------------------------------------------------------------------/

//-----------------------------------------------------------------------------------------------------/
// Todo lists.
//-----------------------------------------------------------------------------------------------------/
public:
    using RoundedTracksCorner_Container = std::set<TrackNodeItem::ROUNDEDTRACKSCORNER*>;
    using RoundedCornerTrack_Container = std::set<ROUNDEDCORNERTRACK*>;
    void UpdateListClear(void);
    void UpdateListAdd(const TRACK* aTrackSegFrom);
    void UpdateListDo(void);
    void UpdateListDo(EDA_DRAW_PANEL* aPanel, wxDC* aDC, GR_DRAWMODE aDrawMode, bool aErase);
    void UpdateListDo_UndoRedo(void);
    void UpdateListDo_BlockDuplicate(const wxPoint aMoveVector, PICKED_ITEMS_LIST* aUndoRedoList);

    void UpdateList_DrawTracks(EDA_DRAW_PANEL* aPanel, wxDC* aDC, GR_DRAWMODE aDrawMode);
    RoundedCornerTrack_Container* UpdateList_GetUpdatedTracks(void) const { return m_update_tracks_list; }

private:
    RoundedTracksCorner_Container* m_update_list;
    RoundedCornerTrack_Container* m_update_tracks_list;
    RoundedTracksCorner_Container* m_recreate_list;
    void UpdateListAdd(const TrackNodeItem::ROUNDEDTRACKSCORNER* aCorner);
//-----------------------------------------------------------------------------------------------------/

//-----------------------------------------------------------------------------------------------------/
// Update
//-----------------------------------------------------------------------------------------------------/
public:
    void Update(const BOARD_ITEM* aItemAt);
    void Update(const TRACK* aTrackSegAt);
    void Update(TRACK* aTrackSegAt, EDA_DRAW_PANEL* aPanel, wxDC* aDC, GR_DRAWMODE aDrawMode, bool aErase);
    
private:
    void Update(TrackNodeItem::ROUNDEDTRACKSCORNER* aCorner, EDA_DRAW_PANEL* aPanel, wxDC* aDC, GR_DRAWMODE aDrawMode, bool aErase);
//-----------------------------------------------------------------------------------------------------/
    
//-----------------------------------------------------------------------------------------------------/
// Track Routing (legacy canvas) Edit corner
//-----------------------------------------------------------------------------------------------------/
public:
    enum TO_EDIT_T
    {
        EDIT_NULL_T = 0,
        EDIT_LENGTH_RATIO_T,
        EDIT_LENGTH_SET_T,
    };
    void RouteCreate_Start(void);
    void RouteCreate_Stop(void);
    bool CanEdit(void) const { return m_can_edit;}
    void ToggleEdit(const TO_EDIT_T aEdit);
    bool IsEditOn(void) const { return (bool)m_to_edit; }
    TrackNodeItem::ROUNDEDTRACKSCORNER* UpdateRouteEdit(EDA_DRAW_PANEL* aPanel, wxDC* aDC, const TRACK* aTrack, const TRACK* aTrackSecond, const bool aErase, bool* aTrack45Only);
    void DestroyRouteEdit(void);
    TrackNodeItem::ROUNDEDTRACKSCORNER* GetEditCorner(void) const {return m_track_edit_corner;}
    
private:
    TrackNodeItem::ROUNDEDTRACKSCORNER* m_track_edit_corner{nullptr};
    TO_EDIT_T m_to_edit;
    bool m_can_edit;
    TrackNodeItem::ROUNDEDTRACKSCORNER::PARAMS m_edit_params;
    bool m_editparams_drawn{false};
    wxPoint m_edit_start_point;
    bool m_track45Only_before_edit;
    bool* m_track45Only;
    void DrawEditParams(EDA_DRAW_PANEL* aPanel, wxDC* aDC, const wxPoint& aOffset) const;
//-----------------------------------------------------------------------------------------------------/

//-----------------------------------------------------------------------------------------------------/
//DRC, return false on error.
//-----------------------------------------------------------------------------------------------------/
public:
    void AddToDragList(const TRACK* aTrackFrom, std::vector<DRAG_SEGM_PICKER>& aDragSegList);
    bool DRC_Clearance(const BOARD_CONNECTED_ITEM* aRefFirst, const BOARD_CONNECTED_ITEM* aRefSecond, const int aMinDist, DRC* aDRC);
    bool DRC_Clearance(const BOARD_CONNECTED_ITEM* aRef, const D_PAD* aPad, const int aMinDist, DRC* aDRC);
    void DRC_Clearance(const BOARD_CONNECTED_ITEM* aRef, const TEXTE_PCB* aText, const int aMinDist, DRC* aDRC);
    //void DRC_Rules(const Teardrop::TEARDROP* aTeardrop, DRC* aDRC);

private:
    bool DRC_DoClearanceTest(const TrackNodeItem::ROUNDEDTRACKSCORNER* aCorner, const wxPoint aTestPoint, const int aMinDist);
    bool DRC_DoClearanceTest(const TrackNodeItem::ROUNDEDTRACKSCORNER* aCorner, const D_PAD* aPad, const int aMinDist, DRC* aDRC);
    bool DRC_ClearanceTest(const TrackNodeItem::ROUNDEDTRACKSCORNER* aCorner, const VIA* aVia, const int aMinDist);
    bool DRC_ClearanceTest(const TrackNodeItem::ROUNDEDTRACKSCORNER* aCorner, const TRACK* aTrackSeg, const int aMinDist);
    bool DRC_ClearanceTest(const TrackNodeItem::ROUNDEDTRACKSCORNER* aCornerFirst, TrackNodeItem::ROUNDEDTRACKSCORNER* aCornerSecond, const int aMinDist);
    bool DRC_TestClearance(const TrackNodeItem::ROUNDEDTRACKSCORNER* aCorner, const TRACK* aTrackSeg, const int aMinDist, DRC* aDRC);
//-----------------------------------------------------------------------------------------------------/

//-----------------------------------------------------------------------------------------------------/
// Gal canvas commit  push
//-----------------------------------------------------------------------------------------------------/
public:
    void GalCommitPushPrepare(void);
    void GalCommitPushAdd(BOARD_ITEM* aItem, PICKED_ITEMS_LIST* aUndoRedoList);
    void GalCommitPushRemove(BOARD_ITEM* aItemFrom, PICKED_ITEMS_LIST* aUndoRedoList);
    void GalCommitPushFinish(PICKED_ITEMS_LIST* aUndoRedoList);
private:
    void GalRemovedListAdd(const TrackNodeItem::ROUNDEDTRACKSCORNER* aCorner);
    RoundedTracksCorner_Container* m_gal_removed_list;
    int m_current_routed_track_netcode;

    std::set<TRACK*>m_gal_commit_tracks;
//-----------------------------------------------------------------------------------------------------/


//-----------------------------------------------------------------------------------------------------/
// Menus
//-----------------------------------------------------------------------------------------------------/
public:
    void Popup(wxMenu* aMenu, const TRACK* aTrackSeg, const wxPoint& aPos) const;
    void MenuToDo_ChangeSize(const int aMenuID);

private:
    void Menu_AddToTrack(wxMenu* aMenu, const TRACK* aTrackSeg, const wxPoint& aPos) const;
    void Menu_RemoveFromTrack(wxMenu* aMenu, const TRACK* aTrackSeg, const wxPoint& aPos) const;
    void Menu_ChangeFromTrack(wxMenu* aMenu, const TRACK* aTrackSeg, const wxPoint& aPos) const;
    void Menu_CopyParamsToCurrent(wxMenu* aMenu, const TRACK* aTrackSeg, const wxPoint& aPos) const;

    wxString ParamsTxtToMenu(const TrackNodeItem::ROUNDEDTRACKSCORNER::PARAMS aParams) const;
    inline int MenuToDo_CalcSizeLengthSet(const int aMenuID);
    inline int MenuToDo_CalcSizeLengthRatio(const int aMenuID);
    void Menu_ChangeSize(wxMenu* aMenu) const;

//-----------------------------------------------------------------------------------------------------/
// NETSCAN OPERATIONS 
//-----------------------------------------------------------------------------------------------------/
private:

    class NET_SCAN_NET_ADD : public NET_SCAN_BASE
    {
    public:
        NET_SCAN_NET_ADD(const int aNet, const ROUNDEDTRACKSCORNERS* aParent, PICKED_ITEMS_LIST* aUndoRedoList);
        ~NET_SCAN_NET_ADD() {};

    protected:
        bool ExecuteAt(const TRACK* aTrackSeg) override;

        PICKED_ITEMS_LIST* m_picked_items {nullptr};
    };

    class NET_SCAN_NET_REMOVE : public NET_SCAN_BASE
    {
    public:
        NET_SCAN_NET_REMOVE(const int aNet, const ROUNDEDTRACKSCORNERS* aParent, PICKED_ITEMS_LIST* aUndoRedoList, RoundedTracksCorner_Container* aRecreateList, const bool aLockedToo);
        ~NET_SCAN_NET_REMOVE() {};

    protected:
        bool ExecuteAt(const TRACK* aTrackSeg) override;

        PICKED_ITEMS_LIST* m_picked_items {nullptr};
        RoundedTracksCorner_Container* m_recreate_list {nullptr};
        bool m_locked_too{false};
    };

    class NET_SCAN_NET_RECREATE : public NET_SCAN_NET_REMOVE
    {
    public:
        NET_SCAN_NET_RECREATE(const int aNet, const ROUNDEDTRACKSCORNERS* aParent, PICKED_ITEMS_LIST* aUndoRedoList, RoundedTracksCorner_Container* aRecreateList);
        ~NET_SCAN_NET_RECREATE();

    protected:
        bool ExecuteAt(const TRACK* aTrackSeg) override;
        
    private:
        TrackNodeItem::ROUNDEDTRACKSCORNER::PARAMS m_current_params;
    };

    class NET_SCAN_TRACK_UPDATE : public NET_SCAN_BASE
    {
    public:
        NET_SCAN_TRACK_UPDATE(const TRACK* aTrackSeg, const ROUNDEDTRACKSCORNERS* aParent);
        ~NET_SCAN_TRACK_UPDATE() {};

    protected:
        bool ExecuteAt(const TRACK* aTrackSeg) override;
    };

//-----------------------------------------------------------------------------------------------------/
// PROGRESS OPERATIONS 
//-----------------------------------------------------------------------------------------------------/
private:
    
    class ROUNDEDTRACKSCORNERS_PROGRESS : public TrackNodeItems::TRACKS_PROGRESS
    {
    protected:
        ROUNDEDTRACKSCORNERS_PROGRESS(const ROUNDEDTRACKSCORNERS* aParent, const DLIST<TRACK>* aTracks, PICKED_ITEMS_LIST* aUndoRedoList) :
            TRACKS_PROGRESS(aParent->GetEditFrame(), aTracks, aUndoRedoList) {
                m_parent = const_cast<ROUNDEDTRACKSCORNERS*>(aParent);
            }
        
        ROUNDEDTRACKSCORNERS* m_parent{nullptr};
        
    };

    class ROUNDEDTRACKSCORNERS_PROGRESS_ADD_CORNERS : public ROUNDEDTRACKSCORNERS_PROGRESS
    {
    public:
        ROUNDEDTRACKSCORNERS_PROGRESS_ADD_CORNERS(const ROUNDEDTRACKSCORNERS* aParent, const DLIST<TRACK>* aTracks, PICKED_ITEMS_LIST* aUndoRedoList);
    protected:
        uint ExecuteItem(const BOARD_ITEM* aItemAt) override;
    };

    //Convert all TRACKs to ROUNDEDCORNERTRACKs.
    class ROUNDEDTRACKSCORNERS_PROGRESS_CONVERT_TRACKS : public ROUNDEDTRACKSCORNERS_PROGRESS
    {
    public:
        ROUNDEDTRACKSCORNERS_PROGRESS_CONVERT_TRACKS(const ROUNDEDTRACKSCORNERS* aParent, const DLIST<TRACK>* aTracks, PICKED_ITEMS_LIST* aUndoRedoList);
    protected:
        uint ExecuteItem(const BOARD_ITEM* aItemAt) override;
    };

    class ROUNDEDTRACKSCORNERS_PROGRESS_REMOVE_CORNERS : virtual public ROUNDEDTRACKSCORNERS_PROGRESS
    {
    public:
        ROUNDEDTRACKSCORNERS_PROGRESS_REMOVE_CORNERS(const ROUNDEDTRACKSCORNERS* aParent, const DLIST<TRACK>* aTracks, PICKED_ITEMS_LIST* aUndoRedoList);
        ~ROUNDEDTRACKSCORNERS_PROGRESS_REMOVE_CORNERS();

    protected:
        uint ExecuteItem(const BOARD_ITEM* aItemAt) override;
        void ExecuteEnd(void) override;
        RoundedTracksCorner_Container* m_remove_corners {nullptr};

    private:
        DLIST<TRACK>* m_tracks {nullptr};
    };

//-----------------------------------------------------------------------------------------------------/
//Clean 
//-----------------------------------------------------------------------------------------------------/
public:
    bool Clean(const DLIST<TRACK>* aTracksAt, BOARD_COMMIT& aCommit);
    
private:
    class ROUNDEDTRACKSCORNERS_PROGRESS_CLEAN : public ROUNDEDTRACKSCORNERS_PROGRESS_REMOVE_CORNERS
    {
    public:
        ROUNDEDTRACKSCORNERS_PROGRESS_CLEAN(const ROUNDEDTRACKSCORNERS* aParent, const DLIST<TRACK>* aTracks, PICKED_ITEMS_LIST* aUndoRedoList);
    protected:
        uint ExecuteItem(const BOARD_ITEM* aItemAt) override;
    };
//-----------------------------------------------------------------------------------------------------/

};

#endif //ROUNDEDTRACKSCORNERS_H
