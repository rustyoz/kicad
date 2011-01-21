/*******************/
/* onleftclick.cpp */
/*******************/

#include "fctsys.h"
#include "common.h"
#include "eeschema_id.h"
#include "class_drawpanel.h"
#include "confirm.h"
#include "class_sch_screen.h"
#include "wxEeschemaStruct.h"

#include "general.h"
#include "protos.h"
#include "sch_bus_entry.h"
#include "sch_text.h"
#include "sch_marker.h"
#include "sch_junction.h"
#include "sch_line.h"
#include "sch_no_connect.h"
#include "sch_component.h"
#include "sch_sheet.h"


static wxArrayString s_CmpNameList;
static wxArrayString s_PowerNameList;


/* Process the command triggers by the left button of the mouse when a tool
 * is already selected.
 */
void SCH_EDIT_FRAME::OnLeftClick( wxDC* DC, const wxPoint& MousePos )
{
    SCH_ITEM* DrawStruct = GetScreen()->GetCurItem();

    if( ( m_ID_current_state == 0 ) || ( DrawStruct && DrawStruct->m_Flags ) )
    {
        DrawPanel->m_AutoPAN_Request = FALSE;
        m_itemToRepeat = NULL;

        if( DrawStruct && DrawStruct->m_Flags )
        {
            switch( DrawStruct->Type() )
            {
            case SCH_LABEL_T:
            case SCH_GLOBAL_LABEL_T:
            case SCH_HIERARCHICAL_LABEL_T:
            case SCH_TEXT_T:
            case SCH_SHEET_LABEL_T:
            case SCH_SHEET_T:
            case SCH_BUS_ENTRY_T:
            case SCH_JUNCTION_T:
            case SCH_COMPONENT_T:
            case SCH_FIELD_T:
                DrawStruct->Place( this, DC );
                GetScreen()->SetCurItem( NULL );
                GetScreen()->TestDanglingEnds();
                DrawPanel->Refresh( TRUE );
                return;

            case SCH_SCREEN_T:
                DisplayError( this, wxT( "OnLeftClick err: unexpected type for Place" ) );
                DrawStruct->m_Flags = 0;
                break;

            case SCH_LINE_T: // May already be drawing segment.
                break;

            default:
            {
                wxString msg;
                msg.Printf( wxT( "SCH_EDIT_FRAME::OnLeftClick err: m_Flags != 0, itmetype %d" ),
                            DrawStruct->Type());
                DisplayError( this, msg );
                DrawStruct->m_Flags = 0;
                break;
            }
            }
        }
        else
        {
            DrawStruct = SchematicGeneralLocateAndDisplay( true );
        }
    }

    switch( m_ID_current_state )
    {
    case 0:
        break;

    case ID_NO_SELECT_BUTT:
        break;

    case ID_HIERARCHY_PUSH_POP_BUTT:
        if( DrawStruct && DrawStruct->m_Flags )
            break;
        DrawStruct = SchematicGeneralLocateAndDisplay();
        if( DrawStruct && ( DrawStruct->Type() == SCH_SHEET_T ) )
        {
            InstallNextScreen( (SCH_SHEET*) DrawStruct );
        }
        else
            InstallPreviousSheet();
        break;

    case ID_NOCONN_BUTT:
        if( ( DrawStruct == NULL ) || ( DrawStruct->m_Flags == 0 ) )
        {
            m_itemToRepeat = CreateNewNoConnectStruct( DC );
            GetScreen()->SetCurItem( m_itemToRepeat );
            DrawPanel->m_AutoPAN_Request = TRUE;
        }
        else
        {
            DrawStruct->Place( this, DC );
            DrawPanel->m_AutoPAN_Request = FALSE;
        }
        GetScreen()->TestDanglingEnds();
        DrawPanel->Refresh( TRUE );
        break;

    case ID_JUNCTION_BUTT:
        if( ( DrawStruct == NULL ) || ( DrawStruct->m_Flags == 0 ) )
        {
            m_itemToRepeat = CreateNewJunctionStruct( DC, GetScreen()->m_Curseur, TRUE );
            GetScreen()->SetCurItem( m_itemToRepeat );
            DrawPanel->m_AutoPAN_Request = TRUE;
        }
        else
        {
            DrawStruct->Place( this, DC );
            DrawPanel->m_AutoPAN_Request = FALSE;
        }
        GetScreen()->TestDanglingEnds();
        DrawPanel->Refresh( TRUE );
        break;

    case ID_WIRETOBUS_ENTRY_BUTT:
    case ID_BUSTOBUS_ENTRY_BUTT:
        if( ( DrawStruct == NULL ) || ( DrawStruct->m_Flags == 0 ) )
        {
            DrawStruct = CreateBusEntry( DC,
                                         (m_ID_current_state == ID_WIRETOBUS_ENTRY_BUTT) ?
                                         WIRE_TO_BUS : BUS_TO_BUS );
            GetScreen()->SetCurItem( DrawStruct );
            DrawPanel->m_AutoPAN_Request = TRUE;
        }
        else
        {
            DrawStruct->Place( this, DC );
            GetScreen()->SetCurItem( NULL );
            GetScreen()->TestDanglingEnds();
            DrawPanel->Refresh( TRUE );
            DrawPanel->m_AutoPAN_Request = FALSE;
        }
        break;

    case ID_SCHEMATIC_DELETE_ITEM_BUTT:
        LocateAndDeleteItem( this, DC );
        OnModify( );
        GetScreen()->SetCurItem( NULL );
        GetScreen()->TestDanglingEnds();
        DrawPanel->Refresh( TRUE );
        break;

    case ID_WIRE_BUTT:
        BeginSegment( DC, LAYER_WIRE );
        DrawPanel->m_AutoPAN_Request = TRUE;
        break;

    case ID_BUS_BUTT:
        BeginSegment( DC, LAYER_BUS );
        DrawPanel->m_AutoPAN_Request = TRUE;
        break;

    case ID_LINE_COMMENT_BUTT:
        BeginSegment( DC, LAYER_NOTES );
        DrawPanel->m_AutoPAN_Request = TRUE;
        break;

    case ID_TEXT_COMMENT_BUTT:
        if( ( DrawStruct == NULL ) || ( DrawStruct->m_Flags == 0 ) )
        {
            GetScreen()->SetCurItem( CreateNewText( DC, LAYER_NOTES ) );
            DrawPanel->m_AutoPAN_Request = TRUE;
        }
        else
        {
            DrawStruct->Place( this, DC );
            DrawPanel->m_AutoPAN_Request = FALSE;
        }
        break;

    case ID_LABEL_BUTT:
        if( ( DrawStruct == NULL ) || ( DrawStruct->m_Flags == 0 ) )
        {
            GetScreen()->SetCurItem( CreateNewText( DC, LAYER_LOCLABEL ) );
            DrawPanel->m_AutoPAN_Request = TRUE;
        }
        else
        {
            DrawStruct->Place( this, DC );
            DrawPanel->m_AutoPAN_Request = FALSE;
            GetScreen()->TestDanglingEnds();
            DrawPanel->Refresh( TRUE );
        }
        break;

    case ID_GLABEL_BUTT:
    case ID_HIERLABEL_BUTT:
        if( (DrawStruct == NULL) || (DrawStruct->m_Flags == 0) )
        {
            if(m_ID_current_state == ID_GLABEL_BUTT)
                GetScreen()->SetCurItem( CreateNewText( DC, LAYER_GLOBLABEL ) );
            if(m_ID_current_state == ID_HIERLABEL_BUTT)
                GetScreen()->SetCurItem( CreateNewText( DC, LAYER_HIERLABEL ) );
            DrawPanel->m_AutoPAN_Request = TRUE;
        }
        else
        {
            DrawStruct->Place( this, DC );
            DrawPanel->m_AutoPAN_Request = FALSE;
            GetScreen()->TestDanglingEnds();
            DrawPanel->Refresh( TRUE );
        }
        break;

    case ID_SHEET_SYMBOL_BUTT:
        if( ( DrawStruct == NULL ) || ( DrawStruct->m_Flags == 0 ) )
        {
            GetScreen()->SetCurItem( CreateSheet( DC ) );
            DrawPanel->m_AutoPAN_Request = TRUE;
        }
        else
        {
            DrawStruct->Place( this, DC );
            DrawPanel->m_AutoPAN_Request = FALSE;
            GetScreen()->TestDanglingEnds();
            DrawPanel->Refresh( TRUE );
        }
        break;

    case ID_IMPORT_HLABEL_BUTT:
    case ID_SHEET_LABEL_BUTT:
        if( ( DrawStruct == NULL ) || ( DrawStruct->m_Flags == 0 ) )
            DrawStruct = SchematicGeneralLocateAndDisplay();

        if( DrawStruct == NULL )
            break;

        if( (DrawStruct->Type() == SCH_SHEET_T)
           && (DrawStruct->m_Flags == 0) )
        {
            if( m_ID_current_state == ID_IMPORT_HLABEL_BUTT )
                GetScreen()->SetCurItem( Import_PinSheet( (SCH_SHEET*) DrawStruct, DC ) );
            else
                GetScreen()->SetCurItem( Create_PinSheet( (SCH_SHEET*) DrawStruct, DC ) );
        }
        else if( (DrawStruct->Type() == SCH_SHEET_LABEL_T) && (DrawStruct->m_Flags != 0) )
        {
            DrawStruct->Place( this, DC );
            GetScreen()->TestDanglingEnds();
            DrawPanel->Refresh( TRUE );
        }
        break;

    case ID_COMPONENT_BUTT:
        if( (DrawStruct == NULL) || (DrawStruct->m_Flags == 0) )
        {
            GetScreen()->SetCurItem( Load_Component( DC, wxEmptyString, s_CmpNameList, TRUE ) );
            DrawPanel->m_AutoPAN_Request = TRUE;
        }
        else
        {
            DrawStruct->Place( this, DC );
            DrawPanel->m_AutoPAN_Request = FALSE;
            GetScreen()->TestDanglingEnds();
            DrawPanel->Refresh( TRUE );
        }
        break;

    case ID_PLACE_POWER_BUTT:
        if( ( DrawStruct == NULL ) || ( DrawStruct->m_Flags == 0 ) )
        {
            GetScreen()->SetCurItem( Load_Component( DC, wxT( "power" ), s_PowerNameList, FALSE ) );
            DrawPanel->m_AutoPAN_Request = TRUE;
        }
        else
        {
            DrawStruct->Place( this, DC );
            DrawPanel->m_AutoPAN_Request = FALSE;
            GetScreen()->TestDanglingEnds();
            DrawPanel->Refresh( TRUE );
        }
        break;

    default:
    {
        SetToolID( 0, wxCURSOR_ARROW, wxEmptyString );
        wxString msg( wxT( "SCH_EDIT_FRAME::OnLeftClick error state " ) );

        msg << m_ID_current_state;
        DisplayError( this, msg );
        break;
    }
    }
}


/**
 * Function OnLeftDClick
 * called on a double click event from the drawpanel mouse handler
 *  if an editable item is found (text, component)
 *      Call the suitable dialog editor.
 *  Id a create command is in progress:
 *      validate and finish the command
 */
void SCH_EDIT_FRAME::OnLeftDClick( wxDC* DC, const wxPoint& MousePos )

{
    EDA_ITEM* DrawStruct = GetScreen()->GetCurItem();
    wxPoint   pos = GetPosition();

    switch( m_ID_current_state )
    {
    case 0:
        if( ( DrawStruct == NULL ) || ( DrawStruct->m_Flags == 0 ) )
        {
            DrawStruct = SchematicGeneralLocateAndDisplay();
        }

        if( ( DrawStruct == NULL ) || ( DrawStruct->m_Flags != 0 ) )
            break;

        switch( DrawStruct->Type() )
        {
        case SCH_SHEET_T:
            InstallNextScreen( (SCH_SHEET*) DrawStruct );
            break;

        case SCH_COMPONENT_T:
            InstallCmpeditFrame( this, pos, (SCH_COMPONENT*) DrawStruct );
            DrawPanel->MouseToCursorSchema();
            break;

        case SCH_TEXT_T:
        case SCH_LABEL_T:
        case SCH_GLOBAL_LABEL_T:
        case SCH_HIERARCHICAL_LABEL_T:
            EditSchematicText( (SCH_TEXT*) DrawStruct );
            break;

        case SCH_FIELD_T:
            EditCmpFieldText( (SCH_FIELD*) DrawStruct, DC );
            DrawPanel->MouseToCursorSchema();
            break;

        case SCH_MARKER_T:
            ( (SCH_MARKER*) DrawStruct )->DisplayMarkerInfo( this );
            break;

        default:
            break;
        }

        break;

    case ID_BUS_BUTT:
    case ID_WIRE_BUTT:
    case ID_LINE_COMMENT_BUTT:
        if( DrawStruct && ( DrawStruct->m_Flags & IS_NEW ) )
            EndSegment( DC );
        break;
    }
}
