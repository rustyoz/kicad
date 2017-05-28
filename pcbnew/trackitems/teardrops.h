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
 * @file teardrops.h
 * @brief Definitions for teardrop using class.
 */

#ifndef TEARDROPS_H
#define TEARDROPS_H

//Interface of TEARDROP.

#include "teardrop.h"
#include "tracknodeitems.h"
#include "tracknodeitem.h"
#include "roundedtrackscorners.h"


//-----------------------------------------------------------------------------------------------------/
// Class TEARDROPS
//-----------------------------------------------------------------------------------------------------/
class TEARDROPS : public TrackNodeItems::TRACKNODEITEMS
{

public:

    static const wxString TXT_TEARDROPS;
    
    enum TEARDROPS_TYPE_TODO
    {
        ALL_TYPES_T = 1,
        ONLY_TEARDROPS_T,
        ONLY_TJUNCTIONS_T,
        ONLY_JUNCTIONS_T,
        JUNCTIONS_AND_TJUNCTIONS_T,
        ONLY_PAD_TEARDROPS_T
    };
    
    TEARDROPS(const TRACKITEMS* aParent, const BOARD* aBoard);
    ~TEARDROPS();

    // Add when edit new track
    TrackNodeItem::TEARDROP* Add(const TRACK* aTrackSegTo, const BOARD_CONNECTED_ITEM* aViaOrPadTo, DLIST<TRACK>*aTrackListAt);
    TrackNodeItem::TEARDROP* Add(const TRACK* aTrackSegTo, const BOARD_CONNECTED_ITEM* aViaOrPadTo, DLIST<TRACK>*aTrackListAt, const bool aNullTrackCheck);

    // Add when edit fixed tracks
    void Add(const D_PAD* aPadTo, PICKED_ITEMS_LIST* aUndoRedoList); //Add pads all tracks.
    void Add(const VIA* aViaTo, PICKED_ITEMS_LIST* aUndoRedoList); //Add vias all tracks.
    void Add(const BOARD_CONNECTED_ITEM* aViaOrPadTo); //Add vias or pads all track segments.

    TrackNodeItem::TEARDROP* Add(const TRACK* aTrackSegTo, const wxPoint& aCurPosAt); //Add track segmnts cur pos via or pad,
    // Add all tracks connected to via or pad where is no teardrop
    TrackNodeItem::TEARDROP* Add(const TRACK* aTrackSegTo, const BOARD_CONNECTED_ITEM* aViaOrPadTo, PICKED_ITEMS_LIST* aUndoRedoList, const wxPoint aPosition = wxPoint(0,0)); //Add vias or pads one track segment.

    void Add(const TRACK* aTrackSegTo); //Add segments vias or/and pads.
    void Add(const TRACK* aTrackSegTo, PICKED_ITEMS_LIST* aUndoRedoList);

    void Add(const MODULE* aModuleTo); //Add to one module.
    void Add(const int aNetCodeTo, const TEARDROPS::TEARDROPS_TYPE_TODO aTypeToDo); // Add one net, vias and pads.
    void Add(const DLIST<TRACK>* aTracksAt, const TEARDROPS::TEARDROPS_TYPE_TODO aTypeToDo);
    void Add(const DLIST<MODULE>* aModulesTo); //Add all modules pads.
    //Junction & T-Junction
    static const bool TJUNCTION_TO_ADD = true;
    static const bool JUNCTION_TO_ADD = false;
    void Add(const TRACK* aTrackSegTo, const bool aTJunction, PICKED_ITEMS_LIST* aUndoRedoList); 

    //Remove teardrops when track(s) or via(s) are removed. Or remove only teardrop(s).
    void Remove(const BOARD_CONNECTED_ITEM* aItemFrom, const bool aUndo, const bool aLockedToo);
    void Remove(const BOARD_CONNECTED_ITEM* aItemFrom, PICKED_ITEMS_LIST* aUndoRedoList, const bool aLockedToo);
    void Remove(const BOARD_CONNECTED_ITEM* aItemFrom, BOARD_COMMIT& aCommit, const bool aLockedToo);
    void Remove(const TRACK* aTrackSegFrom, const BOARD_CONNECTED_ITEM* aViaOrPadFrom, PICKED_ITEMS_LIST* aUndoRedoList, const bool aLockedToo);

    void Remove(const MODULE* aModuleFrom, const bool aUndo, const bool aLockedToo);
    void Remove(const MODULE* aModuleFrom, PICKED_ITEMS_LIST* aUndoRedoList, const bool aLockedToo);
    void Remove(const int aNetCodeFrom, const TEARDROPS::TEARDROPS_TYPE_TODO aTypeToDo, const bool aUndo, const bool aLockedToo);
    void Remove(const int aNetCodeFrom, const TEARDROPS::TEARDROPS_TYPE_TODO aTypeToDo, PICKED_ITEMS_LIST* aUndoRedoList, const bool aLockedToo);
    
    void Remove(DLIST<TRACK>* aTracksAt, const TEARDROPS::TEARDROPS_TYPE_TODO aTypeToDo); //Remove from all vias
    void Remove(const DLIST<MODULE>* aModulesFrom); //Remove from all modules

    //Change existing teardrops or juctions to another values.
    void Change(const BOARD_CONNECTED_ITEM* aItemAt);
    void Change(const int aNetCodeAt, const TEARDROPS::TEARDROPS_TYPE_TODO aTypeToDo);
    void Change(const MODULE* aModuleAt);
    void Change(const DLIST<TRACK>* aTracksAt, const TEARDROPS::TEARDROPS_TYPE_TODO aTypeToDo);
    void Change(const DLIST<MODULE>* aModulesAt); //Add all modules pads.

    void Recreate(const int aNetCodeTo, PICKED_ITEMS_LIST* aUndoRedoList);
    void Recreate(const int aNetCodeTo, const bool aUndo);
    void Recreate(const MODULE* aModuleTo, PICKED_ITEMS_LIST* aUndoRedoList);
    void Recreate(const MODULE* aModuleTo, const bool aUndo);
    void Recreate(const D_PAD* aPadTo, PICKED_ITEMS_LIST* aUndoRedoList);
    void Recreate(const D_PAD* aPadTo, const bool aUndo);

    void Repopulate(const int aNetCodeAt, const TEARDROPS::TEARDROPS_TYPE_TODO aTypeToDo, const bool aUndo);
    void Repopulate(const int aNetCodeAt, const TEARDROPS::TEARDROPS_TYPE_TODO aTypeToDo, PICKED_ITEMS_LIST* aUndoRedoList);
    void Repopulate(DLIST<TRACK>* aTracksAll, const bool aUndo);

    //Calculate new values to teardrop(s)
    void Update(const BOARD_ITEM* aItemAt);
    void Update(const TRACK* aTrackSegAt);
    void Update(const D_PAD* aPadAt);
    void Update(const MODULE* aModuleAt);
    void Update(const VIA* atVia);
    void Update(const int aNetCodeAt, const TRACK* aTrackSeg);
    //New values and draw.
    void Update(TRACK* aTrackSegAt, EDA_DRAW_PANEL* aPanel, wxDC* aDC, GR_DRAWMODE aDrawMode, bool aErase);
    void Update(D_PAD* aPadAt, EDA_DRAW_PANEL* aPanel, wxDC* aDC, GR_DRAWMODE aDrawMode, bool aErase);
    void Update(MODULE* aModuleAt, EDA_DRAW_PANEL* aPanel, wxDC* aDC, GR_DRAWMODE aDrawMode, bool aErase);
    void Refresh(const TRACK* aTrackSegAt);

    //Is at least one empty place.
    bool Empty(const D_PAD* aPadTo) const;
    bool Empty(const VIA* aViaTo) const;
    bool Empty(const MODULE* aModuleTo) const;
    bool Empty(const int aNetCodeTo, const TRACK* aTrackSeg, const TEARDROPS::TEARDROPS_TYPE_TODO aTypeToDo) const;
    //Is at least one teardrop.
    int Contains(const D_PAD* aPadAt, int& aNumLocked) const;
    int Contains(const VIA* aViaAt, int& aNumLocked) const;
    int Contains(const MODULE* aModuleAt, int& aNumLocked) const;
    bool Contains(const int aNetCodeAt, const TRACK* aTrackSeg, const TEARDROPS::TEARDROPS_TYPE_TODO aTypeToDo) const;

    bool IsTrimmed(const TRACK* aTrackSeg) const;
    bool IsTrimmed(const TrackNodeItem::TEARDROP* aTear) const;

    void MarkWarnings(const DLIST<MODULE>* aModulesAt, DRC* aDRC);
    void MarkWarnings(const DLIST<TRACK>* aTracksAt, const TEARDROPS::TEARDROPS_TYPE_TODO aTypeToDo, DRC* aDRC);

    void MarkDifferent(const DLIST<TRACK>* aTracksAt, const TEARDROPS::TEARDROPS_TYPE_TODO aTypeToDo, DRC* aDRC);
    void MarkCurrent(const DLIST<TRACK>* aTracksAt, const TEARDROPS::TEARDROPS_TYPE_TODO aTypeToDo, DRC* aDRC);

    bool IsOn(void) const { return (bool)m_current_shape; }

    void LoadDefaultParams(const TrackNodeItem::TEARDROP::SHAPES_T aShape);
    TrackNodeItem::TEARDROP::SHAPES_T GetCurrentShape(void) const { return m_current_shape; }
    void SelectShape(const TrackNodeItem::TEARDROP::SHAPES_T aShape);  //Select current set teardrops OFF.
    TrackNodeItem::TEARDROP::PARAMS CopyCurrentParams(const TRACK* aTrackSegAt, const wxPoint& aCurPosAt);
    
    void LockToggle(const TRACK* aTrackSegAt, const wxPoint& aCurPosAt);
    void Lock(const DLIST<TRACK>* aTracksAt);
    void Unlock(const DLIST<TRACK>* aTracksAt);
    void Lock(const MODULE* aModuleAt);
    void Unlock(const MODULE* aModuleAt);
    void Lock(const BOARD_CONNECTED_ITEM* aViaOrPadAt);
    void Unlock(const BOARD_CONNECTED_ITEM* aViaOrPadAt);
    void Lock(const DLIST<TRACK>* aTracksAt, const TEARDROPS::TEARDROPS_TYPE_TODO aTypeToDo);
    void Unlock(const DLIST<TRACK>* aTracksAt, const TEARDROPS::TEARDROPS_TYPE_TODO aTypeToDo);
    void Lock(const DLIST<MODULE>* aModulesAt);
    void Unlock(const DLIST<MODULE>* aModulesAt);
    void Lock(const int aNetCodeAt, const TRACK* aTrackSeg);
    void Unlock(const int aNetCodeAt, const TRACK* aTrackSeg);
    void Lock(const TRACK* aTrackSegAt, const wxPoint& aCurPosAt, const DLIST<TRACK>* aTracksAt);
    void Unlock(const TRACK* aTrackSegAt, const wxPoint& aCurPosAt, const DLIST<TRACK>* aTracksAt);

    TrackNodeItem::TRACKNODEITEM* Next(const TRACK* aTrackSegAt) const override;
    TrackNodeItem::TRACKNODEITEM* Back(const TRACK* aTrackSegAt) const override;

    TrackNodeItem::TEARDROP* GetTeardrop(const TRACK* aTrackSegAt, const BOARD_CONNECTED_ITEM* atViaOrPad) const;
    
    void ChangePad(TrackNodeItem::TEARDROP* aTeardrop, const MODULE* aModuleAt);

    //Plot.
    void Plot(const TrackNodeItem::TRACKNODEITEM* aTrackNodeItem, PLOTTER* aPlotter, const EDA_DRAW_MODE_T* aPlotMode, void* aData ) override;

    //Save, Load
    void Format(OUTPUTFORMATTER* aOut, const int aNestLevel ) const throw( IO_ERROR ) override;
    TrackNodeItem::TRACKNODEITEM* Parse(PCB_PARSER* aParser) throw( IO_ERROR, PARSE_ERROR ) override;

    //Clean teardrops. Can not be undone. Remove only serious errors. "Never" happens.
    bool Clean(const DLIST<TRACK>* aTracksAt);

private:
    //Creation
    TrackNodeItem::TEARDROP* Create(const TRACK* aTrackSegTo, const BOARD_CONNECTED_ITEM* aViaOrPadTo, const bool aNullTrackCheck, const wxPoint aPosition = wxPoint(0,0));
    //Removing
    void Delete(TrackNodeItem::TEARDROP* aTeardrop, DLIST<TRACK>*aTrackListAt, PICKED_ITEMS_LIST* aUndoRedoList);

    //Private change funcs.
    void Change(TrackNodeItem::TEARDROP* aTeardrop, PICKED_ITEMS_LIST* aUndoRedoList);
    void Change(const VIA* aViaAt, PICKED_ITEMS_LIST* aUndoRedoList);
    void Change(const D_PAD* aPadAt, PICKED_ITEMS_LIST* aUndoRedoList);

    //Private remove funcs.
    static const bool CAN_RECREATE = true;
    void Remove(TrackNodeItem::TEARDROP* aTeardrop, PICKED_ITEMS_LIST* aUndoRedoList, const bool aSaveRemoved, const bool aLockedToo);
    void Remove(const VIA* aViaFrom, PICKED_ITEMS_LIST* aUndoRedoList, const bool aSaveRemoved, const bool aLockedToo);
    void Remove(const D_PAD* aPadFrom, PICKED_ITEMS_LIST* aUndoRedoList, const bool aSaveRemoved, const bool aLockedToo);

    void Lock(const D_PAD* aPadAt);
    void Unlock(const D_PAD* aPadAt);
    void Lock(const VIA* aViaAt);
    void Unlock(const VIA* aViaAt);

    bool IsSmallTeardrops(const D_PAD* aPadAt) const;
    inline bool IsConnected(const D_PAD* aPadAt, const TRACK* aTrackSegTo) const;

public:
    TrackNodeItem::TEARDROP::PARAMS GetShapeParams(const TrackNodeItem::TEARDROP::SHAPES_T aShape) const;
    TrackNodeItem::TEARDROP::PARAMS GetDefaultParams(const TrackNodeItem::TEARDROP::SHAPES_T aShape) const;
    wxString GetShapeName(const TrackNodeItem::TEARDROP::SHAPES_T aShape) const;
    void SetShapeParams(const TrackNodeItem::TEARDROP::PARAMS aShapeParams);
    void SetCurrentShape(const TrackNodeItem::TEARDROP::SHAPES_T aShape);
    
private:
    TrackNodeItem::TEARDROP::PARAMS SetParamLimits(const TrackNodeItem::TEARDROP::PARAMS aShapeParams);
    TrackNodeItem::TEARDROP::SHAPES_T m_current_shape;
    TrackNodeItem::TEARDROP::PARAMS m_teardrop_params;
    TrackNodeItem::TEARDROP::PARAMS m_fillet_params;
    TrackNodeItem::TEARDROP::PARAMS m_subland_params;
    TrackNodeItem::TEARDROP::PARAMS m_zero_params;
//-----------------------------------------------------------------------------------------------------/


//-----------------------------------------------------------------------------------------------------/
// One track segment teardrops, t-junctions and junctions memory.
//-----------------------------------------------------------------------------------------------------/
public:
    void ToMemory(const TRACK* aTrackSegFrom);
    void FromMemory(const TRACK* aTrackSegTo, PICKED_ITEMS_LIST* aUndoRedoList);
    void FromMemory(const TRACK* aTrackSegTo);
    void FromMemory(const TRACK* aTrackSegTo, BOARD_COMMIT& aCommit);

private:
    TrackNodeItem::TEARDROP* m_next_tear_in_memory;
    TrackNodeItem::TEARDROP* m_back_tear_in_memory;
//-----------------------------------------------------------------------------------------------------/

//-----------------------------------------------------------------------------------------------------/
// Track Route Edit teardrop (legacy canvas)
//-----------------------------------------------------------------------------------------------------/
public:
    enum TO_EDIT_T
    {
        EDIT_NULL_T = 0,
        EDIT_SIZE_WIDTH_T,
        EDIT_SIZE_LENGTH_T,
    };
    //Edit current teardrop params.
    void RouteCreate_Start(void);
    void RouteCreate_Stop(void);
    bool CanEdit(void) const { return m_can_edit;}
    void ToggleEdit(const TO_EDIT_T aEdit);
    bool IsEditOn(void) const { return (bool)m_to_edit; }
    void UpdateRouteEdit(EDA_DRAW_PANEL* aPanel, wxDC* aDC, const TRACK* aTrack, const uint aViaRad, const bool aErase, bool* aTrack45Only);

private:
    TrackNodeItem::TEARDROP_VIA* m_track_edit_tear;
    TO_EDIT_T m_to_edit;
    bool m_can_edit;
    TrackNodeItem::TEARDROP::PARAMS m_edit_params;
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
    void DRC_Rules(const TrackNodeItem::TEARDROP* aTeardrop, DRC* aDRC);

private:
    bool DRC_DoClearanceTest(const TrackNodeItem::TEARDROP* aTear, const wxPoint aTestPoint, const int aMinDist);
    bool DRC_DoClearanceTest(const TrackNodeItem::TEARDROP* aTear, const D_PAD* aPad, const int aMinDist, DRC* aDRC);
    bool DRC_ClearanceTest(const TrackNodeItem::TEARDROP* aTear, const VIA* aVia, const int aMinDist);
    bool DRC_ClearanceTest(const TrackNodeItem::TEARDROP* aTear, const TRACK* aTrackSeg, const int aMinDist);
    bool DRC_ClearanceTest(const TrackNodeItem::TEARDROP* aTear, TrackNodeItem::ROUNDEDTRACKSCORNER* aCorner, const int aMinDist);
    bool DRC_TestClearance(const TrackNodeItem::TEARDROP* aTear, const TRACK* aTrackSeg, const int aMinDist, DRC* aDRC);
//-----------------------------------------------------------------------------------------------------/

//-----------------------------------------------------------------------------------------------------/
// Teardrops todo lists.
//-----------------------------------------------------------------------------------------------------/
public:
    using Teardrop_Container = std::set<TrackNodeItem::TEARDROP*>;
    void UpdateListClear(void);
    void UpdateListAdd(const TRACK* aTrackSegFrom);
    void UpdateListAdd(const TrackNodeItem::TEARDROP* aTear);
    void UpdateListAdd(const ROUNDEDTRACKSCORNERS::RoundedCornerTrack_Container* aRoundedTracks);
    void UpdateListDo(void);
    void UpdateListDo(EDA_DRAW_PANEL* aPanel, wxDC* aDC, GR_DRAWMODE aDrawMode, bool aErase);
    void UpdateListDo_UndoRedo(void);
    void UpdateListDo_BlockDuplicate(const wxPoint aMoveVector, PICKED_ITEMS_LIST* aUndoRedoList);

private:
    Teardrop_Container* m_update_list;
    Teardrop_Container* m_recreate_list;
    void AddToDoList(const TRACK* aTrackSegFrom, Teardrop_Container* aListToAdd);
//-----------------------------------------------------------------------------------------------------/

//-----------------------------------------------------------------------------------------------------/
// COMMIT
//-----------------------------------------------------------------------------------------------------/
public:
    //Collect teardrops to commit.
    void CollectCommit(const TRACK* aTrackSegFrom, std::set<TRACK*>* aCommitContainer, const bool aLockedToo);
    void CollectCommit(TrackNodeItem::TEARDROP* aTeardrop, std::set<TRACK*>* aCommitContainer, const bool aLockedToo);
    void CollectCommit(const VIA* aViaFrom, std::set<TRACK*>* aCommitContainer, const bool aLockedToo);
    void CollectCommit(const D_PAD* aPadFrom, std::set<TRACK*>* aCommitContainer, const bool aLockedToo);
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
    void GalRemovedListAdd(const TrackNodeItem::TEARDROP* aTear);
    Teardrop_Container* m_gal_removed_list;
    int m_current_routed_track_netcode;

    VIA* m_gal_drag_via_dragged{nullptr};
    std::set<VIA*> m_gal_drag_vias_added;
    std::set<TrackNodeItem::TEARDROP*> m_gal_drag_tears_used;
    std::vector<TRACK*>m_gal_commit_tracks;

    TrackNodeItem::TEARDROP::SHAPES_T m_current_shape_gal;
    TrackNodeItem::TEARDROP::PARAMS m_teardrop_params_gal;
    TrackNodeItem::TEARDROP::PARAMS m_fillet_params_gal;
    TrackNodeItem::TEARDROP::PARAMS m_subland_params_gal;
//-----------------------------------------------------------------------------------------------------/

//-----------------------------------------------------------------------------------------------------/
// Menus
//-----------------------------------------------------------------------------------------------------/
public:
    void MenuToDo_ChangeSize(const int aMenuID);
    void Popup(wxMenu* aMenu, const TRACK* aTrackSeg, const wxPoint& aPos) const;
    void Popup(wxMenu* aMenu, const D_PAD* aPad) const;
    void Popup(wxMenu* aMenu, const MODULE* aModule) const;
private:
    static const wxString TXT_TEARDROP;
    static const wxString TXT_TJUNCTION;
    static const wxString TXT_JUNCTION;

    inline bool IsTeardropPlace(const TRACK* aTrackSeg, const wxPoint& aPos) const;
    wxString ParamsTxtToMenu(const TrackNodeItem::TEARDROP::PARAMS aParams) const;

    inline int MenuToDo_CalcSizeLength(const int aMenuID);
    inline int MenuToDo_CalcSizeWidth(const int aMenuID, const TrackNodeItem::TEARDROP::SHAPES_T aShape);
    inline int MenuToDo_CalcNumSegs(const int aMenuID, const TrackNodeItem::TEARDROP::SHAPES_T aShape);
    void Menu_ChangeSize(wxMenu* aMenu) const;

    void Menu_Select(wxMenu* aMenu) const;
    void Menu_TrackAdd(wxMenu* aMenu, const TRACK* aTrackSeg, const wxPoint& aPos) const;
    void Menu_TrackChange(wxMenu* aMenu, const TRACK* aTrackSeg, const wxPoint& aPos) const;
    void Menu_TrackRemove(wxMenu* aMenu, const TRACK* aTrackSeg, const wxPoint& aPos) const;
    bool Menu_NetAdd_Teardrops(wxMenu* aMenu, const TRACK* aTrackSeg, const wxPoint& aPos) const;
    bool Menu_NetChange_Teardrops(wxMenu* aMenu, const TRACK* aTrackSeg, const wxPoint& aPos) const;
    bool Menu_NetRemove_Teardrops(wxMenu* aMenu, const TRACK* aTrackSeg, const wxPoint& aPos) const;
    void Menu_NetAdd_TJunctions(wxMenu* aMenu, const TRACK* aTrackSeg) const;
    void Menu_NetChange_TJunctions(wxMenu* aMenu, const TRACK* aTrackSeg) const;
    void Menu_NetRemove_TJunctions(wxMenu* aMenu, const TRACK* aTrackSeg) const;
    void Menu_NetAdd_Junctions(wxMenu* aMenu, const TRACK* aTrackSeg) const;
    void Menu_NetChange_Junctions(wxMenu* aMenu, const TRACK* aTrackSeg) const;
    void Menu_NetRemove_Junctions(wxMenu* aMenu, const TRACK* aTrackSeg) const;
    void Menu_NetLock_Teardrops(wxMenu* aMenu, const TRACK* aTrackSeg, const wxPoint& aPos) const;
    void Menu_NetUnlock_Teardrops(wxMenu* aMenu, const TRACK* aTrackSeg, const wxPoint& aPos) const;
    void Menu_PadAdd(wxMenu* aMenu, const D_PAD* aPad) const;
    void Menu_PadChange(wxMenu* aMenu, const D_PAD* aPad) const;
    void Menu_PadRemove(wxMenu* aMenu, const D_PAD* aPad) const;
    void Menu_ModuleAdd(wxMenu* aMenu, const MODULE* aModule) const;
    void Menu_ModuleChange(wxMenu* aMenu, const MODULE* aModule) const;
    void Menu_ModuleRemove(wxMenu* aMenu, const MODULE* aModule) const;
    void Menu_SelectToDo(wxMenu* aMenu, const int aBaseToDo) const;
    void Menu_LockAllSameParam(wxMenu* aMenu, const TRACK* aTrackSeg, const wxPoint& aPos) const;

    void CreateMenu(wxMenu* aMenu) const override;
//-----------------------------------------------------------------------------------------------------/


//-----------------------------------------------------------------------------------------------------/
// Class private NETSCAN OPERATIONS 
//-----------------------------------------------------------------------------------------------------/
private:
    
    class NET_SCAN_GET_TEARDROP : public NET_SCAN_BASE
    {
    public:
        NET_SCAN_GET_TEARDROP(const TRACK* aTrackSeg, const BOARD_CONNECTED_ITEM* aViaOrPad, const TEARDROPS* aParent);
        ~NET_SCAN_GET_TEARDROP() {};

        TrackNodeItem::TEARDROP* GetResult(void) const {
            return m_result_teardrop;
        }

    protected:
        bool ExecuteAt(const TRACK* aTrackSeg) override;
        TrackNodeItem::TEARDROP* m_result_teardrop {nullptr};

    private:
        BOARD_CONNECTED_ITEM* m_via_or_pad {nullptr};
    };

    class NET_SCAN_GET_NEXT_TEARDROP : public NET_SCAN_GET_TEARDROP
    {
    public:
        NET_SCAN_GET_NEXT_TEARDROP(const TRACK* aTrackSeg, const TEARDROPS* aParent);
        ~NET_SCAN_GET_NEXT_TEARDROP() {};

    protected:
        bool ExecuteAt(const TRACK* aTrackSeg) override;
    };

    class NET_SCAN_GET_BACK_TEARDROP : public NET_SCAN_GET_TEARDROP
    {
    public:
        NET_SCAN_GET_BACK_TEARDROP(const TRACK* aTrackSeg, const TEARDROPS* aParent);
        ~NET_SCAN_GET_BACK_TEARDROP() {};

    protected:
        bool ExecuteAt(const TRACK* aTrackSeg) override;
    };

    class NET_SCAN_TRACK_UPDATE : public NET_SCAN_BASE
    {
    public:
        NET_SCAN_TRACK_UPDATE(const TRACK* aTrackSeg, const TEARDROPS* aParent);
        ~NET_SCAN_TRACK_UPDATE() {};

    protected:
        bool ExecuteAt(const TRACK* aTrackSeg) override;
    };

    class NET_SCAN_TRACK_DRAW_UPDATE : public NET_SCAN_BASE
    {
    public:
        NET_SCAN_TRACK_DRAW_UPDATE(const TRACK* aTrackSeg, const TEARDROPS* aParent, const EDA_DRAW_PANEL* aPanel, const wxDC* aDC, const GR_DRAWMODE aDrawMode, const bool aErase);
        ~NET_SCAN_TRACK_DRAW_UPDATE() {};

    protected:
        bool ExecuteAt(const TRACK* aTrackSeg) override;

    private:
        EDA_DRAW_PANEL* m_panel {nullptr};
        wxDC* m_dc {nullptr};
        GR_DRAWMODE m_draw_mode {GR_XOR};
        bool m_erase {true};
    };

    class NET_SCAN_MULTI_TEARDROPS : public NET_SCAN_BASE
    {
    public:
        NET_SCAN_MULTI_TEARDROPS(const TrackNodeItem::TEARDROP* aTeardrop, const TEARDROPS* aParent);
        ~NET_SCAN_MULTI_TEARDROPS() {};

        TrackNodeItem::TEARDROP* GetResult(void) const {
            return m_result_teardrop;
        }

    protected:
        bool ExecuteAt(const TRACK* aTrackSeg) override;

    private:
        TrackNodeItem::TEARDROP* m_result_teardrop {nullptr};
    };

//-----------------------------------------------------------------------------------------------------/
// Class private VIA OPERATIONS 
//-----------------------------------------------------------------------------------------------------/
    //Base class of via operations.
    class NET_SCAN_VIA_UPDATE : public NET_SCAN_BASE
    {
    public:
        NET_SCAN_VIA_UPDATE(const VIA* aVia, const TEARDROPS* aParent);
        ~NET_SCAN_VIA_UPDATE() {};

    protected:
        bool ExecuteAt(const TRACK* aTrackSeg) override;

        VIA* m_via {nullptr};
        wxPoint m_via_pos {0,0};
    };

    class NET_SCAN_VIA_ADD : public NET_SCAN_VIA_UPDATE
    {
    public:
        NET_SCAN_VIA_ADD(const VIA* aVia, const TEARDROPS* aParent, PICKED_ITEMS_LIST* aUndoRedoList);
        ~NET_SCAN_VIA_ADD() {};

    protected:
        bool ExecuteAt(const TRACK* aTrackSeg) override;

        PICKED_ITEMS_LIST* m_picked_items {nullptr};
    };

    class NET_SCAN_VIA_REMOVE : public NET_SCAN_VIA_ADD
    {
    public:
        NET_SCAN_VIA_REMOVE(const VIA* aVia, const TEARDROPS* aParent, PICKED_ITEMS_LIST* aUndoRedoList, Teardrop_Container* aRecreateList, const bool aLockedToo);
        ~NET_SCAN_VIA_REMOVE() {};

    protected:
        bool ExecuteAt(const TRACK* aTrackSeg) override;

    private:
        Teardrop_Container* m_recreate_list {nullptr};
        bool m_locked_too{false};
    };

    class NET_SCAN_VIA_EMPTY : public NET_SCAN_VIA_UPDATE
    {
    public:
        NET_SCAN_VIA_EMPTY(const VIA* aVia, const TEARDROPS* aParent);
        ~NET_SCAN_VIA_EMPTY() {};

        int NumTeardrops(void) const {
            return m_num_tears;
        }

    protected:
        bool ExecuteAt(const TRACK* aTrackSeg) override;

        int m_num_tears{0};
    };

    class NET_SCAN_VIA_CONTAINS : public NET_SCAN_VIA_EMPTY
    {
    public:
        NET_SCAN_VIA_CONTAINS(const VIA* aVia, const TEARDROPS* aParent);
        ~NET_SCAN_VIA_CONTAINS() {};

        int NumLocked(void) const { return m_num_locked; }

    protected:
        bool ExecuteAt(const TRACK* aTrackSeg) override;
        
    private:
        int m_num_locked{0};
    };

    class NET_SCAN_VIA_LOCK : public NET_SCAN_VIA_UPDATE
    {
    public:
        NET_SCAN_VIA_LOCK(const VIA* aVia, const TEARDROPS* aParent);
        ~NET_SCAN_VIA_LOCK() {};

    protected:
        bool ExecuteAt(const TRACK* aTrackSeg) override;
    };

    class NET_SCAN_VIA_UNLOCK : public NET_SCAN_VIA_UPDATE
    {
    public:
        NET_SCAN_VIA_UNLOCK(const VIA* aVia, const TEARDROPS* aParent);
        ~NET_SCAN_VIA_UNLOCK() {};

    protected:
        bool ExecuteAt(const TRACK* aTrackSeg) override;
    };

    class NET_SCAN_VIA_COLLECTCOMMIT : public NET_SCAN_VIA_UPDATE
    {
    public:
        NET_SCAN_VIA_COLLECTCOMMIT(const VIA* aVia, const TEARDROPS* aParent, std::set<TRACK*>* aCommitContainer, const bool aLockedToo);
        ~NET_SCAN_VIA_COLLECTCOMMIT() {};
        
    protected:
        bool ExecuteAt(const TRACK* aTrackSeg) override;

        bool m_locked_too{false};
        std::set<TRACK*>* m_commit_container{nullptr};
    };
    
//-----------------------------------------------------------------------------------------------------/
// Class private NET OPERATIONS
//-----------------------------------------------------------------------------------------------------/
    //Base class of net operations.
    class NET_SCAN_NET_UPDATE : public NET_SCAN_BASE
    {
    public:
        NET_SCAN_NET_UPDATE(const int aNet, const TRACK* aTrackSeg, const TEARDROPS* aParent);
        virtual ~NET_SCAN_NET_UPDATE() {};

    protected:
        virtual bool ExecuteAt(const TRACK* aTrackSeg) override;
    };

    class NET_SCAN_NET_ADD : public NET_SCAN_NET_UPDATE
    {
    public:
        NET_SCAN_NET_ADD(const int aNet, const TEARDROPS* aParent, PICKED_ITEMS_LIST* aUndoRedoList, const TEARDROPS::TEARDROPS_TYPE_TODO aTypeToDo);
        ~NET_SCAN_NET_ADD() {};

    protected:
        bool ExecuteAt(const TRACK* aTrackSeg) override;

        PICKED_ITEMS_LIST* m_picked_items {nullptr};
        TEARDROPS::TEARDROPS_TYPE_TODO m_type_todo;
    };

    class NET_SCAN_NET_REMOVE : public NET_SCAN_NET_ADD
    {
    public:
        NET_SCAN_NET_REMOVE(const int aNet, const TEARDROPS* aParent, PICKED_ITEMS_LIST* aUndoRedoList, const TEARDROPS::TEARDROPS_TYPE_TODO aTypeToDo, Teardrop_Container* aRecreateList, const bool aLockedToo);
        ~NET_SCAN_NET_REMOVE() {};

    protected:
        bool ExecuteAt(const TRACK* aTrackSeg) override;

        Teardrop_Container* m_recreate_list {nullptr};
        bool m_locked_too{false};
    };

    class NET_SCAN_NET_RECREATE : public NET_SCAN_NET_REMOVE
    {
    public:
        NET_SCAN_NET_RECREATE(const int aNet, const TEARDROPS* aParent, PICKED_ITEMS_LIST* aUndoRedoList, Teardrop_Container* aRecreateList);
        ~NET_SCAN_NET_RECREATE();

    protected:
        bool ExecuteAt(const TRACK* aTrackSeg) override;

    private:
        TrackNodeItem::TEARDROP::SHAPES_T m_current_shape;
        TrackNodeItem::TEARDROP::PARAMS m_teardrop_params;
        TrackNodeItem::TEARDROP::PARAMS m_fillet_params;
        TrackNodeItem::TEARDROP::PARAMS m_subland_params;
    };

    class NET_SCAN_NET_EMPTY : public NET_SCAN_NET_UPDATE
    {
    public:
        NET_SCAN_NET_EMPTY(const int aNet, const TRACK* aTrackSeg, const TEARDROPS* aParent, const TEARDROPS::TEARDROPS_TYPE_TODO aTypeToDo);
        ~NET_SCAN_NET_EMPTY() {};

        bool GetResult(void) const {
            return m_result_value;
        }

    protected:
        bool ExecuteAt(const TRACK* aTrackSeg) override;

        TEARDROPS::TEARDROPS_TYPE_TODO m_type_todo;
        bool m_result_value {false};

        static const uint COUNTS_MAX {
            100
        };
    };

    class NET_SCAN_NET_CONTAINS : public NET_SCAN_NET_EMPTY
    {
    public:
        NET_SCAN_NET_CONTAINS(const int aNet, const TRACK* aTrackSeg, const TEARDROPS* aParent, const TEARDROPS::TEARDROPS_TYPE_TODO aTypeToDo);
        ~NET_SCAN_NET_CONTAINS() {};

    protected:
        bool ExecuteAt(const TRACK* aTrackSeg) override;
    };

    class NET_SCAN_NET_LOCK : public NET_SCAN_NET_UPDATE
    {
    public:
        NET_SCAN_NET_LOCK(const int aNet, const TRACK* aTrackSeg, const TEARDROPS* aParent) :
            NET_SCAN_NET_UPDATE(aNet, aTrackSeg, aParent){};
        virtual ~NET_SCAN_NET_LOCK() {};

    protected:
        virtual bool ExecuteAt(const TRACK* aTrackSeg) override;
    };

    class NET_SCAN_NET_UNLOCK : public NET_SCAN_NET_UPDATE
    {
    public:
        NET_SCAN_NET_UNLOCK(const int aNet, const TRACK* aTrackSeg, const TEARDROPS* aParent) :
            NET_SCAN_NET_UPDATE(aNet, aTrackSeg, aParent){};
        virtual ~NET_SCAN_NET_UNLOCK() {};

    protected:
        virtual bool ExecuteAt(const TRACK* aTrackSeg) override;
    };


//-----------------------------------------------------------------------------------------------------/
// Class private PAD OPERATIONS
//-----------------------------------------------------------------------------------------------------/
    //Base class of pad operations.
    class NET_SCAN_PAD_BASE
    {
    public:
        NET_SCAN_PAD_BASE(const D_PAD* aPad, const TEARDROPS* aParent);
        virtual ~NET_SCAN_PAD_BASE() {};

        void Execute(void);

    protected:
        virtual bool ExecuteAt(const TRACK* aTrackSeg)=0;

        D_PAD* m_pad {nullptr};
        wxPoint m_pad_pos {0,0};
        TEARDROPS* m_Parent {nullptr};
        bool m_only_exact_connected = true;
    };

    class NET_SCAN_PAD_UPDATE : public NET_SCAN_PAD_BASE
    {
    public:
        NET_SCAN_PAD_UPDATE(const D_PAD* aPad, const TEARDROPS* aParent);
        ~NET_SCAN_PAD_UPDATE() {};

    protected:
        bool ExecuteAt(const TRACK* aTrackSeg) override;
    };
    
    class NET_SCAN_PAD_ADD : public NET_SCAN_PAD_BASE
    {
    public:
        NET_SCAN_PAD_ADD(const D_PAD* aPad, const TEARDROPS* aParent, PICKED_ITEMS_LIST* aUndoRedoList);
        ~NET_SCAN_PAD_ADD() {};

    protected:
        bool ExecuteAt(const TRACK* aTrackSeg) override;

        PICKED_ITEMS_LIST* m_picked_items {nullptr};
    };

    class NET_SCAN_PAD_REMOVE : public NET_SCAN_PAD_ADD
    {
    public:
        NET_SCAN_PAD_REMOVE(const D_PAD* aPad, const TEARDROPS* aParent, PICKED_ITEMS_LIST* aUndoRedoList, Teardrop_Container* aRecreateList, const bool aLockedToo);
        ~NET_SCAN_PAD_REMOVE() {};

    protected:
        bool ExecuteAt(const TRACK* aTrackSeg) override;

        Teardrop_Container* m_recreate_list {nullptr};
        bool m_locked_too{false};
    };

    class NET_SCAN_PAD_EMPTY : public NET_SCAN_PAD_BASE
    {
    public:
        NET_SCAN_PAD_EMPTY(const D_PAD* aPad, const TEARDROPS* aParent);
        ~NET_SCAN_PAD_EMPTY() {};

        int NumTeardrops(void) const { return m_num_tears; }

    protected:
        bool ExecuteAt(const TRACK* aTrackSeg) override;

        int m_num_tears{0};
    };

    class NET_SCAN_PAD_CONTAINS : public NET_SCAN_PAD_EMPTY
    {
    public:
        NET_SCAN_PAD_CONTAINS(const D_PAD* aPad, const TEARDROPS* aParent);
        ~NET_SCAN_PAD_CONTAINS() {};

        int NumLocked(void) const { return m_num_locked; }
    protected:
        bool ExecuteAt(const TRACK* aTrackSeg) override;

    private:
        int m_num_locked{0};
    };

    class NET_SCAN_PAD_RECREATE : public NET_SCAN_PAD_REMOVE
    {
    public:
        NET_SCAN_PAD_RECREATE(const D_PAD* aPad, const TEARDROPS* aParent, PICKED_ITEMS_LIST* aUndoRedoList, Teardrop_Container* aRecreateList);
        ~NET_SCAN_PAD_RECREATE();

    protected:
        bool ExecuteAt(const TRACK* aTrackSeg) override;

    private:
        TrackNodeItem::TEARDROP::SHAPES_T m_current_shape;
        TrackNodeItem::TEARDROP::PARAMS m_teardrop_params;
        TrackNodeItem::TEARDROP::PARAMS m_fillet_params;
        TrackNodeItem::TEARDROP::PARAMS m_subland_params;
    };

    class NET_SCAN_PAD_SMALLTEARS : public NET_SCAN_PAD_EMPTY
    {
    public:
        NET_SCAN_PAD_SMALLTEARS(const D_PAD* aPad, const TEARDROPS* aParent);
        ~NET_SCAN_PAD_SMALLTEARS() {};

    protected:
        bool ExecuteAt(const TRACK* aTrackSeg) override;
    };

    class NET_SCAN_PAD_LOCK : public NET_SCAN_PAD_BASE
    {
    public:
        NET_SCAN_PAD_LOCK(const D_PAD* aPad, const TEARDROPS* aParent);
        ~NET_SCAN_PAD_LOCK() {};

    protected:
        bool ExecuteAt(const TRACK* aTrackSeg) override;
    };

    class NET_SCAN_PAD_UNLOCK : public NET_SCAN_PAD_BASE
    {
    public:
        NET_SCAN_PAD_UNLOCK(const D_PAD* aPad, const TEARDROPS* aParent);
        ~NET_SCAN_PAD_UNLOCK() {};

    protected:
        bool ExecuteAt(const TRACK* aTrackSeg) override;
    };

    class NET_SCAN_PAD_COLLECTCOMMIT : public NET_SCAN_PAD_BASE
    {
    public:
        NET_SCAN_PAD_COLLECTCOMMIT(const D_PAD* aPad, const TEARDROPS* aParent, std::set<TRACK*>* aCommitContainer, const bool aLockedToo);
        ~NET_SCAN_PAD_COLLECTCOMMIT() {};

    protected:
        bool ExecuteAt(const TRACK* aTrackSeg) override;

        bool m_locked_too{false};
        std::set<TRACK*>* m_commit_container{nullptr};
    };
    
//-----------------------------------------------------------------------------------------------------/
// PROGRESS 
//-----------------------------------------------------------------------------------------------------/
    class TEARDROPS_MODULES_PROGRESS : public TrackNodeItems::MODULES_PROGRESS
    {
    protected:
        TEARDROPS_MODULES_PROGRESS(const TEARDROPS* aParent, const DLIST<MODULE>* aModules, PICKED_ITEMS_LIST* aUndoRedoList) :
            MODULES_PROGRESS(aParent->GetEditFrame(), aModules, aUndoRedoList) {
                m_Parent = const_cast<TEARDROPS*>(aParent);
            }
        
        TEARDROPS* m_Parent{nullptr};
        
    };

//-----------------------------------------------------------------------------------------------------/
// Modules (PADs)
//-----------------------------------------------------------------------------------------------------/
    class MODULES_PROGRESS_ADD_TEARS : public TEARDROPS_MODULES_PROGRESS
    {
    public:
        MODULES_PROGRESS_ADD_TEARS(const TEARDROPS* aParent, const DLIST<MODULE>* aModules, PICKED_ITEMS_LIST* aUndoRedoList);
    protected:
        uint DoAtPad(const D_PAD* aPadAt) override;
    };

    class MODULES_PROGRESS_REMOVE_TEARS : public TEARDROPS_MODULES_PROGRESS
    {
    public:
        MODULES_PROGRESS_REMOVE_TEARS(const TEARDROPS* aParent, const DLIST<MODULE>* aModules, PICKED_ITEMS_LIST* aUndoRedoList);
    protected:
        uint DoAtPad(const D_PAD* aPadAt) override;
    };

    class MODULES_PROGRESS_CHANGE_TEARS : public MODULES_PROGRESS_REMOVE_TEARS
    {
    public:
        MODULES_PROGRESS_CHANGE_TEARS(const TEARDROPS* aParent, const DLIST<MODULE>* aModules, 
                                      const Teardrop_Container* aRemovedList, PICKED_ITEMS_LIST* aUndoRedoList);
        ~MODULES_PROGRESS_CHANGE_TEARS();
    protected:
        uint DoAtPad(const D_PAD* aPadAt) override;
        void ExecuteEnd(void) override;
    private:
        TEARDROPS::Teardrop_Container* m_removed_tears {nullptr};
        TEARDROPS::Teardrop_Container* m_tears_list {nullptr};
    };

    class MODULES_PROGRESS_MARK_WARNINGS : public TEARDROPS_MODULES_PROGRESS
    {
    public:
        MODULES_PROGRESS_MARK_WARNINGS(const TEARDROPS* aParent, const DLIST<MODULE>* aModules, DRC* aDRC);
        ~MODULES_PROGRESS_MARK_WARNINGS();
    protected:
        uint DoAtPad(const D_PAD* aPadAt) override;
        DRC* m_DRC = nullptr;
    };

    class MODULES_PROGRESS_LOCK_TEARS : public TEARDROPS_MODULES_PROGRESS
    {
    public:
        MODULES_PROGRESS_LOCK_TEARS(const TEARDROPS* aParent, const DLIST<MODULE>* aModules);
    protected:
        uint DoAtPad(const D_PAD* aPadAt) override;
    };

    class MODULES_PROGRESS_UNLOCK_TEARS : public TEARDROPS_MODULES_PROGRESS
    {
    public:
        MODULES_PROGRESS_UNLOCK_TEARS(const TEARDROPS* aParent, const DLIST<MODULE>* aModules);
    protected:
        uint DoAtPad(const D_PAD* aPadAt) override;
    };

//-----------------------------------------------------------------------------------------------------/
// TRACKS_PROGRESS 
//-----------------------------------------------------------------------------------------------------/
    class TEARDROPS_TRACKS_PROGRESS : public TrackNodeItems::TRACKS_PROGRESS
    {
    protected:
        TEARDROPS_TRACKS_PROGRESS(const TEARDROPS* aParent, const DLIST<TRACK>* aTracks, PICKED_ITEMS_LIST* aUndoRedoList) :
            TRACKS_PROGRESS(aParent->GetEditFrame(), aTracks, aUndoRedoList) {
                m_Parent = const_cast<TEARDROPS*>(aParent);
            }
        
        TEARDROPS* m_Parent{nullptr};
        
    };

    class TRACKS_PROGRESS_ADD_TEARS_VIAS : public TEARDROPS_TRACKS_PROGRESS
    {
    public:
        TRACKS_PROGRESS_ADD_TEARS_VIAS(const TEARDROPS* aParent, const DLIST<TRACK>* aTracks, PICKED_ITEMS_LIST* aUndoRedoList);
    protected:
        uint ExecuteItem(const BOARD_ITEM* aItemAt) override;
    };

    class TRACKS_PROGRESS_REMOVE_TEARS_VIAS : virtual public TEARDROPS_TRACKS_PROGRESS
    {
    public:
        TRACKS_PROGRESS_REMOVE_TEARS_VIAS(const TEARDROPS* aParent, const DLIST<TRACK>* aTracks, PICKED_ITEMS_LIST* aUndoRedoList);
        ~TRACKS_PROGRESS_REMOVE_TEARS_VIAS();

    protected:
        uint ExecuteItem(const BOARD_ITEM* aItemAt) override;
        void ExecuteEnd(void) override;
        TEARDROPS::Teardrop_Container* m_remove_tears {nullptr};

    private:
        DLIST<TRACK>* m_tracks {nullptr};
    };

    class TRACKS_PROGRESS_CHANGE_TEARS_VIAS : virtual public TRACKS_PROGRESS_REMOVE_TEARS_VIAS
    {
    public:
        TRACKS_PROGRESS_CHANGE_TEARS_VIAS(const TEARDROPS* aParent, const DLIST<TRACK>* aTracks, PICKED_ITEMS_LIST* aUndoRedoList);

    protected:
        void ExecuteEnd(void) override;
    };

    class TRACKS_PROGRESS_ADD_TJUNCTIONS : public TRACKS_PROGRESS_ADD_TEARS_VIAS
    {
    public:
        TRACKS_PROGRESS_ADD_TJUNCTIONS(const TEARDROPS* aParent, const DLIST<TRACK>* aTracks, PICKED_ITEMS_LIST* aUndoRedoList);
    protected:
        uint ExecuteItem(const BOARD_ITEM* aItemAt) override;
    };

    class TRACKS_PROGRESS_REMOVE_TJUNCTIONS : virtual public TRACKS_PROGRESS_REMOVE_TEARS_VIAS
    {
    public:
        TRACKS_PROGRESS_REMOVE_TJUNCTIONS(const TEARDROPS* aParent, const DLIST<TRACK>* aTracks, PICKED_ITEMS_LIST* aUndoRedoList);

    protected:
        uint ExecuteItem(const BOARD_ITEM* aItemAt) override;
    };

    class TRACKS_PROGRESS_CHANGE_TJUNCTIONS : public TRACKS_PROGRESS_CHANGE_TEARS_VIAS, public TRACKS_PROGRESS_REMOVE_TJUNCTIONS
    {
    public:
        TRACKS_PROGRESS_CHANGE_TJUNCTIONS(const TEARDROPS* aParent, const DLIST<TRACK>* aTracks, PICKED_ITEMS_LIST* aUndoRedoList);
    protected:
        uint ExecuteItem(const BOARD_ITEM* aItemAt) override;
        void ExecuteEnd(void) override;
    };

    class TRACKS_PROGRESS_ADD_JUNCTIONS : public TRACKS_PROGRESS_ADD_TJUNCTIONS
    {
    public:
        TRACKS_PROGRESS_ADD_JUNCTIONS(const TEARDROPS* aParent, const DLIST<TRACK>* aTracks, PICKED_ITEMS_LIST* aUndoRedoList);
    protected:
        uint ExecuteItem(const BOARD_ITEM* aItemAt) override;
    };

    class TRACKS_PROGRESS_REMOVE_JUNCTIONS : public TRACKS_PROGRESS_REMOVE_TJUNCTIONS
    {
    public:
        TRACKS_PROGRESS_REMOVE_JUNCTIONS(const TEARDROPS* aParent, const DLIST<TRACK>* aTracks, PICKED_ITEMS_LIST* aUndoRedoList);

    protected:
        uint ExecuteItem(const BOARD_ITEM* aItemAt) override;
    };

    class TRACKS_PROGRESS_CHANGE_JUNCTIONS : public TRACKS_PROGRESS_CHANGE_TEARS_VIAS, public TRACKS_PROGRESS_REMOVE_JUNCTIONS
    {
    public:
        TRACKS_PROGRESS_CHANGE_JUNCTIONS(const TEARDROPS* aParent, const DLIST<TRACK>* aTracks, PICKED_ITEMS_LIST* aUndoRedoList);
    protected:
        uint ExecuteItem(const BOARD_ITEM* aItemAt) override;
        void ExecuteEnd(void) override;
    };

    class TRACKS_PROGRESS_MARK_WARNINGS : public TEARDROPS_TRACKS_PROGRESS
    {
    public:
        TRACKS_PROGRESS_MARK_WARNINGS(const TEARDROPS* aParent, const DLIST<TRACK>* aTracks, const TEARDROPS::TEARDROPS_TYPE_TODO aTypeToDo, DRC* aDRC);
        ~TRACKS_PROGRESS_MARK_WARNINGS();
    protected:
        uint ExecuteItem(const BOARD_ITEM* aItemAt) override;
        DRC* m_DRC = nullptr;
        TEARDROPS::TEARDROPS_TYPE_TODO m_type_todo;
    };

    class TRACKS_PROGRESS_MARK_DIFF : public TRACKS_PROGRESS_MARK_WARNINGS
    {
    public:
        TRACKS_PROGRESS_MARK_DIFF(const TEARDROPS* aParent, const DLIST<TRACK>* aTracks, const TEARDROPS::TEARDROPS_TYPE_TODO aTypeToDo, DRC* aDRC);
        ~TRACKS_PROGRESS_MARK_DIFF();
        
    protected:
        uint ExecuteItem(const BOARD_ITEM* aItemAt) override;
        Teardrop_Container* m_collected_tears {nullptr};
    };

    class TRACKS_PROGRESS_MARK_CURR : public TRACKS_PROGRESS_MARK_WARNINGS
    {
    public:
        TRACKS_PROGRESS_MARK_CURR(const TEARDROPS* aParent, const DLIST<TRACK>* aTracks, const TEARDROPS::TEARDROPS_TYPE_TODO aTypeToDo, DRC* aDRC);
        
    protected:
        uint ExecuteItem(const BOARD_ITEM* aItemAt) override;
    };

    //Clean all teardrops and junctions.
    class TRACKS_PROGRESS_CLEAN : public TRACKS_PROGRESS_REMOVE_TEARS_VIAS
    {
    public:
        TRACKS_PROGRESS_CLEAN(const TEARDROPS* aParent, const DLIST<TRACK>* aTracks, PICKED_ITEMS_LIST* aUndoRedoList);
    protected:
        uint ExecuteItem(const BOARD_ITEM* aItemAt) override;
    };

    class TRACKS_PROGRESS_LOCK_SAME : public TEARDROPS_TRACKS_PROGRESS
    {
    public:
        TRACKS_PROGRESS_LOCK_SAME(const TEARDROPS* aParent, const DLIST<TRACK>* aTracks, const TrackNodeItem::TEARDROP::PARAMS aParams);
    protected:
        uint ExecuteItem(const BOARD_ITEM* aItemAt) override;
        TrackNodeItem::TEARDROP::PARAMS m_params{TrackNodeItem::TEARDROP::NULL_T,0,0,0};
    };

    class TRACKS_PROGRESS_UNLOCK_SAME : public TRACKS_PROGRESS_LOCK_SAME
    {
    public:
        TRACKS_PROGRESS_UNLOCK_SAME(const TEARDROPS* aParent, const DLIST<TRACK>* aTracks, const TrackNodeItem::TEARDROP::PARAMS aParams);
    protected:
        uint ExecuteItem(const BOARD_ITEM* aItemAt) override;
    };

    class TRACKS_PROGRESS_LOCK_TEARS_VIAS : public TEARDROPS_TRACKS_PROGRESS
    {
    public:
        TRACKS_PROGRESS_LOCK_TEARS_VIAS(const TEARDROPS* aParent, const DLIST<TRACK>* aTracks);
    protected:
        uint ExecuteItem(const BOARD_ITEM* aItemAt) override;
    };

    class TRACKS_PROGRESS_UNLOCK_TEARS_VIAS : public TEARDROPS_TRACKS_PROGRESS
    {
    public:
        TRACKS_PROGRESS_UNLOCK_TEARS_VIAS(const TEARDROPS* aParent, const DLIST<TRACK>* aTracks);
    protected:
        uint ExecuteItem(const BOARD_ITEM* aItemAt) override;
    };

    class TRACKS_PROGRESS_LOCK_TJUNCTIONS : public TRACKS_PROGRESS_LOCK_TEARS_VIAS
    {
    public:
        TRACKS_PROGRESS_LOCK_TJUNCTIONS(const TEARDROPS* aParent, const DLIST<TRACK>* aTracks);
    protected:
        uint ExecuteItem(const BOARD_ITEM* aItemAt) override;
    };

    class TRACKS_PROGRESS_UNLOCK_TJUNCTIONS : public TRACKS_PROGRESS_UNLOCK_TEARS_VIAS
    {
    public:
        TRACKS_PROGRESS_UNLOCK_TJUNCTIONS(const TEARDROPS* aParent, const DLIST<TRACK>* aTracks);
    protected:
        uint ExecuteItem(const BOARD_ITEM* aItemAt) override;
    };

    class TRACKS_PROGRESS_LOCK_JUNCTIONS : public TRACKS_PROGRESS_LOCK_TEARS_VIAS
    {
    public:
        TRACKS_PROGRESS_LOCK_JUNCTIONS(const TEARDROPS* aParent, const DLIST<TRACK>* aTracks);
    protected:
        uint ExecuteItem(const BOARD_ITEM* aItemAt) override;
    };

    class TRACKS_PROGRESS_UNLOCK_JUNCTIONS : public TRACKS_PROGRESS_UNLOCK_TEARS_VIAS
    {
    public:
        TRACKS_PROGRESS_UNLOCK_JUNCTIONS(const TEARDROPS* aParent, const DLIST<TRACK>* aTracks);
    protected:
        uint ExecuteItem(const BOARD_ITEM* aItemAt) override;
    };
}; //class TEARDROPS


#endif /* TEARDROPS_H */
