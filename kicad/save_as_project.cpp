/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2018 KiCad Developers, see AUTHORS.txt for contributors.
 * @author Russell Oliver <roliver8143@gmail.com>
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
 * @file save_as_project.cpp
 * @brief routines for saving a project under a new name.
 */

 #include <wx/filename.h>
 #include <wx/dir.h>
 #include <wx/log.h>
 #include <wx/stdpaths.h>
 #include <wx/string.h>

 #include <common.h>
 #include <confirm.h>
 #include <hotkeys_basic.h>
 #include <kiway.h>
 #include <richio.h>
 #include <wildcards_and_files_ext.h>
 #include <systemdirsappend.h>
 #include <kiway_player.h>
 #include <stdexcept>
 #include "pgm_kicad.h"

 #include <io_mgr.h>
 #include <sch_io_mgr.h>
 #include <pcb_edit_frame.h>
 #include <sch_edit_frame.h>
 #include <netlist.h>

class PCB_EDIT_FRAME;

 #include "kicad.h"

void KICAD_MANAGER_FRAME::OnSaveAsProject( wxCommandEvent& event )
{
    static const wxChar* extentionList[] =
    {
        wxT( "*.sch" ), wxT( "*.lib" ),       wxT( "*.mod" ), wxT( "*.cmp" ),
        wxT( "*.brd" ), wxT( "*.kicad_pcb" ), wxT( "*.gbr" ), wxT( "*.pos" ),
        wxT( "*.net" ), wxT( "*.pro" ),       wxT( "*.drl" ), wxT( "*.py" ),
        wxT( "*.pdf" ), wxT( "*.txt" ),       wxT( "*.dcm" ), wxT( "*.kicad_wks" ), wxT(
                "*.kicad_mod" )
    };

    // Close other windows.
    if( !Kiway.PlayersClose( false ) )
        return;

    wxFileName  oldpro = GetProjectFileName();
    wxString    oldprjname( oldpro.GetName() );
    wxString    oldCwd = wxGetCwd();

    ClearMsg();


    // newpro.SetExt( ProjectFileExtension );

    wxString protitle = _( "KiCad Project Destination" );

    wxFileDialog prodlg( this, protitle, Prj().GetProjectPath(), wxEmptyString,
            ProjectFileWildcard(), wxFD_SAVE | wxFD_OVERWRITE_PROMPT );

    if( prodlg.ShowModal() == wxID_CANCEL )
        return;

    wxFileName newpro = prodlg.GetPath();
    newpro.SetExt( ProjectFileExtension );     // enforce extension


    if( !newpro.IsAbsolute() )
        newpro.MakeAbsolute();

    newpro.AppendDir( newpro.GetName() );

    // Check if the project directory is empty
    wxDir newdirectory( newpro.GetPath() );

    std::cout << newpro.GetPath() << '\n';

    if( !newpro.DirExists() )
    {
        if( !newpro.Mkdir() )
        {
            wxString msg;
            msg.Printf( _( "Directory \"%s\" could not be created.\n\n"
                           "Please make sure you have write permissions and try again." ),
                    newpro.GetPath() );
            DisplayErrorMessage( this, msg );
            return;
        }
    }
    else if( newdirectory.HasFiles() )
    {
        wxString msg = _( "The selected directory is not empty.  It is recommended that you "
                          "create projects in their own empty directory.\n\nDo you "
                          "want to continue?" );

        if( !IsOK( this, msg ) )
            return;
    }

    wxString currdirname = oldpro.GetPathWithSep();
    wxDir dir( currdirname );

    wxArrayString files;

    for( unsigned ii = 0; ii < DIM( extentionList ); ii++ )
        wxDir::GetAllFiles( currdirname, &files, extentionList[ii] );

    wxString newproname = newpro.GetName();

    wxString oldpropath = oldpro.GetPath();

    // Copy all files matching the extension list.

    for( unsigned ii = 0; ii < files.GetCount(); ii++ )
    {
        wxString oldfilepath = files[ii];

        oldfilepath.Replace( oldpropath, newpro.GetPath(), true );

        wxFileName  new_fn( oldfilepath );
        wxString    oldfilename = new_fn.GetName();

        // do not rename schematic libraries unless it is the cache library.
        if( new_fn.GetExt() != wxT( "lib" ) || new_fn.GetFullPath().EndsWith( "cache.lib" ) )
        {
            oldfilename.Replace( oldprjname, newproname, true );
            new_fn.SetName( oldfilename );
        }

        // create directories if necessary.
        if( !wxFileName::DirExists( new_fn.GetPath() ) )
            wxFileName::Mkdir( new_fn.GetPath(), wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL );

        wxCopyFile( files[ii], new_fn.GetFullPath(), true );
    }

    // Copy footprint library table
    wxFileName footprinttable( Prj().FootprintLibTblName() );

    if( footprinttable.Exists() )
    {
        footprinttable.SetPath( newpro.GetPath() );
        wxCopyFile( Prj().FootprintLibTblName(), footprinttable.GetFullPath(), true );
    }

    // Copy symbol library table
    wxFileName symboltable( Prj().SymbolLibTableName() );

    if( symboltable.Exists() )
    {
        symboltable.SetPath( newpro.GetPath() );
        wxCopyFile( Prj().SymbolLibTableName(), symboltable.GetFullPath(), true );

        // Replace any references to a library with the name of the project
        if( symboltable.Exists() )
        {
            wxCopyFile( Prj().SymbolLibTableName(), symboltable.GetFullPath(), true );

            wxTextFile symtable( symboltable.GetFullPath() );

            if( symtable.Open() )
            {
                for( auto str = symtable.GetFirstLine();
                     !symtable.Eof();
                     str = symtable.GetNextLine() )
                {
                    str.Replace( oldprjname, newproname, false );
                }

                symtable.Write();
                symtable.Close();
            }
        }
    }

    wxString msg = _( "Do you want to open the saved project?" );

    wxMessageDialog openproject( this, msg, _( "Open saved project?" ), wxYES_NO );

    openproject.SetYesNoLabels( _( "Yes" ), _( "No" ) );

    if( openproject.ShowModal() == wxID_YES )
    {
        LoadProject( newpro.GetFullPath() );
    }
}
