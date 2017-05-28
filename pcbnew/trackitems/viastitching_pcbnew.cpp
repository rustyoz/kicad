/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2016 Heikki Pulkkinen.
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

#include "viastitching.h"

using namespace ViaStitching;


//Legacy canvas add.
void VIASTITCHING::AddThermalVia( const PCB_EDIT_FRAME* aEditFrame, const int aViaType_ID ) 
{
    VIATYPE_T viatype = VIA_THROUGH;
    if( ( aViaType_ID == ID_POPUP_PCB_PLACE_ZONE_BLIND_BURIED_VIA ) || 
        ( aViaType_ID ==  ID_POPUP_PCB_SEL_LAYERS_AND_PLACE_ZONE_BLIND_BURIED_VIA ) )
        viatype = VIA_BLIND_BURIED;

    bool select_layer = ( aViaType_ID == ID_POPUP_PCB_SEL_LAYERS_AND_PLACE_ZONE_BLIND_BURIED_VIA ) || 
                        ( aViaType_ID == ID_POPUP_PCB_SEL_LAYER_AND_PLACE_ZONE_THROUGH_VIA );

    AddThermalVia( aEditFrame, viatype, select_layer );
    const_cast<PCB_EDIT_FRAME*>(aEditFrame)->GetCanvas()->Refresh( true );
}

//Gal canvas add.
void VIASTITCHING::AddThermalVia( const PCB_EDIT_FRAME* aEditFrame, const VIATYPE_T aViaType, const bool aSelectLayer )
{
    if( aEditFrame->GetToolId() == ID_TRACK_BUTT )
    {
        if( ( aViaType == VIA_BLIND_BURIED ) && !aEditFrame->GetBoard()->GetDesignSettings().m_BlindBuriedViaAllowed )
            return;

        wxPoint pos = aEditFrame->GetCrossHairPosition();
        BOARD* board = aEditFrame->GetBoard();

        if( aSelectLayer )
        {
            if( aViaType == VIA_THROUGH )
            {
                if( !SelectLayer( const_cast<PCB_EDIT_FRAME*>(aEditFrame), pos ) )
                    return;
            }
            else
                if( aViaType == VIA_BLIND_BURIED )
                {
                    if( !SelectLayerPair( const_cast<PCB_EDIT_FRAME*>(aEditFrame), pos ) )
                        return;
                }
                else
                    return;
        }

        PCB_LAYER_ID currentLayer = aEditFrame->GetActiveLayer();
        PCB_LAYER_ID pairTop = aEditFrame->GetScreen()->m_Route_Layer_TOP;
        PCB_LAYER_ID pairBottom = aEditFrame->GetScreen()->m_Route_Layer_BOTTOM;

        if( ( aViaType == VIA_BLIND_BURIED ) && ( pairTop == pairBottom ) )
            return;

        if( ( currentLayer != pairTop ) && ( currentLayer != pairBottom ) )
        {
            const_cast<PCB_EDIT_FRAME*>(aEditFrame)->SetActiveLayer( ToLAYER_ID( pairTop ) );
        }

        PCB_LAYER_ID layer = aEditFrame->GetActiveLayer();
        //wxPoint pos = wxPoint( start_snap_point.x, start_snap_point.y );
        ZONE_CONTAINER* zone = board->HitTestForAnyFilledArea( pos, layer, layer, -1 );
        if( zone )
        {
            int netcode = zone->GetNetCode();

            BOARD_DESIGN_SETTINGS& bds = board->GetDesignSettings();
            VIA* via = new VIA( const_cast<BOARD*>(board) );
            via->SetFlags( IS_NEW );
            via->SetWidth( bds.GetCurrentViaSize());
            via->SetDrill( bds.GetCurrentViaDrill() );

            via->SetViaType( aViaType );
            if( aViaType == VIA_BLIND_BURIED && ( ( currentLayer == B_Cu ) || ( currentLayer == F_Cu ) )
                                            && ( ( pairTop == B_Cu && pairBottom == F_Cu )
                                            || ( pairTop == F_Cu && pairBottom == B_Cu ) ) )
                via->SetViaType( VIA_THROUGH );

            via->SetLayerPair( pairTop, pairBottom );
            if( aViaType == VIA_BLIND_BURIED )
            {
                if( currentLayer == pairTop || currentLayer == pairBottom )
                    via->SetLayerPair( pairTop, pairBottom );
                else
                    via->SetLayerPair( pairTop, currentLayer );
            }

            via->SetNetCode( netcode );
            via->SetPosition( pos );
            const_cast<VIA*>(dynamic_cast<const VIA*>(via))->SetThermalCode( netcode );
            if( !DestroyConflictingThermalVia( via, const_cast<PCB_EDIT_FRAME*>(aEditFrame) ) )
            {
                BOARD_COMMIT commit( const_cast<PCB_EDIT_FRAME*>(aEditFrame) );
                commit.Add( via );
                commit.Push( _( "Add Thermal Via" ) );
            }
        }
    }
}

void VIASTITCHING::AddViaArrayPrepare( const PCB_EDIT_FRAME* aEditFrame, const VIA* aVia )
{
    m_via_array_netcode_pads.clear();
    m_via_array_netcode = aVia->GetNetCode();
    if(g_Drc_On)
    {
        //Do not add pads in same netcode
        CONNECTIONS connections( const_cast<BOARD*>(aEditFrame->GetBoard()) );
        connections.BuildPadsList(m_via_array_netcode);
        m_via_array_netcode_pads = connections.GetPadsList();
        for(auto pad : m_via_array_netcode_pads)
            pad->SetNetCode(0); //Mark them diff netcode
    }
}

void VIASTITCHING::AddViaArrayFinnish( void )
{
    for(auto pad : m_via_array_netcode_pads)
        pad->SetNetCode(m_via_array_netcode); //Netcode back to right value.
}

bool VIASTITCHING::IsTrackConnection( const VIA* aVia, const int aNetcode )
{
    TRACK* start_track = const_cast<VIA*>(aVia);
    if( start_track )
    {
        for( int n = 0; n < 2; ++n )
        {
            TRACK* track_seg = start_track;
            if( n ) 
                track_seg = start_track->Back();

            while( track_seg )
            {
                if( track_seg->Type() == PCB_TRACE_T )
                {
                    if( track_seg->GetNetCode() == aNetcode )
                    {
                        if( aVia->IsOnLayer( track_seg->GetLayer() ) )
                        {
                            //Test whole segment length
                            //if( track_seg->HitTest( aVia->GetEnd() ) ) 
                            //Test only endpoints (connection points).
                            if((track_seg->GetStart() == aVia->GetEnd()) || (track_seg->GetEnd() == aVia->GetEnd()))
                            {
                                return true;
                            }
                        }
                    }
                    else
                        track_seg = nullptr;
                }
                if( track_seg )
                    (n)? track_seg = track_seg->Back() : track_seg = track_seg->Next();
            }
        }
    }
    return false;
}

void VIASTITCHING::SetNetcodes( const std::unordered_map<const VIA*, int>& aVias )
{
    for( auto& via_n : aVias )
    {
        const VIA* via = via_n.first;
        int netcode_was = via_n.second;

        int thermalcode = dynamic_cast<const VIA*>( via) ->GetThermalCode();
        int netcode = via->GetNetCode();

        if( thermalcode )
        {
            // Remove thermalcode reconnected thermalvia.
            if( netcode && ( netcode_was == netcode ) && ( thermalcode == netcode ) && (IsTrackConnection( via, thermalcode ) ) )// || ( via->m_TracksConnected.size() ) )
                const_cast<VIA*>( via )->SetThermalCode( 0 );
            // Set netcode to thermalvia.
            else
                const_cast<VIA*>( via )->SetNetCode( thermalcode );
        }
        else
            // Set thermalcode unconnected via inside zone.
            if( ( netcode_was && !netcode ) || 
                ( netcode_was && netcode && ( netcode_was == netcode ) && !via->m_TracksConnected.size() )  )
            {
                std::vector<ZONE_CONTAINER*> zones;
                Collect_Zones_Hit_Via( zones, via, netcode_was );
                if( zones.size() )
                {
                    const_cast<VIA*>( via )->SetNetCode( netcode_was );
                    const_cast<VIA*>( via )->SetThermalCode( netcode_was );
                }
            }
    }
}


//Set zone connections to thermal vias.
void VIASTITCHING::SetNetcodes( void )
{
    std::unordered_map<const VIA*, int> collected_vias;
    collected_vias.clear();

    for( TRACK* t = m_board->m_Track;  t;  t = t->Next() )
    {
         const VIA* via = dynamic_cast<const VIA*>( t );
         if( via  && ( dynamic_cast<const VIA*>(via)->GetThermalCode() 
                       || ( t->GetNetCode() 
                            && !via->m_TracksConnected.size() 
                            && !dynamic_cast<const VIA*>(via)->GetThermalCode() ) ) )
            collected_vias.insert( std::pair<const VIA*, int> ( via, t->GetNetCode() ) );
    }
    SetNetcodes( collected_vias );
}

//Set real zone polygon connections to thermal vias.
void VIASTITCHING::ConnectThermalViasToZones( void )
{
    //Connect unconnected single vias in copper pours. 
    if( m_board->GetAreaCount() )
    {
        //Collect all vias that has no netcode and vias with thermal code.
        std::unordered_map<const VIA*, bool> thermal_vias_all; //Via map with flag that it has been examined.
        thermal_vias_all.clear();

        //Collect other vias own container to look up connectivity.
        std::vector<const VIA*> other_zone_vias;
        other_zone_vias.clear();

        for( TRACK* t = m_board->m_Track;  t;  t = t->Next() )
        {
            const VIA* via = dynamic_cast<const VIA*>( t );
            if( via )
            {
                int thermalcode = dynamic_cast<const VIA*>(via)->GetThermalCode();
                int netcode = via->GetNetCode();

                // Clear vias polyzone and zone containers.
                std::unordered_map<const SHAPE_POLY_SET::POLYGON*, ZONE_CONTAINER*> poly_zone;
                poly_zone.clear();

                if( thermalcode )
                {
                    netcode = NETINFO_LIST::UNCONNECTED;
                    const_cast<VIA*>( via )->SetNetCode( netcode );
                }
                
                std::vector<ZONE_CONTAINER*> zones;
                int zonecode = thermalcode? thermalcode : netcode;
                Collect_Zones_Hit_Via( zones, via, zonecode );
                if( ( zones.size() >= MIN_THERMALVIA_ZONES ) || ( zones.size() && netcode ) )
                {
                    wxPoint via_pos = via->GetEnd();
                    for( auto& zone : zones )
                    {
                        const SHAPE_POLY_SET& zone_polys = zone->GetFilledPolysList();
                        const SHAPE_POLY_SET::POLYGON* poly = zone_polys.GetPolygon( VECTOR2I( via_pos.x, via_pos.y ) );
                        if( poly )
                            poly_zone.insert( std::pair<const SHAPE_POLY_SET::POLYGON*, ZONE_CONTAINER*> ( poly, zone ) );
                    }
                    if( !netcode )
                        thermal_vias_all.insert( std::pair<const VIA*, int>( via, false ) );
                    else
                        // ... other vias own container.
                        if( poly_zone.size() )
                            other_zone_vias.push_back( via );
                }

                const_cast<VIA*>(via)->SetThermalPolysZones( poly_zone ); 
                const_cast<VIA*>(via)->SetThermalZones( zones );

            }
        }

        //Test all collected vias connectivity.
        for( auto& zone_via : thermal_vias_all )
        {
            const VIA* via_test = zone_via.first;
            bool via_tested = zone_via.second;
            int thermalcode = dynamic_cast<const VIA*>(via_test)->GetThermalCode();
            int netcode = via_test->GetNetCode();

            if( !netcode && !via_tested )
            {
                zone_via.second = true;

                std::unordered_multimap<const VIA*, const SHAPE_POLY_SET::POLYGON*> vias_polys;
                vias_polys.clear();

                std::unordered_map<const SHAPE_POLY_SET::POLYGON*, ZONE_CONTAINER*>* via_polyzone 
                    = const_cast<VIA*>(via_test)->GetThermalPolysZones();

                if( via_polyzone->size() >= MIN_THERMALVIA_ZONES )
                {
                    for( auto& polyzone : *via_polyzone )
                    {
                        const SHAPE_POLY_SET::POLYGON* poly = polyzone.first;
                        vias_polys.insert( std::pair<const VIA*, const SHAPE_POLY_SET::POLYGON*> ( via_test, poly ) );
                        //Collect all zones poly and vias that hits with via in same zone poly.
                        Collect_Vias_Zones_Chain( vias_polys, thermalcode, poly, thermal_vias_all );
                    }

                    //Test pad connectivity.
                    bool hit = false;

                    CONNECTIONS connections( const_cast<BOARD*>( m_board ) );
                    connections.BuildPadsList( thermalcode );
                    std::vector<D_PAD*> test_pads = connections.GetPadsList();

                    for( auto& poly: vias_polys )
                    {
                        const VIA* via =  poly.first;
                        const SHAPE_POLY_SET::POLYGON* via_poly = poly.second;

                        // Other vias have pad connectivity.
                        for( auto& other_via : other_zone_vias)
                        {
                            if( other_via->GetNetCode() == thermalcode )
                            {
                                std::unordered_map<const SHAPE_POLY_SET::POLYGON*, ZONE_CONTAINER*>* othervia_polyszones = const_cast<VIA*>(other_via)->GetThermalPolysZones();
                                for( auto& othervia_polyzone : *othervia_polyszones )
                                {
                                    if( othervia_polyzone.first == via_poly )
                                    {
                                        hit = true;
                                        break;
                                    }
                                }
                            }
                        }

                        if( !hit )
                        {
                            via_polyzone = const_cast<VIA*>(via)->GetThermalPolysZones();
                            ZONE_CONTAINER* zone = nullptr;
                            std::unordered_map<const SHAPE_POLY_SET::POLYGON*, ZONE_CONTAINER*>::const_iterator vpz = via_polyzone->find( via_poly );
                            if( vpz != via_polyzone->end() ) //always true, must be.
                                zone = vpz->second;

                            if( zone )
                            {
                                PCB_LAYER_ID zone_layer = zone->GetLayer();

                                // Tracks have pad connectivity.
                                TRACK* start_track = const_cast<VIA*>(via);
                                if( start_track )
                                {
                                    for( int n = 0; n < 2; ++n )
                                    {
                                        TRACK* track_seg = start_track;
                                        if( n ) 
                                            track_seg = start_track->Back();

                                        while( track_seg )
                                        {
                                            if( track_seg->Type() == PCB_TRACE_T )
                                            {
                                                if( track_seg->GetNetCode() == thermalcode )
                                                {
                                                    if( track_seg->IsOnLayer( zone_layer ) && via->IsOnLayer( zone_layer ) )
                                                    {
                                                        for( int m = 0; m < 2; ++m )
                                                        {
                                                            wxPoint seg_pos = m? track_seg->GetEnd() : track_seg->GetStart();
                                                            if( zone->HitTestInsideZone( seg_pos ) )
                                                            {
                                                                const SHAPE_POLY_SET& zone_polys = zone->GetFilledPolysList();
                                                                const SHAPE_POLY_SET::POLYGON* seg_poly = zone_polys.GetPolygon( VECTOR2I( seg_pos.x, seg_pos.y ) );

                                                                if( seg_poly == via_poly )
                                                                {
                                                                    hit = true;
                                                                    track_seg = nullptr;
                                                                    break;
                                                                }
                                                            }
                                                        }
                                                    }
                                                }
                                                else
                                                    track_seg = nullptr;
                                            }
                                            if( track_seg )
                                                (n)? track_seg = track_seg->Back() : track_seg = track_seg->Next();
                                        }
                                    }
                                }

                                // Pad connection if no tracks or via connectivity.
                                if( !hit )
                                {
                                    for( D_PAD* pad : test_pads )
                                    {
                                        wxPoint pad_pos = pad->GetPosition();

                                        if( via->IsOnLayer( zone_layer ) && pad->IsOnLayer( zone_layer ) && zone->HitTestInsideZone( pad_pos ) )
                                        {
                                            const SHAPE_POLY_SET& zone_polys = zone->GetFilledPolysList();
                                            const SHAPE_POLY_SET::POLYGON* pad_poly = zone_polys.GetPolygon( VECTOR2I( pad_pos.x, pad_pos.y ) );

                                            if( pad_poly == via_poly )
                                            {
                                                hit = true;
                                                break;
                                            }

                                        }
                                        if( hit )
                                            break;
                                    }
                                }
                            }
                        }
                        if( hit )
                            break;
                    }

                    // Set netcodes if connection with pad or track.
                    if( hit )
                        for( auto& via_poly: vias_polys )
                            const_cast<VIA*>(via_poly.first)->SetNetCode( thermalcode );
                }
            }
        }
    }
}

//Recursively collect all vias to aViasPolys container which hits aVia in aZonePoly.
void VIASTITCHING::Collect_Vias_Zones_Chain( std::unordered_multimap<const VIA*, const SHAPE_POLY_SET::POLYGON*>& aViasPolys,
                                             const int aThermalCode,
                                             const SHAPE_POLY_SET::POLYGON* aZonePoly,
                                             std::unordered_map<const VIA*, bool>& aThermalViasAll )
{
    for( auto& thermal_via : aThermalViasAll )
    {
        const VIA* via_in_poly = thermal_via.first;

        if( !thermal_via.second )
        {
            int via_in_poly_thermalcode = dynamic_cast<const VIA*>( via_in_poly )->GetThermalCode();

            if( via_in_poly_thermalcode == aThermalCode )
            {
                std::unordered_map<const SHAPE_POLY_SET::POLYGON*, ZONE_CONTAINER*>* via_polyzone 
                    = const_cast<VIA*>( via_in_poly )->GetThermalPolysZones();
                std::unordered_map<const SHAPE_POLY_SET::POLYGON*, ZONE_CONTAINER*>::const_iterator vpz 
                    = via_polyzone->find ( aZonePoly );

                if( vpz != via_polyzone->end() )
                {
                    thermal_via.second = true;

                    if( via_polyzone->size() >= MIN_THERMALVIA_ZONES )
                    {
                        for( auto& polyzone : *via_polyzone )
                        {
                            const SHAPE_POLY_SET::POLYGON* poly = polyzone.first;
                            aViasPolys.insert( std::pair<const VIA*, const SHAPE_POLY_SET::POLYGON*>( via_in_poly, poly ) );
                            Collect_Vias_Zones_Chain( aViasPolys, aThermalCode, poly, aThermalViasAll);
                        }

                    }
                }
            }
        }

    }
}

//Collect all zones to aZone container, which hits aVia pos inside layer pair in same netcode.
void VIASTITCHING::Collect_Zones_Hit_Via( std::vector<ZONE_CONTAINER*>& aZones,
                                          const VIA* aVia,
                                          const int aNetCode,
                                          const LAYER_NUM aLayerOnly )
{
    PCB_LAYER_ID via_top_layer, via_bottom_layer;
    aVia->LayerPair( &via_top_layer, &via_bottom_layer );
    wxPoint via_pos = aVia->GetEnd();
    Collect_Zones_Hit_Pos( aZones, via_pos, via_top_layer, via_bottom_layer, aNetCode, aLayerOnly );
}

void VIASTITCHING::Collect_Zones_Hit_Pos( std::vector<ZONE_CONTAINER*>& aZones,
                                          wxPoint aPos,
                                          PCB_LAYER_ID aTopLayer,
                                          PCB_LAYER_ID aBottomLayer,
                                          const int aNetCode,
                                          const LAYER_NUM aLayerOnly )
{
    int num_areas = m_board->GetAreaCount();
    aZones.clear();

    for( int area_index = 0; area_index < num_areas; area_index++ )
    {
        ZONE_CONTAINER* area  = m_board->GetArea( area_index );
        if(area)
        {
            LAYER_NUM area_layer = area->GetLayer();
            if( ( aLayerOnly < F_Cu ) || ( aLayerOnly == area_layer ) )
                if( (area_layer >= aTopLayer) && (area_layer <= aBottomLayer) )
                {
                    if( area->HitTestInsideZone( aPos ) && ( area->GetNetCode() == aNetCode ) )
                    {
                        aZones.push_back( area );
                    }
                }
        }
    }
}

void VIASTITCHING::RuleCheck( const TRACK* aTrack, DRC* aDRC )
{
    if( dynamic_cast<const VIA*>( aTrack ) )
    {
        if( dynamic_cast<const VIA*>( aTrack )->GetThermalCode() )
        {
            std::vector<const SHAPE_POLY_SET::POLYGON*> via_zonepoly;
            via_zonepoly.clear();
            std::vector<ZONE_CONTAINER*>* zones = const_cast<VIA*>(dynamic_cast<const VIA*>( aTrack ))->GetThermalZones();;
            wxPoint via_pos = aTrack->GetEnd();
            for( auto& zone : *zones )
            {
                const SHAPE_POLY_SET& zone_polys = zone->GetFilledPolysList();
                const SHAPE_POLY_SET::POLYGON* poly = zone_polys.GetPolygon( VECTOR2I( via_pos.x, via_pos.y ) );
                if( poly )
                    via_zonepoly.push_back( poly );
            }

            //Warning Do not have at least two pours connected.
            //Locking via disables warning. Is that good?
            if( ( via_zonepoly.size() < 2 ) && !aTrack->IsLocked() ) 
            {
                aDRC->AddMarker( aTrack, aTrack->GetEnd(), DRCE_THERMAL_VIA_CONNECTED_POURS, nullptr );
            }

            //Unconnected thermal via.
            if( !via_zonepoly.size() )
            {
                aDRC->AddMarker( aTrack, aTrack->GetEnd(), DRCE_THERMAL_VIA_UNCONNECTED, nullptr );
            }

        }
    }   
}


bool VIASTITCHING::CleanThermalVias( PCB_EDIT_FRAME* aEditFrame, BOARD_COMMIT* aCommit )
{
    BOARD* board = aEditFrame->GetBoard();
    std::set<TRACK*> vias_remove;
    vias_remove.clear();
    bool modified = false;

    for( TRACK* t = board->m_Track;  t;  t = t->Next() )
    {
        const VIA* thermal_via = dynamic_cast<const VIA*>( t );
        if( thermal_via )
        {
            int thermalcode = dynamic_cast<const VIA*>(thermal_via)->GetThermalCode();
            if( thermalcode )
            {
                wxPoint via_pos = thermal_via->GetEnd();
                LSET via_layerset = thermal_via->GetLayerSet();
                int netcode = 0;

                if( thermalcode )
                {
                    BOARD_CONNECTED_ITEM* lock_point_item = const_cast<BOARD*>(board)->GetLockPoint( via_pos, via_layerset );
                    if( lock_point_item )
                    {
                        if( ( lock_point_item->Type() == PCB_PAD_T ) && ( lock_point_item->GetNetCode() != thermalcode ) )
                            netcode = lock_point_item->GetNetCode();
                    }
                    if( !netcode )
                    {
                        TRACK* start_track = board->m_Track;
                        TRACK* track = ViaBreakTrack( start_track, thermal_via );
                        bool test_again = true;
                        do
                        {
                            if( track ) 
                            {
                                //Do not have to remove when thermal via in track with same netcode.
                                test_again = track->GetNetCode() == thermalcode;
                                //But, if it is via.
                                if( track->Type() == PCB_VIA_T )
                                {
                                    /*
                                    //Remove thermal, if it conflicts with normal via with diff netcode.
                                    int test_via_thermalcode = dynamic_cast<VIA*>(track)->GetThermalCode();
                                    if( !test_via_thermalcode && ( track->GetNetCode() != thermalcode ) )
                                        */
                                    //Remove thermal, if it conflicts with normal via with same netcode.
                                    int test_via_thermalcode = dynamic_cast<VIA*>(track)->GetThermalCode();
                                    if( !test_via_thermalcode && ( track->GetNetCode() == thermalcode ) )
                                        test_again = false;

                                    //Remove if both are same thermalcode thermals.
                                    if( test_via_thermalcode == thermalcode )
                                    {
                                        //But not, if other is removed already.
                                        std::set<TRACK*>::iterator it = vias_remove.find( track );
                                        if( it == vias_remove.end() )
                                            test_again = false;
                                    }
                                    else //Both are thermal and diff netcode. Do not Remove.
                                        if( test_via_thermalcode && ( test_via_thermalcode != thermalcode ) )
                                            test_again = true;
                                }

                                if( test_again )
                                {
                                    start_track = track->Next();
                                    if( start_track )
                                        track = ViaBreakTrack( start_track, thermal_via );
                                    else
                                    {
                                        track = nullptr;
                                        test_again = false;
                                    }
                                }
                            }
                            else
                                test_again = false;
                        }
                        while( test_again );

                        if( track )
                            netcode = track->GetNetCode();
                    }

                    if( netcode )
                    {
                        vias_remove.insert( const_cast<VIA*>( thermal_via ) );
                    }
                }
            }
        }
    }
    
    for( auto via_remove : vias_remove )
    {
        board->Remove( via_remove );
        aCommit->Removed( via_remove );
        modified = true;
    }
    
    return modified;
}

TRACK* VIASTITCHING::ViaBreakTrack( const TRACK* aStartingTrack,
                                    const VIA* aVia )
{
    PCB_LAYER_ID via_top_layer, via_bottom_layer;
    aVia->LayerPair( &via_top_layer, &via_bottom_layer );
    wxPoint via_pos = aVia->GetEnd();

    for( const TRACK* track = aStartingTrack; track; track = track->Next() )
    {
        if( track != aVia )
        {
            if( ( track->Type() == PCB_VIA_T ) )
            {
                if( track->IsOnLayer( via_top_layer ) || track->IsOnLayer( via_bottom_layer ) )
                {
                    if( ( aVia->GetThermalCode() == dynamic_cast<const VIA*>(track)->GetThermalCode() ) )
                    {
                        if( track->HitTest( via_pos ) )
                            return const_cast<TRACK*>( track );
                    }
                    else
                        if( HitTestPoints( via_pos, track->GetEnd(),
                            aVia->GetWidth() / 2 + track->GetWidth() / 2 ) )
                            return const_cast<TRACK*>( track );
                }
            }
            else
            {
                if( aVia->IsOnLayer( track->GetLayer() ) )
                {
                    if( track->HitTest( via_pos ) )
                        return const_cast<TRACK*>( track );
                    else
                        //Track endpoints may connect with zone.
                        //Test endpoints with clearance. Do not want break connection to zone.
                        if( HitTestPoints( via_pos, track->GetStart(),
                            aVia->GetWidth() / 2 + track->GetWidth() / 2 + track->GetClearance() ) )
                            return const_cast<TRACK*>( track );
                        else
                            if( HitTestPoints( via_pos, track->GetEnd(),
                                aVia->GetWidth() / 2 + track->GetWidth() / 2 + track->GetClearance() ) )
                                return const_cast<TRACK*>( track );
                }

            }
        }
    }
    return nullptr;
}

//Do not add thermal vias in array add. if no zone in current layer. And use DRC if it is ON.
bool VIASTITCHING::DestroyConflictingThermalVia( BOARD_ITEM* aItem, PCB_BASE_FRAME* aFrame )
{
    if( aItem->Type() == PCB_VIA_T )
    {
        int thermalcode = dynamic_cast<const VIA*>(aItem)->GetThermalCode();
        if( thermalcode )
        {
            std::vector<ZONE_CONTAINER*> zones;
            Collect_Zones_Hit_Via( zones, static_cast<const VIA*>(aItem),
                                   dynamic_cast<const VIA*>(aItem)->GetNetCode(),
                                   aFrame->GetActiveLayer() );
            
            aItem->SetFlags( IS_NEW );
            bool hit_drc = false;
            if(g_Drc_On)
                hit_drc = dynamic_cast<PCB_EDIT_FRAME*>(aFrame)->GetDrcController()->Drc( static_cast<TRACK*>(aItem), m_board->m_Track );
                
            if( !zones.size() || hit_drc )
            {
                delete aItem;
                aItem = nullptr;
                return true;
            }
            aItem->ClearFlags( IS_NEW );
        }
    }
    return false;
}


bool VIASTITCHING::SelectLayer( PCB_EDIT_FRAME* aEditFrame, const wxPoint aPos )
{
    if( aEditFrame->GetBoard()->HitTestForAnyFilledArea( aPos, F_Cu, B_Cu, -1 ) )
    {
        THROUGH_VIA_LAYER_SELECTOR layer_selector( aEditFrame, aPos );

        if( layer_selector.ShowModal() == wxID_OK )
        {
            PCB_LAYER_ID layer = ToLAYER_ID( layer_selector.GetLayerSelection() );
            aEditFrame->GetScreen()->m_Route_Layer_TOP = layer;
            aEditFrame->SetActiveLayer( layer );
            aEditFrame->GetCanvas()->MoveCursor(aPos);
            return true;
        }
    }
    return false;
}

ZONE_CONTAINER* ViaStitching::HitTestZone( const BOARD* aPcb, const wxPoint aPos, PCB_LAYER_ID aLayer )
{
    int num_areas = aPcb->GetAreaCount();
    for( int area_index = 0; area_index < num_areas; area_index++ )
    {
        ZONE_CONTAINER* area  = aPcb->GetArea( area_index );
        if(area->GetLayer() == aLayer)
            if( area->HitTestInsideZone( aPos ) )
                return area;
    }
    return nullptr;
}

THROUGH_VIA_LAYER_SELECTOR::THROUGH_VIA_LAYER_SELECTOR( PCB_EDIT_FRAME* aEditFrame, const wxPoint aPos ) :
    VS_LAYER_SELECTOR( aEditFrame->GetBoard() ),
    DIALOG_LAYER_SELECTION_BASE( static_cast<wxWindow*>(aEditFrame) )
{
    m_pos = aPos;
    m_frame = const_cast<PCB_EDIT_FRAME*>(aEditFrame);

    m_layerSelected = m_frame->GetScreen()->m_Route_Layer_TOP;
    m_notAllowedLayersMask = LSET::AllNonCuMask();
    buildList();
    Layout();
    GetSizer()->SetSizeHints( this );
    SetFocus();
    Center();
}

void THROUGH_VIA_LAYER_SELECTOR::buildList( void )
{
    m_leftGridLayers->SetColSize( COLOR_COLNUM, 20 );
    m_rightGridLayers->SetColSize( COLOR_COLNUM, 20 );
    int row = 0;
    
    for( LSEQ ui_seq = m_brd->GetEnabledLayers().UIOrder();  ui_seq;  ++ui_seq )
    {
        PCB_LAYER_ID  layerid = *ui_seq;

        if( IsCopperLayer( layerid ) && !m_notAllowedLayersMask[layerid] )
        {
            ZONE_CONTAINER* zone = m_frame->GetBoard()->HitTestForAnyFilledArea( m_pos, layerid, layerid, -1 );
            if( zone )
            {
                COLOR4D color = GetLayerColor( layerid );
                wxString layername = GetLayerName( layerid );
                wxString netname = zone->GetShortNetname();

                if( row )
                    m_leftGridLayers->AppendRows( 1 );

                m_leftGridLayers->SetCellBackgroundColour ( row, COLOR_COLNUM,
                                                            color.ToColour() );
                m_leftGridLayers->SetCellValue( row, LAYERNAME_COLNUM,
                                                layername + " : " + netname );

                if( m_layerSelected == layerid )
                {
                    m_leftGridLayers->SetCellValue( row, SELECT_COLNUM,
                                                    wxT("X") );
                    m_leftGridLayers->SetCellBackgroundColour ( row, SELECT_COLNUM,
                                                            color.ToColour() );
                    m_leftGridLayers->SetGridCursor( row, LAYERNAME_COLNUM );
                }

                m_layersIdLeftColumn.push_back( layerid );
                row++;
            }
        }
    }

    if( row <= 0 )
        m_leftGridLayers->Show( false );

    m_rightGridLayers->Show( false );
    m_leftGridLayers->AutoSizeColumn(LAYERNAME_COLNUM);
    m_rightGridLayers->AutoSizeColumn(LAYERNAME_COLNUM);
    m_leftGridLayers->AutoSizeColumn(SELECT_COLNUM);
    m_rightGridLayers->AutoSizeColumn(SELECT_COLNUM);
}


void THROUGH_VIA_LAYER_SELECTOR::OnLeftGridCellClick( wxGridEvent& event )
{
    m_layerSelected = m_layersIdLeftColumn[ event.GetRow() ];
    m_leftGridLayers->SetGridCursor( event.GetRow(), LAYERNAME_COLNUM );

    //Snapglue OnLeftButtonReleased event problem to block start routing.
    m_frame->GetCanvas()->IgnoreNextLeftButtonRelease();//In EDA_DRAW_PANEL
    EndModal( wxID_OK );
}

//This event does never happens. Connected in DIALOG_LAYER_SELECTION_BASE.
//Mabe other wx version or wxGrid removed in OnLeftGridCellClick at EndModal() before mouse released.
void THROUGH_VIA_LAYER_SELECTOR::OnLeftButtonReleased( wxMouseEvent& event )
{
    EndModal( wxID_OK );
}


bool VIASTITCHING::SelectLayerPair( PCB_EDIT_FRAME* aEditFrame, const wxPoint aPos )
{
    if( aEditFrame->GetBoard()->HitTestForAnyFilledArea( aPos, F_Cu, B_Cu, -1 ) )
    {
        BURIEDBLIND_VIA_LAYER_SELECTOR layers_selector( aEditFrame, aPos );

        if( layers_selector.ShowModal() == wxID_OK )
        {
            layers_selector.GetLayerPair( aEditFrame->GetScreen()->m_Route_Layer_TOP, 
                                        aEditFrame->GetScreen()->m_Route_Layer_BOTTOM );
            aEditFrame->SetActiveLayer( aEditFrame->GetScreen()->m_Route_Layer_TOP );
            aEditFrame->GetCanvas()->MoveCursor(aPos);
            return true;
        }
    }
    return false;
}


BURIEDBLIND_VIA_LAYER_SELECTOR::BURIEDBLIND_VIA_LAYER_SELECTOR( PCB_EDIT_FRAME* aEditFrame, const wxPoint aPos ) :
    VS_LAYER_SELECTOR( aEditFrame->GetBoard() ),
    DIALOG_COPPER_LAYER_PAIR_SELECTION_BASE( static_cast<wxWindow*>(aEditFrame) )
{
    m_pos = aPos;
    m_frame = const_cast<PCB_EDIT_FRAME*>(aEditFrame);

    m_frontLayer = m_frame->GetScreen()->m_Route_Layer_TOP;
    m_backLayer = m_frame->GetScreen()->m_Route_Layer_BOTTOM;
    m_leftRowSelected = 0;
    m_rightRowSelected = 0;
    
    m_staticTextTopLayer->SetLabel("From   Layer  :  Net");
    m_staticTextBottomLayer->SetLabel("To     Layer  :  Net");
    
    buildList();
    SetFocus();
    GetSizer()->SetSizeHints( this );
    Center();
}


void BURIEDBLIND_VIA_LAYER_SELECTOR::buildList()
{
    m_leftGridLayers->SetColSize( COLOR_COLNUM, 20 );
    m_rightGridLayers->SetColSize( COLOR_COLNUM, 20 );

    int left_row = 0;
    int right_row = 0;

    for( LSEQ ui_seq = m_brd->GetEnabledLayers().UIOrder();  ui_seq;  ++ui_seq )
    {
        PCB_LAYER_ID  layerid = *ui_seq;

        if( IsCopperLayer( layerid ) )
        {
            COLOR4D color = GetLayerColor( layerid );
            wxString layername = GetLayerName( layerid );
            wxString netname = "";

            ZONE_CONTAINER* zone = m_frame->GetBoard()->HitTestForAnyFilledArea( m_pos, layerid, layerid, -1 );
            if( zone )
            {
                netname = zone->GetShortNetname();

                if( left_row )
                    m_leftGridLayers->AppendRows( 1 );

                m_leftGridLayers->SetCellBackgroundColour( left_row, COLOR_COLNUM, color.ToColour() );
                m_leftGridLayers->SetCellValue( left_row, LAYERNAME_COLNUM, layername + "  :  " + netname );
                m_left_layersId.push_back( layerid );

                if( m_frontLayer == layerid )
                {
                    SetGridCursor( m_leftGridLayers, left_row, true );
                    m_leftRowSelected = left_row;
                }
                left_row++;
            }

            zone = HitTestZone( m_frame->GetBoard(), m_pos, layerid );
            if( zone || ( layerid == F_Cu ) || ( layerid == B_Cu) )
            {
                netname = "";
                if( zone )
                    netname = zone->GetShortNetname();

                if( right_row )
                    m_rightGridLayers->AppendRows( 1 );

                m_rightGridLayers->SetCellBackgroundColour ( right_row, COLOR_COLNUM, color.ToColour() );
                m_rightGridLayers->SetCellValue( right_row, LAYERNAME_COLNUM, layername + "  :  " + netname );
                m_right_layersId.push_back( layerid );

                if( m_backLayer == layerid )
                {
                    SetGridCursor( m_rightGridLayers, right_row, true );
                    m_rightRowSelected = right_row;
                }

                right_row++;
            }

        }
    }

    m_leftGridLayers->AutoSizeColumn(LAYERNAME_COLNUM);
    m_rightGridLayers->AutoSizeColumn(LAYERNAME_COLNUM);
    m_leftGridLayers->AutoSizeColumn(SELECT_COLNUM);
    m_rightGridLayers->AutoSizeColumn(SELECT_COLNUM);
}


void BURIEDBLIND_VIA_LAYER_SELECTOR::SetGridCursor( wxGrid* aGrid, int aRow,
                                                      bool aEnable )
{
    if( aEnable )
    {
        PCB_LAYER_ID  layerid;
        if( aGrid == m_leftGridLayers )
            layerid = m_left_layersId[aRow];
        else
            if( aGrid == m_rightGridLayers )
                layerid = m_right_layersId[aRow];
            else
                return;
        COLOR4D color = GetLayerColor( layerid );
        aGrid->SetCellValue( aRow, SELECT_COLNUM, wxT("X") );
        aGrid->SetCellBackgroundColour( aRow, SELECT_COLNUM, color.ToColour() );
        aGrid->SetGridCursor( aRow, LAYERNAME_COLNUM );
    }
    else
    {
        aGrid->SetCellValue( aRow, SELECT_COLNUM, wxEmptyString );
        aGrid->SetCellBackgroundColour( aRow, SELECT_COLNUM,
                                        aGrid->GetDefaultCellBackgroundColour() );
        aGrid->SetGridCursor( aRow, LAYERNAME_COLNUM );
    }
}


void BURIEDBLIND_VIA_LAYER_SELECTOR::OnLeftGridCellClick( wxGridEvent& event )
{
    int         row = event.GetRow();
    PCB_LAYER_ID    layer = m_left_layersId[row];

    if( m_frontLayer == layer )
        return;

    SetGridCursor( m_leftGridLayers, m_leftRowSelected, false );
    m_frontLayer = layer;
    m_leftRowSelected = row;
    SetGridCursor( m_leftGridLayers, m_leftRowSelected, true );
}


void BURIEDBLIND_VIA_LAYER_SELECTOR::OnRightGridCellClick( wxGridEvent& event )
{
    int         row = event.GetRow();
    PCB_LAYER_ID    layer = m_right_layersId[row];

    if( m_backLayer == layer )
        return;

    SetGridCursor( m_rightGridLayers, m_rightRowSelected, false );
    m_backLayer = layer;
    m_rightRowSelected = row;
    SetGridCursor( m_rightGridLayers, m_rightRowSelected, true );
}

void BURIEDBLIND_VIA_LAYER_SELECTOR::OnOkClick( wxCommandEvent& event )
{
    EndModal( wxID_OK );
}

void BURIEDBLIND_VIA_LAYER_SELECTOR::OnCancelClick( wxCommandEvent& event )
{
    EndModal( wxID_CANCEL );
}


