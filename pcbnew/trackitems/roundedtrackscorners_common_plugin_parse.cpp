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

#include "roundedtrackscorners.h"

using namespace TrackNodeItem;


//-----------------------------------------------------------------------------------------------------/
// Save
//-----------------------------------------------------------------------------------------------------/
void ROUNDEDTRACKSCORNERS::Format(OUTPUTFORMATTER* aOut, const int aNestLevel ) const throw( IO_ERROR )
{
    bool corners = false;
    ROUNDEDTRACKSCORNER::PARAMS prev_params = {0, 0};
    for( TRACK* track_seg = m_Board->m_Track;  track_seg; track_seg = track_seg->Next() )
    {
        if(track_seg->Type() == PCB_ROUNDEDTRACKSCORNER_T)
        {
            TRACK* connected_track = dynamic_cast<TRACK*>(dynamic_cast<ROUNDEDTRACKSCORNER*>(track_seg)->GetTrackSeg());
            if(connected_track && (connected_track->Type() == PCB_TRACE_T))
            {
                ROUNDEDTRACKSCORNER::PARAMS param = dynamic_cast<ROUNDEDTRACKSCORNER*>(track_seg)->GetParams();
                if(param != prev_params)
                {
                    aOut->Print(aNestLevel, "(roundedtrackscorner ");
                    aOut->Print(0, " (length_set %d) (length_ratio %d) (segments %d)", param.length_set, param.length_ratio, param.num_segments);
                    aOut->Print( 0, " (parameters))\n" );
                    prev_params = param;
                }
                aOut->Print(aNestLevel, "(roundedtrackscorner ");
                aOut->Print( 0, " (position %s) (net %d)", FMT_IU(track_seg->GetEnd() ).c_str(), track_seg->GetNetCode());
                aOut->Print( 0, " (layer %s)", aOut->Quotew(connected_track->GetLayerName()).c_str());
                aOut->Print(0, " (start %s) (end %s)", FMT_IU(connected_track->GetStart() ).c_str(),  FMT_IU(connected_track->GetEnd() ).c_str());
                if(dynamic_cast<ROUNDEDTRACKSCORNER*>(track_seg)->IsLocked())
                    aOut->Print(0, " (locked)");
                aOut->Print( 0, " (arc))\n");
                corners = true;
            }
        }
    }
    //If no corners, do not save even teardrop parameters.
    if(corners)
    {
        //Save current last. When load, it is current back.
        ROUNDEDTRACKSCORNER::PARAMS cur_params = GetParams();
        aOut->Print(aNestLevel, "(roundedtrackscorner ");
        aOut->Print(0, " (length_set %d) (length_ratio %d) (segments %d)", cur_params.length_set, cur_params.length_ratio, cur_params.num_segments);
        aOut->Print( 0, " (parameters))\n" );
    }
}

//-----------------------------------------------------------------------------------------------------/
// Load
//-----------------------------------------------------------------------------------------------------/
TRACKNODEITEM* ROUNDEDTRACKSCORNERS::Parse(PCB_PARSER* aParser) throw( IO_ERROR, PARSE_ERROR )
{
    using namespace PCB_KEYS_T;
    wxCHECK_MSG( aParser->CurTok() == T_roundedtrackscorner, nullptr,
                 wxT( "Cannot parse " ) + aParser->GetTokenString( aParser->CurTok() ) + wxT( " as roundedtrackscorner." ) );

    ROUNDEDTRACKSCORNER::PARAMS param = {0, 0};
    int net_code = 0;
    wxPoint pos;
    PCB_LAYER_ID track_layer_id;
    wxPoint track_start;
    wxPoint track_end;
    TRACK* track_seg;
    PICKED_ITEMS_LIST picked_items;
    ROUNDEDTRACKSCORNER* ret_corner = nullptr;
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
            case T_length_set:
                param.length_set = aParser->parseInt("length_set");
                break;                
            case T_length_ratio:
                param.length_ratio = aParser->parseInt("length_ratio");
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
                SetParams(param);
                RecreateMenu();
                break;
            case T_arc:
                track_seg = GetTrackSegment(track_start, track_end, track_layer_id, net_code);
                if(track_seg)
                    ret_corner = Add(track_seg, pos, &picked_items);
                break;
            default:
                aParser->Expecting( "arc, length_set, lengt_ratio, segments, roundedtrackscorner, position, net, layer, start, end, parameters, locked" );
        }
        aParser->NeedRIGHT();
    }
    if(ret_corner)
        ret_corner->SetLocked(locked);
    return ret_corner;
}


