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
// Save
//-----------------------------------------------------------------------------------------------------/
void TEARDROPS::Format(OUTPUTFORMATTER* aOut, const int aNestLevel ) const throw( IO_ERROR )
{
    bool tears = false;
    TEARDROP::PARAMS prev_params = {TEARDROP::NULL_T, 0, 0, 0};
    for( TRACK* track_seg = m_Board->m_Track;  track_seg; track_seg = track_seg->Next() )
    {
        if(track_seg->Type() == PCB_TEARDROP_T)
        {
            TRACK* connected_track = dynamic_cast<TRACK*>(dynamic_cast<TEARDROP*>(track_seg)->GetTrackSeg());
            if(connected_track && (connected_track->Type() == PCB_TRACE_T))
            {
                TEARDROP::PARAMS param = dynamic_cast<TEARDROP*>(track_seg)->GetParams();
                if(param != prev_params)
                {
                    aOut->Print(aNestLevel, "(teardrop ");
                    aOut->Print(0, " (shape %d) (length_ratio %d) (width_ratio %d) (segments %d)", param.shape, param.length_ratio, param.width_ratio, param.num_segments);
                    aOut->Print( 0, " (parameters))\n" );
                    prev_params = param;
                }
                aOut->Print(aNestLevel, "(teardrop ");
                aOut->Print( 0, " (position %s) (net %d)", FMT_IU(track_seg->GetEnd() ).c_str(), track_seg->GetNetCode());
                aOut->Print( 0, " (layer %s)", aOut->Quotew(connected_track->GetLayerName()).c_str());
                aOut->Print(0, " (start %s) (end %s)", FMT_IU(connected_track->GetStart() ).c_str(),  FMT_IU(connected_track->GetEnd() ).c_str());
                if(dynamic_cast<TEARDROP*>(track_seg)->IsLocked())
                    aOut->Print(0, " (locked)");
                aOut->Print( 0, " (%s))\n", aOut->Quotew(dynamic_cast<TEARDROP*>(track_seg)->GetConnectedItemName().MakeLower()).c_str());
                tears = true;
            }
        }
    }
    //If no teardrops, do not save even teardrop parameters.
    if(tears)
    {
        //Save current last. When load, it is current back.
        TEARDROP::PARAMS cur_shape_params = GetShapeParams(GetCurrentShape());
        if(cur_shape_params.shape != TEARDROP::TEARDROP_T)
        {
            aOut->Print(aNestLevel, "(teardrop ");
            aOut->Print(0, " (shape %d) (length_ratio %d) (width_ratio %d) (segments %d)", m_teardrop_params.shape, m_teardrop_params.length_ratio, m_teardrop_params.width_ratio, m_teardrop_params.num_segments);
            aOut->Print( 0, " (parameters))\n" );
        }
        if(cur_shape_params.shape != TEARDROP::FILLET_T)
        {
            aOut->Print(aNestLevel, "(teardrop ");
            aOut->Print(0, " (shape %d) (length_ratio %d) (width_ratio %d) (segments %d)", m_fillet_params.shape, m_fillet_params.length_ratio, m_fillet_params.width_ratio, m_fillet_params.num_segments);
            aOut->Print( 0, " (parameters))\n" );
        }
        if(cur_shape_params.shape != TEARDROP::SUBLAND_T)
        {
            aOut->Print(aNestLevel, "(teardrop ");
            aOut->Print(0, " (shape %d) (length_ratio %d) (width_ratio %d) (segments %d)", m_subland_params.shape, m_subland_params.length_ratio, m_subland_params.width_ratio, m_subland_params.num_segments);
            aOut->Print( 0, " (parameters))\n" );
        }
        aOut->Print(aNestLevel, "(teardrop ");
        aOut->Print(0, " (shape %d) (length_ratio %d) (width_ratio %d) (segments %d)", cur_shape_params.shape, cur_shape_params.length_ratio, cur_shape_params.width_ratio, cur_shape_params.num_segments);
        aOut->Print( 0, " (parameters))\n" );
    }
}

//-----------------------------------------------------------------------------------------------------/
// Load
//-----------------------------------------------------------------------------------------------------/
TRACKNODEITEM* TEARDROPS::Parse(PCB_PARSER* aParser) throw( IO_ERROR, PARSE_ERROR )
{
    using namespace PCB_KEYS_T;
    wxCHECK_MSG( aParser->CurTok() == T_teardrop, nullptr,
                 wxT( "Cannot parse " ) + aParser->GetTokenString( aParser->CurTok() ) + wxT( " as Teardrop." ) );

    TEARDROP::PARAMS param = {TEARDROP::NULL_T, 0, 0, 0};
    int net_code = 0;
    wxPoint pos;
    PCB_LAYER_ID track_layer_id;
    wxPoint track_start;
    wxPoint track_end;
    D_PAD* pad;
    TRACK* via;
    TRACK* track_seg;
    PICKED_ITEMS_LIST picked_items;
    TEARDROP* ret_tear = nullptr;
    bool locked = false;
    T token;

    for( token = aParser->NextTok();  token != T_RIGHT;  token = aParser->NextTok() )
    {
        if( token != T_LEFT )
            aParser->Expecting( T_LEFT );

        token = aParser->NextTok();

        switch( token )
        {
            case T_locked:
                locked = true;
                break;
            case T_shape:
                param.shape = (TEARDROP::SHAPES_T) aParser->parseInt("param shape");
                break;
            case T_length_ratio:
                param.length_ratio = aParser->parseInt("length_ratio");
                break;                
            case T_width_ratio:
                param.width_ratio = aParser->parseInt("width_ratio");
                break;
            case T_segments:
                param.num_segments = aParser->parseInt("segments");
                break;
            case T_position:
                pos.x = aParser->parseBoardUnits( "pos x" );
                pos.y = aParser->parseBoardUnits( "pos y" );
                break;
            case T_net:
                net_code = aParser->parseInt( "net number" );
                break;
            case T_layer:
                track_layer_id = aParser->parseBoardItemLayer();
                break;
            case T_start:
                track_start.x = aParser->parseBoardUnits( "start x" );
                track_start.y = aParser->parseBoardUnits( "start y" );
                break;
            case T_end:
                track_end.x = aParser->parseBoardUnits( "end x" );
                track_end.y = aParser->parseBoardUnits( "end y" );
                break;
            case T_parameters:
                SetShapeParams(param);
                SetCurrentShape(param.shape);                
                RecreateMenu();
                break;
            case T_via:
                via = m_Board->GetViaByPosition(pos, track_layer_id);
                track_seg = GetTrackSegment(track_start, track_end, track_layer_id, net_code);
                if(track_seg)
                    ret_tear = Add(track_seg, static_cast<BOARD_CONNECTED_ITEM*>(via), &picked_items);
                break;
            case T_pad:
                track_seg = GetTrackSegment(track_start, track_end, track_layer_id, net_code);
                if(track_seg)
                {
                    pad = m_Board->GetPad(pos, track_seg->GetLayerSet());
                    ret_tear = Add(track_seg, static_cast<BOARD_CONNECTED_ITEM*>(pad), &picked_items);
                }
                break;
            case T_junction:
                track_seg = GetTrackSegment(track_start, track_end, track_layer_id, net_code);
                if(track_seg)
                {
                    ret_tear = Add(track_seg, nullptr, &picked_items, pos);
                }
                break;
            default:
                aParser->Expecting( "shape, length_ratio, width_ratio, segments, teardrop, position, net, layer, start, end, parameters, via, pad, junction, locked" );
        }
        aParser->NeedRIGHT();
    }
    if(ret_tear)
        ret_tear->SetLocked(locked);
    return ret_tear;
}

