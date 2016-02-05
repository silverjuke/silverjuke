/*******************************************************************************
 *
 *                                 Silverjuke
 *     Copyright (C) 2015 Björn Petersen Software Design and Development
 *                   Contact: r10s@b44t.com, http://b44t.com
 *
 * This program is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation, either version 3 of the License, or (at your option) any later
 * version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program.  If not, see http://www.gnu.org/licenses/ .
 *
 *******************************************************************************
 *
 * File:    skinml.cpp
 * Authors: Björn Petersen
 * Purpose: Silverjuke skins - Skin [m]arkup [l]anguage parsing
 *
 *******************************************************************************
 *
 * Skin XML Format:
 *
 * <skin name="My skin name" debug="0">
 *     <layout name="Default">
 *         <img x="0" y="0" w="100%" h="60" src="bg_ctrl.png" />
 *         <button x="50%-189" y="63" src="ctrl_buttons.png" srcindex="0" target="play" />
 *         <button inactive="1" src="ctrl_buttons.png" srcindex="0" target="play" />
 *         <scrollbar x="12%+16" y="13" h="65" src="vol_slider.png" target="volSlider" />
 *         <workspace x="30" y="143" w="100%-60" h="100%-154">
 *             <color fgcolor="#000000" bgcolor="#D6DACC" target="normal" />
 *             <!-- other colours... -->
 *         </workspace>
 *         <!-- other items... -->
 *     </layout>
 * </skin>
 *
 * You'll get the idea.  The attributes "w" and "h" default to the
 * pixel size of the image or to any size, "x" and "y" default to 0.
 *
 * x/y/w/h may be given as pixels or percantage values; both refer to the
 * parent item which is the window if there is no parent used. Relative
 * and absolute values may be mixed in a simple way: [+|-]123<%<{+|-}123>>
 * So you can use positions as 100%-20. Other calculations are not supported.
 *
 * Moreover, for x/y you can use "next" to use the last coordinate plus
 * width/height or "same" to use the last position again.  For w/h you can
 * only use "same" to use the last size again.
 *
 * Targets are "play", "pause", "prev" etc.; see SJ_TARGET_*. Not all
 * targets work with all controls, eg. the "Volume" Target needs a slider
 * On the other hand it is okay to use the same Target more than one time.
 *
 * You can also use "layout:<layout name>" as a target for a button. This
 * allows you to use several switchable layouts, eg. you can show/hide
 * some items this way.
 *
 * With inactive="1" you can avoid a button or a scrollbar react to mouse
 * input.  This may be usefull if you have several items with the same
 * target but only one should be used by the user and the other will only
 * show the state.
 *
 * You can use some simple conditions: <if cond="osx">...</if>,
 * <if cond="!osx">...</if>, <if cond="gtk,linux">...</if> etc.
 *
 * Depending on the way the image is used several "subimages" are expected
 * inside an image (eg. to reflect different button states).  These subimages
 * are divided from each other using the colour of the first upper left pixel
 * in the image.  The second pixel defines a mask colour (optional) and the
 * third the colour to skip a subimage (optional).  See the examples to get
 * the idea.  The subimage index is currently only used by the buttons;
 * all other items need the subimages theirselves.
 *
 * To remember the meaning of the first three pixels, use the alphabeth:
 * [C]ontrolcolour [M]askcolour [S]kipcolour
 *
 ******************************************************************************/


#include <sjbase/base.h>
#include <sjbase/skin.h>
#include <sjbase/skinml.h>
#include <sjmodules/kiosk/kiosk.h>
#include <see_dom/sj_see.h>
#include <wx/tokenzr.h>
#include <wx/cmdline.h>


class SjSkinMlTagHandler : public wxHtmlTagHandler
{
public:
					 SjSkinMlTagHandler  (SjSkinMlParser* parser);
	                 ~SjSkinMlTagHandler ();
	virtual wxString GetSupportedTags    ();
	virtual bool     HandleTag           (const wxHtmlTag& tag);
	static wxString  GetFormattedTag     (const wxHtmlTag& tag, bool emptyOnNoParam);

private:
	SjLLHash        m_elseStack;
	SjSkinMlParser* m_skinMlParser;
	long            ParseLong           (const wxHtmlTag& tag, const wxString& paramName, long def);
	void            ParsePos            (const wxHtmlTag& tag, const wxString& paramName, SjSkinPos& pos);
	void            ParseImgSrc         (const wxHtmlTag& tag, SjSkinItem* item);
	void            ParseTarget         (const wxHtmlTag& tag, SjSkinItem* item);
	bool            ParseColourTarget   (const wxHtmlTag& tag, long& colourTargetId);
};


/*******************************************************************************
 * SjSkinMlParserData
 ******************************************************************************/


static SjSkinMlParserRaw s_target2IdTable[] =
{
	{ wxT("gotoa"),                 IDT_WORKSPACE_GOTO_A+0                           },
	{ wxT("gotob"),                 IDT_WORKSPACE_GOTO_A+1                           },
	{ wxT("gotoc"),                 IDT_WORKSPACE_GOTO_A+2                           },
	{ wxT("gotod"),                 IDT_WORKSPACE_GOTO_A+3                           },
	{ wxT("gotoe"),                 IDT_WORKSPACE_GOTO_A+4                           },
	{ wxT("gotof"),                 IDT_WORKSPACE_GOTO_A+5                           },
	{ wxT("gotog"),                 IDT_WORKSPACE_GOTO_A+6                           },
	{ wxT("gotoh"),                 IDT_WORKSPACE_GOTO_A+7                           },
	{ wxT("gotoi"),                 IDT_WORKSPACE_GOTO_A+8                           },
	{ wxT("gotoj"),                 IDT_WORKSPACE_GOTO_A+9                           },
	{ wxT("gotok"),                 IDT_WORKSPACE_GOTO_A+10                          },
	{ wxT("gotol"),                 IDT_WORKSPACE_GOTO_A+11                          },
	{ wxT("gotom"),                 IDT_WORKSPACE_GOTO_A+12                          },
	{ wxT("goton"),                 IDT_WORKSPACE_GOTO_A+13                          },
	{ wxT("gotoo"),                 IDT_WORKSPACE_GOTO_A+14                          },
	{ wxT("gotop"),                 IDT_WORKSPACE_GOTO_A+15                          },
	{ wxT("gotoq"),                 IDT_WORKSPACE_GOTO_A+16                          },
	{ wxT("gotor"),                 IDT_WORKSPACE_GOTO_A+17                          },
	{ wxT("gotos"),                 IDT_WORKSPACE_GOTO_A+18                          },
	{ wxT("gotot"),                 IDT_WORKSPACE_GOTO_A+19                          },
	{ wxT("gotou"),                 IDT_WORKSPACE_GOTO_A+20                          },
	{ wxT("gotov"),                 IDT_WORKSPACE_GOTO_A+21                          },
	{ wxT("gotow"),                 IDT_WORKSPACE_GOTO_A+22                          },
	{ wxT("gotox"),                 IDT_WORKSPACE_GOTO_A+23                          },
	{ wxT("gotoy"),                 IDT_WORKSPACE_GOTO_A+24                          },
	{ wxT("gotoz"),                 IDT_WORKSPACE_GOTO_Z                             },
	{ wxT("goto0"),                 IDT_WORKSPACE_GOTO_0_9                           },
	{ wxT("gotoprevletter"),        IDT_WORKSPACE_GOTO_PREV_AZ                       },
	{ wxT("gotonextletter"),        IDT_WORKSPACE_GOTO_NEXT_AZ                       },
	{ wxT("albumview"),             IDT_WORKSPACE_ALBUM_VIEW                         },
	{ wxT("coverview"),             IDT_WORKSPACE_COVER_VIEW                         },
	{ wxT("listview"),              IDT_WORKSPACE_LIST_VIEW                          },
	{ wxT("toggleview"),            IDT_WORKSPACE_TOGGLE_VIEW                        },
	{ wxT("displayup"),             IDT_DISPLAY_UP          | SJ_TARGET_REPEATBUTTON },
	{ wxT("displaydown"),           IDT_DISPLAY_DOWN        | SJ_TARGET_REPEATBUTTON },
	{ wxT("displayvscroll"),        IDT_DISPLAY_V_SCROLL                             },
	{ wxT("gotocurr"),              IDT_GOTO_CURR                                    },
	{ wxT("morefromcurralbum"),     IDT_MORE_FROM_CURR_ALBUM                         },
	{ wxT("morefromcurrartist"),    IDT_MORE_FROM_CURR_ARTIST                        },
	{ wxT("toggletimemode"),        IDT_TOGGLE_TIME_MODE                             },
	{ wxT("displaycover"),          IDT_DISPLAY_COVER                                },
	{ wxT("play"),                  IDT_PLAY                                         },
	{ wxT("pause"),                 IDT_PAUSE                                        },
	{ wxT("stop"),                  IDT_STOP                                         },
	{ wxT("prev"),                  IDT_PREV                                         },
	{ wxT("next"),                  IDT_NEXT                                         },
	{ wxT("fadetonext"),            IDT_FADE_TO_NEXT                                 },
	{ wxT("seek"),                  IDT_SEEK                | SJ_TARGET_NULLISBOTTOM },
	{ wxT("seekbwd"),               IDT_SEEK_BWD            | SJ_TARGET_REPEATBUTTON },
	{ wxT("seekfwd"),               IDT_SEEK_FWD            | SJ_TARGET_REPEATBUTTON },
	{ wxT("search"),                IDT_SEARCH                                       },
	{ wxT("searchbutton"),          IDT_SEARCH_BUTTON                                },
	{ wxT("searchinfo"),            IDT_SEARCH_INFO                                  },
	{ wxT("currcredit"),            IDT_CURR_CREDIT                                  },
	{ wxT("currtrack"),             IDT_CURR_TRACK                                   },
	{ wxT("nexttrack"),             IDT_NEXT_TRACK                                   },
	{ wxT("currtime"),              IDT_CURR_TIME                                    },
	{ wxT("volslider"),             IDT_MAIN_VOL_SLIDER     | SJ_TARGET_NULLISBOTTOM },
	{ wxT("volup"),                 IDT_MAIN_VOL_UP         | SJ_TARGET_REPEATBUTTON },
	{ wxT("voldown"),               IDT_MAIN_VOL_DOWN       | SJ_TARGET_REPEATBUTTON },
	{ wxT("mute"),                  IDT_MAIN_VOL_MUTE                                },
	{ wxT("workspace"),             IDT_WORKSPACE                                    },
	{ wxT("workspaceleft"),         IDT_WORKSPACE_LINE_LEFT | SJ_TARGET_REPEATBUTTON },
	{ wxT("workspaceright"),        IDT_WORKSPACE_LINE_RIGHT| SJ_TARGET_REPEATBUTTON },
	{ wxT("workspaceup"),           IDT_WORKSPACE_LINE_UP   | SJ_TARGET_REPEATBUTTON },
	{ wxT("workspacedown"),         IDT_WORKSPACE_LINE_DOWN | SJ_TARGET_REPEATBUTTON },
	{ wxT("workspacepageleft"),     IDT_WORKSPACE_PAGE_LEFT | SJ_TARGET_REPEATBUTTON },
	{ wxT("workspacepageright"),    IDT_WORKSPACE_PAGE_RIGHT| SJ_TARGET_REPEATBUTTON },
	{ wxT("workspacepageup"),       IDT_WORKSPACE_PAGE_UP   | SJ_TARGET_REPEATBUTTON },
	{ wxT("workspacepagedown"),     IDT_WORKSPACE_PAGE_DOWN | SJ_TARGET_REPEATBUTTON },
	{ wxT("workspacehscroll"),      IDT_WORKSPACE_H_SCROLL                           },
	{ wxT("workspacevscroll"),      IDT_WORKSPACE_V_SCROLL                           },
	{ wxT("workspacedelete"),       IDT_WORKSPACE_DELETE                             },
	{ wxT("workspaceinsert"),       IDT_WORKSPACE_INSERT                             },
	{ wxT("cursorleft"),            IDT_WORKSPACE_KEY_LEFT  | SJ_TARGET_REPEATBUTTON },
	{ wxT("cursorright"),           IDT_WORKSPACE_KEY_RIGHT | SJ_TARGET_REPEATBUTTON },
	{ wxT("cursorup"),              IDT_WORKSPACE_KEY_UP    | SJ_TARGET_REPEATBUTTON },
	{ wxT("cursordown"),            IDT_WORKSPACE_KEY_DOWN  | SJ_TARGET_REPEATBUTTON },
	{ wxT("gotofirst"),             IDT_WORKSPACE_HOME                               },
	{ wxT("gotolast"),              IDT_WORKSPACE_END                                },
	{ wxT("showcovers"),            IDT_WORKSPACE_SHOW_COVERS                        },
	{ wxT("zoomin"),                IDT_ZOOM_IN             | SJ_TARGET_REPEATBUTTON },
	{ wxT("zoomout"),               IDT_ZOOM_OUT            | SJ_TARGET_REPEATBUTTON },
	{ wxT("zoomnormal"),            IDT_ZOOM_NORMAL                                  },
	{ wxT("enqueuelast"),           IDT_ENQUEUE_LAST                                 },
	{ wxT("enqueuenext"),           IDT_ENQUEUE_NEXT                                 },
	{ wxT("enqueuenow"),            IDT_ENQUEUE_NOW                                  },
	{ wxT("unqueue"),               IDT_UNQUEUE                                      },
	{ wxT("unqueueall"),            IDT_UNQUEUE_ALL                                  },
	{ wxT("mainmenu"),              IDT_MAINMENU                                     },
	{ wxT("startvis"),              IDT_VIS_TOGGLE                                   },
	{ wxT("updateindex"),           IDT_UPDATE_INDEX                                 },
	{ wxT("deepupdateindex"),       IDT_DEEP_UPDATE_INDEX                            },
	{ wxT("settings"),              IDT_SETTINGS                                     },
	{ wxT("gotorandom"),            IDT_WORKSPACE_GOTO_RANDOM                        },
	{ wxT("stopafterthistrack"),    IDT_STOP_AFTER_THIS_TRACK                        },
	{ wxT("stopaftereachtrack"),    IDT_STOP_AFTER_EACH_TRACK                        },
	{ wxT("shuffle"),               IDT_SHUFFLE                                      },
	{ wxT("repeat"),                IDT_REPEAT                                       },
	{ wxT("alwaysontop"),           IDT_ALWAYS_ON_TOP                                },
	{ wxT("visrect"),               IDT_VIS_RECT                                     },
	{ wxT("openfiles"),             IDT_OPEN_FILES                                   },
	{ wxT("openurl"),               IDT_OPEN_FILES                                   },
	{ wxT("appendfiles"),           IDT_APPEND_FILES                                 },
	{ wxT("saveplaylist"),          IDT_SAVE_PLAYLIST                                },
	{ wxT("advsearch"),             IDT_ADV_SEARCH                                   },
	{ wxT("togglekiosk"),           IDT_TOGGLE_KIOSK                                 },
	{ wxT("doubleclickaction"),     IDT_PLAY_NOW_ON_DBL_CLICK                        },
	{ wxT("removeplayed"),          IDT_TOGGLE_REMOVE_PLAYED                         },
	{ wxT("prelisten"),             IDT_PRELISTEN                                    },

	{ NULL, 0 }
};
SjSLHash* SjSkinMlParserData::s_targetHash = NULL;


static SjSkinMlParserRaw s_colour2IdTable[] =
{
	{ wxT("normal"),                SJ_COLOUR_NORMAL+1 },
	{ wxT("normalodd"),             SJ_COLOUR_NORMALODD+1 },
	{ wxT("selection"),             SJ_COLOUR_SELECTION+1 },
	{ wxT("selectionodd"),          SJ_COLOUR_SELECTIONODD+1 },
	{ wxT("title1"),                SJ_COLOUR_TITLE1+1 },
	{ wxT("title2"),                SJ_COLOUR_TITLE2+1 },
	{ wxT("title3"),                SJ_COLOUR_TITLE3+1 },
	{ wxT("verttext"),              SJ_COLOUR_VERTTEXT+1 },
	{ wxT("stubtext"),              SJ_COLOUR_STUBTEXT+1 },

	{ NULL, 0 }
};
SjSLHash* SjSkinMlParserData::s_colourHash = NULL;


SjSLHash* SjSkinMlParserData::HashInit(SjSkinMlParserRaw* src)
{
	SjSLHash* dest = new SjSLHash();
	if( !dest )
	{
		wxLogFatalError(wxT("Out of hash memory.")/*n/t*/);
	}

	while( src->m_name )
	{
		wxASSERT( dest->Lookup(src->m_name) == 0 );
		wxASSERT( wxString(src->m_name).Find('_') == -1 );
		wxASSERT( wxString(src->m_name).Find(' ') == -1 );
		wxASSERT( wxString(src->m_name).Lower() == src->m_name );

		dest->Insert(src->m_name, src->m_id);
		src++;
	}

	return dest;
}


void SjSkinMlParserData::FreeStaticData()
{
	// if we're not in debug mode and we're in shutdown,
	// just let the OS free the memory, there is no advantage
	// to do it here (but some disadvantages,  eg. speed)

	#ifdef __WXDEBUG__
		if( s_targetHash )
			delete s_targetHash;

		if( s_colourHash )
			delete s_colourHash;
	#endif

	s_targetHash = NULL;
	s_colourHash = NULL;
}


wxString SjSkinMlParserData::LoadFile_(const wxString& xmlCmdFile)
{
	wxString commandsFileName = m_skin->m_baseDir + xmlCmdFile;

	wxFileSystem fs;
	wxFSFile* fsFile = fs.OpenFile(commandsFileName);
	if( !fsFile )
	{
		wxLogError(wxT("Cannot open \"%s\" [%s]"), commandsFileName.c_str(), GetUrl().c_str());
		return wxEmptyString;
	}

	wxString content = SjTools::GetFileContent(fsFile->GetStream(), &wxConvUTF8);

	delete fsFile;

	return content;
}


void SjSkinMlParserData::LogError(const wxString& error__, const wxHtmlTag* tagPosition/*may be NULL*/)
{
	wxString error(error__);
	if( error.IsEmpty() )
		error = wxT("Unknown error")/*n/t*/;

	if( tagPosition )
	{
		wxString tag = SjSkinMlTagHandler::GetFormattedTag(*tagPosition, TRUE/*emptyOnNoParam*/);
		if( tag.IsEmpty() )
		{
			// HACK: i don't know why -- sometimes the wxHTMLParser gives me
			// empty tags (true for wxWidgets 2.4.2). In these cases,
			// don't throw out an error.
			return; // assume, this is no error
		}
		error += wxT(" at ") + tag;
	}

	wxLogError(wxT("%s [%s]"), error.c_str(), GetUrl().c_str());
}


wxString SjSkinMlParserData::GetUrl()
{
	if( m_skin )
		return m_skin->GetUrl();
	else
		return wxT("Skin");
}


bool SjSkinMlParserData::CheckName(const wxString& name, const wxHtmlTag* tagPosition)
{
	if( name.First(wxT("?"))!=wxNOT_FOUND )
	{
		LogError(wxT("The character \"?\" is not allowed in skin files, paths or names.")/*n/t*/, tagPosition); // "?" is used as the settings seperator
		return FALSE;
	}

	return TRUE;
}


bool SjSkinMlParserData::CreateNAppendItem(const wxHtmlTag& tag, SjSkinItem* item)
{
	wxString error;

	if( item->Create(tag, error) )
	{
		wxASSERT(m_currLayout);
		m_currLayout->m_itemList.Append(item);

		if( !error.IsEmpty() )
		{
			LogError(error, &tag);
		}

		return TRUE;
	}

	LogError(error, &tag);
	return FALSE;
}


long SjSkinMlParserData::ParseCond(const wxString& str__, bool& notFlag)
{
	wxString str(str__);
	str.Replace(wxT(" "), wxT(""));
	str.MakeLower();

	// negate condition?
	notFlag = FALSE;
	if( str.StartsWith(wxT("!")) )
	{
		str = str.Mid(1);
		notFlag = TRUE;
	}

	// go through all items and collect the flags
	long flags = 0;
	wxStringTokenizer tkz(str, wxT(","));
	while ( tkz.HasMoreTokens() )
	{
		wxString token = tkz.GetNextToken();
		     if( token == wxT("win")                                )   { flags |= SJ_OP_OS_WIN;            }
		else if( token == wxT("mac")                                )   { flags |= SJ_OP_OS_MAC;            }
		else if( token == wxT("gtk")                                )   { flags |= SJ_OP_OS_GTK;            }
		else if( token == wxT("kiosk")                              )   { flags |= SJ_OP_KIOSKON;           }
		else if( token == wxT("creditsystem")                       )   {
			SjTools::SetFlag(m_currCond, SJ_OP_CREDIT_SYS__, SjCreditBase::IsCreditSystemEnabled());
			flags |= SJ_OP_CREDIT_SYS__;
		}
		else if( token == wxT("volume")                             )   { flags |= SJ_OP_MAIN_VOL;          }
		else if( token == wxT("prelisten")                          )   { flags |= SJ_OP_PRELISTEN;         }
		else if( token == wxT("playpause")                          )   { flags |= SJ_OP_PLAYPAUSE;         }
		else if( token == wxT("search")                             )   { flags |= SJ_OP_SEARCH;            }
		else if( token == wxT("musicsel")                           )   { flags |= SJ_OP_MUSIC_SEL;         }
		else if( token == wxT("musicselgenre")                      )   { flags |= SJ_OP_MUSIC_SEL_GENRE;   }
		else if( token == wxT("startvis")                           )   { flags |= SJ_OP_STARTVIS;          }
		else if( token == wxT("editqueue")                          )   { flags |= SJ_OP_EDIT_QUEUE;        }
		else if( token == wxT("unqueue")                            )   { flags |= SJ_OP_UNQUEUE;           }
		else if( token == wxT("enlargedisplay")/*for sj<2.52beta2*/ )   { flags |= SJ_OP_TOGGLE_ELEMENTS;   }
		else if( token == wxT("toggleelements")                     )   { flags |= SJ_OP_TOGGLE_ELEMENTS;   }
		else if( token == wxT("toggletimemode")                     )   { flags |= SJ_OP_TOGGLE_TIME_MODE;  }
		else if( token == wxT("albumview")                          )   { flags |= SJ_OP_ALBUM_VIEW;        }
		else if( token == wxT("coverview")                          )   { flags |= SJ_OP_COVER_VIEW;        }
		else if( token == wxT("listview")                           )   { flags |= SJ_OP_LIST_VIEW;         }
		else if( token == wxT("toggleview")                         )   { flags |= SJ_OP_TOGGLE_VIEW__;     }
		else if( token == wxT("zoom")                               )   { flags |= SJ_OP_ZOOM;              }
		else if( token == wxT("repeat")                             )   { flags |= SJ_OP_REPEAT;            }
		else if( token == wxT("all")                                )   { flags |= SJ_OP_ALL;               }
	}

	return flags;
}


bool SjSkinMlParserData::CheckCond(const wxHtmlTag* tag)
{
	wxString str;

	str = tag->GetParam(wxT("COND"));
	if( !str.IsEmpty() )
	{
		bool notFlag;
		long flags = ParseCond(str, notFlag);
		if( notFlag )
		{
			return (flags & m_currCond)? false : true;
		}
		else
		{
			return (flags & m_currCond)? true : false;
		}
	}

	str = tag->GetParam(wxT("VERSION"));
	if( !str.IsEmpty() )
	{
		if( ((SJ_VERSION_MAJOR<<24)|(SJ_VERSION_MINOR<<16)|(SJ_VERSION_REVISION<<8)) >= SjTools::VersionString2Long(str) )
		{
			return true;
		}
	}

	return false;
}


/*******************************************************************************
 * SjSkinMlTagHandler
 ******************************************************************************/


wxString SjSkinMlTagHandler::GetFormattedTag(const wxHtmlTag& tag, bool emptyOnNoParam)
{
	wxString allParam = tag.GetAllParams();
	allParam.Replace(wxT("/=\"\""), wxT(""));

	if( allParam.Len()<8 && emptyOnNoParam )
	{
		// HACK: i don't know why -- sometimes the wxHTMLParser gives me
		// empty tags (true for wxWidgets 2.4.2). In these cases,
		// don't throw out an error (this function is called by LogError()).
		return wxT("");
	}
	else
	{
		return wxString::Format(wxT("<%s %s>"), tag.GetName().Lower().c_str(), allParam.c_str());
	}
}


SjSkinMlTagHandler::SjSkinMlTagHandler(SjSkinMlParser* parser)
{
	m_skinMlParser = parser;
}


SjSkinMlTagHandler::~SjSkinMlTagHandler()
{
}


wxString SjSkinMlTagHandler::GetSupportedTags()
{
	return wxT("IF, ELSE, SKIN, LAYOUT, INCLUDE, SCRIPT, TOOLTIPS, COLOR, BOX, DIV, IMG, BUTTON, SCROLLBAR, WORKSPACE, INPUT");
}


SjSkinColour::SjSkinColour()
{
	fgColour.Set(0, 0, 0);
	fgPen       = *wxBLACK_PEN;
	//fgBrush   = *wxBLACK_BRUSH; -- not needed
	fgSet       = FALSE;

	bgColour.Set(255, 255, 255);
	bgPen       = *wxWHITE_PEN;
	bgBrush     = *wxWHITE_BRUSH;
	bgSet       = FALSE;

	hiColour.Set(255, 0, 0);
	hiPen       = *wxRED_PEN;
	//hiBrush   = *wxRED_BRUSH; -- not needed
	hiSet       = FALSE;

	offsetX     = 1;
	offsetY     = 0;
}


void SjSkinColour::SetFromTag(const wxHtmlTag& tag)
{
	if( tag.HasParam(wxT("BGCOLOR")) )
	{
		if( tag.GetParamAsColour(wxT("BGCOLOR"), &bgColour) )
		{
			bgPen.SetColour(bgColour);
			bgBrush.SetColour(bgColour);
			bgSet = TRUE;
		}
	}

	if( tag.HasParam(wxT("FGCOLOR")) )
	{
		if( tag.GetParamAsColour(wxT("FGCOLOR"), &fgColour) )
		{
			fgPen.SetColour(fgColour);
			//fgBrush.SetColour(fgColour); -- not needed
			fgSet = TRUE;
		}
	}

	if( tag.HasParam(wxT("HICOLOR")) )
	{
		if( tag.GetParamAsColour(wxT("HICOLOR"), &hiColour) )
		{
			hiPen.SetColour(hiColour);
			//hiBrush.SetColour(hiColour); -- not needed
			hiSet = TRUE;
		}
	}

	if( tag.HasParam(wxT("OFFSETX")) )
	{
		tag.GetParamAsInt(wxT("OFFSETX"), &offsetX);
	}

	if( tag.HasParam(wxT("OFFSETY")) )
	{
		tag.GetParamAsInt(wxT("OFFSETY"), &offsetY);
	}
}


bool SjSkinMlTagHandler::HandleTag(const wxHtmlTag& tag)
{
	wxString tagName = tag.GetName();

	SjSkinSkin* skin = m_skinMlParser->m_data->m_skin;
	wxASSERT(skin);
	if( !skin ) { return FALSE; }

	if( tagName == wxT("IF") )
	{
		/* handle <IF> and <ELSE>
		 ***********************************************************************************/

		if( m_skinMlParser->m_data->CheckCond(&tag) )
		{
			m_elseStack.Remove((long)tag.GetParent());
			ParseInner(tag);
		}
		else
		{
			m_elseStack.Insert((long)tag.GetParent(), 1);
		}
		return TRUE; // parse inner called or inner is void
	}
	else if( tagName == wxT("ELSE") )
	{
		if( m_elseStack.Lookup((long)tag.GetParent()) )
		{
			m_elseStack.Remove((long)tag.GetParent());
			ParseInner(tag);
		}
		return TRUE; // parse inner called or inner is void
	}
	else if( tagName == wxT("SKIN") )
	{
		/* handle <SKIN>
		 ***********************************************************************************/

		// check for errors
		if( m_skinMlParser->m_data->m_skinCount )
		{
			m_skinMlParser->m_data->LogError(wxT("Multiple <skin> tags found")/*n/t*/, &tag);
			return FALSE;
		}

		// get skin parameters
		if( tag.HasParam(wxT("NAME")) )
		{
			skin->m_name = tag.GetParam(wxT("NAME"));
			if( m_skinMlParser->m_data->m_loadNameOnly )
			{
				m_skinMlParser->StopParsing();
				return false;
			}
		}

		if( tag.HasParam(wxT("ABOUT")) )
		{
			skin->m_about = tag.GetParam(wxT("ABOUT"));
		}

		if( tag.HasParam(wxT("DEBUGCOND")) )
		{
			wxString cond = tag.GetParam(wxT("DEBUGCOND"));
			bool dummyNot;
			m_skinMlParser->m_data->m_currCond =
			    m_skinMlParser->m_data->ParseCond(cond, dummyNot);
			wxLogWarning(wxT("Condition set to \"%s\" [%s]")/*n/t*/, cond.c_str(), m_skinMlParser->m_data->GetUrl().c_str());
		}

		if( tag.HasParam(wxT("DEBUGOUTLINE")) )
		{
			skin->m_debugOutline = ParseLong(tag, wxT("DEBUGOUTLINE"), 0)!=0;
		}

		if( !skin->m_debugInfo )
		{
			skin->m_debugInfo = ParseLong(tag, wxT("DEBUGINFO"), 0)!=0;
		}

		if( skin->m_debugInfo )
		{
			wxLogInfo(wxT("Initializing skin \"%s\" [%s]")/*n/t*/, skin->m_name.c_str(), m_skinMlParser->m_data->GetUrl().c_str());
		}

		if( skin->m_name == wxT("") )
		{
			m_skinMlParser->m_data->LogError(wxT("Name attribute missing")/*n/t*/, &tag);
			skin->m_name = wxT("Noname");
		}

		if( !m_skinMlParser->m_data->CheckName(skin->m_name, &tag) )
		{
			return FALSE;
		}

		// parse skin
		m_skinMlParser->m_data->m_skinCount++; // may never be larger than 1!
		if( tag.GetChildren() )
		{
			ParseInner(tag);
		}

		return TRUE; // ParseInner() called
	}
	else if( tagName == wxT("TOOLTIPS") )
	{
		/* handle <TOOLTIPS>
		 ***********************************************************************************/

		if( m_skinMlParser->m_data->m_skinCount != 1 )
		{
			m_skinMlParser->m_data->LogError(wxT("<skin> not yet initialized")/*n/t*/, &tag);
			return FALSE;
		}

		tag.GetParamAsColour(wxT("BGCOLOR"), &skin->m_tooltipBgColour);
		tag.GetParamAsColour(wxT("FGCOLOR"), &skin->m_tooltipFgColour);

		if( tag.HasParam(wxT("BORDERCOLOR")) )
		{
			tag.GetParamAsColour(wxT("BORDERCOLOR"), &skin->m_tooltipBorderColour);
		}
		else
		{
			skin->m_tooltipBorderColour = skin->m_tooltipFgColour;
		}

		skin->m_tooltipColoursSet = TRUE;
	}
	else if( tagName == wxT("LAYOUT") )
	{
		/* handle <LAYOUT>
		 ***********************************************************************************/

		// check for errors
		if( m_skinMlParser->m_data->m_skinCount != 1 )
		{
			m_skinMlParser->m_data->LogError(wxT("<skin> not yet initialized")/*n/t*/, &tag);
			return FALSE;
		}

		if( m_skinMlParser->m_data->m_currLayout || m_skinMlParser->m_data->m_itemStack.GetCount() != 0 )
		{
			m_skinMlParser->m_data->LogError(wxT("<layout> tags may not be nested")/*n/t*/, &tag);
			return FALSE;
		}

		if( skin->m_layoutList.GetCount() > (IDT_LAYOUT_LAST-IDT_LAYOUT_FIRST) )
		{
			m_skinMlParser->m_data->LogError(wxT("Too many layouts"), &tag);
			return FALSE;
		}

		// get layout name
		wxString layoutName;
		if( tag.HasParam(wxT("NAME")) )
		{
			layoutName = tag.GetParam(wxT("NAME"));
		}

		if( skin->m_debugInfo )
		{
			wxLogInfo(wxT("Initializing layout \"%s\" [%s]")/*n/t*/,layoutName.c_str(), m_skinMlParser->m_data->GetUrl().c_str());
		}

		if( layoutName.IsEmpty() )
		{
			m_skinMlParser->m_data->LogError(wxT("Name attribute missing")/*n/t*/, &tag);
			layoutName = wxT("Noname");
		}

		if( !m_skinMlParser->m_data->CheckName(skin->m_name, &tag) )
		{
			return FALSE;
		}

		// is the layout name unique?
		if( skin->GetLayout(layoutName) )
		{
			m_skinMlParser->m_data->LogError(wxT("There is already a layout with this name")/*n/t*/, &tag);
			return FALSE;
		}

		// Allocate layout object, append layout to the list of layouts in the skin, create root item
		SjSkinLayout* currLayout = m_skinMlParser->m_data->m_currLayout = new SjSkinLayout(skin, layoutName, skin->m_layoutList.GetCount());
		SjSkinDivItem* rootItem = new SjSkinDivItem();
		if( !currLayout
		        || !skin->m_layoutList.Append(currLayout)
		        || !rootItem )
		{
			m_skinMlParser->m_data->LogError(wxT("Out of memory")/*n/t*/, &tag);
			return FALSE;
		}

		rootItem->m_colours = skin->m_itemDefColours;
		rootItem->m_width.SetRel(10000);
		rootItem->m_height.SetRel(10000);
		ParseTarget(tag, rootItem);
		m_skinMlParser->m_data->CreateNAppendItem(tag, rootItem);

		// get layout sizes
		int i;
		if( tag.HasParam(wxT("MINW")) ) { tag.GetParamAsInt(wxT("MINW"), &i); currLayout->m_minSize.x = i; }
		if( tag.HasParam(wxT("MINH")) ) { tag.GetParamAsInt(wxT("MINH"), &i); currLayout->m_minSize.y = i; }
		if( tag.HasParam(wxT("MAXW")) ) { tag.GetParamAsInt(wxT("MAXW"), &i); currLayout->m_maxSize.x = i; }
		if( tag.HasParam(wxT("MAXH")) ) { tag.GetParamAsInt(wxT("MAXH"), &i); currLayout->m_maxSize.y = i; }
		if( tag.HasParam(wxT("DEFW")) ) { tag.GetParamAsInt(wxT("DEFW"), &i); currLayout->m_defSize.x = i; }
		if( tag.HasParam(wxT("DEFH")) ) { tag.GetParamAsInt(wxT("DEFH"), &i); currLayout->m_defSize.y = i; }
		if( tag.HasParam(wxT("USEW")) ) { currLayout->m_useWidth  = tag.GetParam(wxT("USEW")).Lower();    }
		if( tag.HasParam(wxT("USEH")) ) { currLayout->m_useHeight = tag.GetParam(wxT("USEH")).Lower();   }
		if( tag.HasParam(wxT("USEPOS"))) { currLayout->m_usePos    = tag.GetParam(wxT("USEPOS")).Lower();   }

		// Parse layout
		if( tag.GetChildren() )
		{
			m_skinMlParser->m_data->m_itemStack.Append(rootItem);

			ParseInner(tag);

			m_skinMlParser->m_data->m_itemStack.DeleteObject(rootItem);
		}

		m_skinMlParser->m_data->m_currLayout = NULL;
		return TRUE; // ParseInner() called
	}
	else if( tagName == wxT("COLOR") )
	{
		/* Handle <COLOR>
		 ***********************************************************************************/

		SjSkinItem* currItem = m_skinMlParser->m_data->GetLastItem();
		if( !currItem )
		{
			m_skinMlParser->m_data->LogError(wxT("No <item> yet initialized")/*n/t*/, &tag);
			return FALSE;
		}

		if( currItem->m_colours == skin->m_itemDefColours )
		{
			currItem->m_colours = new SjSkinColour[SJ_COLOUR_COUNT];
		}

		long colourTargetId;
		if( !ParseColourTarget(tag, colourTargetId) )
		{
			return FALSE;
		}

		currItem->m_colours[colourTargetId].SetFromTag(tag);
	}
	else if( tagName == wxT("INCLUDE") )
	{
		/* Handle <INCLUDE>
		 ***********************************************************************************/

		#define MAX_INCLUDE_RECURSION 16
		if( m_skinMlParser->m_data->m_includeRecursion > MAX_INCLUDE_RECURSION )
		{
			m_skinMlParser->m_data->LogError(wxT("Too deep <include> recursion"), &tag);
		}
		else if( tag.HasParam(wxT("SRC")) || tag.HasParam(wxT("FILE"))/*FILE was used up to 2.10beta7*/ )
		{
			wxString inclFile = tag.GetParam(tag.HasParam(wxT("SRC"))? wxT("SRC") : wxT("FILE"));

			if( skin->m_debugInfo )
			{
				wxLogInfo(wxT("Including \"%s\" [%s]")/*n/t*/, inclFile.c_str(), m_skinMlParser->m_data->GetUrl().c_str());
			}

			SjSkinMlParser   inclParser(m_skinMlParser->m_data, 0);
			wxString            inclContent = inclParser.m_data->LoadFile_(inclFile);

			m_skinMlParser->m_data->m_includeRecursion++;
			inclParser.Parse(inclContent);
			m_skinMlParser->m_data->m_includeRecursion--;
		}
		else
		{
			m_skinMlParser->m_data->LogError(wxT("No file given for <include>")/*n/t*/, &tag);
		}

		return FALSE; // parse inner not called
	}
	else if( tagName == wxT("SCRIPT") )
	{
		/* Handle <SCRIPT>
		 ***********************************************************************************/

		if( !m_skinMlParser->m_data->m_loadNameOnly )
		{
			wxString scriptContent;

			// first, execute external file
			if( tag.HasParam(wxT("SRC")) )
			{
				wxString scriptFile = tag.GetParam(wxT("SRC"));
				if( skin->m_debugInfo )
				{
					wxLogInfo(wxT("Including \"%s\" [%s]")/*n/t*/, scriptFile.c_str(), m_skinMlParser->m_data->GetUrl().c_str());
				}

				scriptContent = m_skinMlParser->m_data->LoadFile_(scriptFile);

				skin->m_globalScript += scriptContent + "\n\n";
				skin->m_hasScripts = true;
			}

			// second, execute stuff between <script> and </script>
			if( tag.HasEnding() )
			{
				scriptContent = m_skinMlParser->GetSource()->Mid(tag.GetBeginPos(), tag.GetEndPos1()-tag.GetBeginPos());
				scriptContent.Trim(true);
				scriptContent.Trim(false);
				if( !scriptContent.IsEmpty() )
				{
					if( scriptContent.StartsWith(wxT("<!--")) )
						scriptContent = scriptContent.Mid(4);

					if( scriptContent.Right(3) == wxT("-->") )
						scriptContent = scriptContent.Left(scriptContent.Len()-3);

					skin->m_globalScript += scriptContent + "\n\n";
					skin->m_hasScripts = true;
				}
			}
		}

		return FALSE; // parse inner not called
	}
	else
	{
		/* Handle all other item commands, Items may be nested!
		 ***********************************************************************************/

		// check for errors
		if( m_skinMlParser->m_data->m_currLayout == NULL )
		{
			m_skinMlParser->m_data->LogError(wxT("<layout> not yet started")/*n/t*/, &tag);
			return FALSE;
		}

		// create the item
		SjSkinItem* currItem;
		if( tagName == wxT("BUTTON") )
		{
			currItem = new SjSkinButtonItem();
		}
		else if( tagName == wxT("BOX") )
		{
			currItem = new SjSkinBoxItem();
		}
		else if( tagName == wxT("IMG") )
		{
			currItem = new SjSkinImageItem();
		}
		else if( tagName == wxT("SCROLLBAR") )
		{
			currItem = new SjSkinScrollbarItem();
		}
		else if( tagName == wxT("WORKSPACE") )
		{
			currItem = new SjSkinWorkspaceItem();
			m_skinMlParser->m_data->m_currLayout->m_hasWorkspace = TRUE;
		}
		else if( tagName == wxT("INPUT") )
		{
			currItem = new SjSkinInputItem();
			m_skinMlParser->m_data->m_currLayout->m_hasInput = TRUE;
		}
		else if( tagName == wxT("DIV") )
		{
			currItem = new SjSkinDivItem();
		}
		else
		{
			m_skinMlParser->m_data->LogError(wxT("Invalid tag")/*n/t*/, &tag);
			return FALSE;
		}

		// check for scripting
		if( tag.HasParam(wxT("ONCLICK")) )
		{
			skin->m_hasScripts = true;
		}

		// initalize the colours
		if( tag.HasParam(wxT("FGCOLOR")) || tag.HasParam(wxT("BGCOLOR")) )
		{
			currItem->m_colours = new SjSkinColour[SJ_COLOUR_COUNT];
			currItem->m_colours[SJ_COLOUR_NORMAL].SetFromTag(tag);
		}
		else
		{
			currItem->m_colours = skin->m_itemDefColours;
		}

		// parsing the position
		ParsePos(tag, wxT("X"), currItem->m_x);
		ParsePos(tag, wxT("Y"), currItem->m_y);
		ParsePos(tag, wxT("W"), currItem->m_width);
		ParsePos(tag, wxT("H"), currItem->m_height);

		// parsing the image source
		ParseImgSrc(tag, currItem);
		if( currItem->m_image )
		{
			currItem->m_alwaysRedrawBackground = currItem->m_image->HasMaskOrAlpha();
		}

		// inactive?
		currItem->m_usesMouse = ParseLong(tag, wxT("INACTIVE"), 0)? FALSE : TRUE;

		// set the optional tooltip; tooltips are esp. used for buttons switching the layout
		if( tag.HasParam(wxT("TOOLTIP")) )
		{
			currItem->m_itemTooltip = new wxString(tag.GetParam(wxT("TOOLTIP")));
		}

		// parsing the target
		ParseTarget(tag, currItem);
		if( currItem->m_targetId >= IDT_DISPLAY_LINE_FIRST && currItem->m_targetId <= IDT_DISPLAY_LINE_LAST )
		{
			int linesCount = (currItem->m_targetId - IDT_DISPLAY_LINE_FIRST) + 1;
			if( linesCount > m_skinMlParser->m_data->m_currLayout->m_linesCount )
			{
				m_skinMlParser->m_data->m_currLayout->m_linesCount = linesCount;
			}
		}

		// parse the userId
		if( tag.HasParam(wxT("ID")) )
		{
			currItem->m_userId = new wxString(tag.GetParam(wxT("ID")));
		}

		// initialize item and add the item to the layout
		if( !m_skinMlParser->m_data->CreateNAppendItem(tag, currItem) )
		{
			delete currItem;
			return false;
		}

		// insert the item into the hierarchy
		SjSkinItem* parent = m_skinMlParser->m_data->GetLastItem();
		wxASSERT( parent );
		currItem->m_parent = parent;
		parent->m_children.Append(currItem);

		// parse inner
		if( tag.GetChildren() )
		{
			m_skinMlParser->m_data->m_itemStack.Append(currItem);
			ParseInner(tag);
			m_skinMlParser->m_data->m_itemStack.DeleteObject(currItem);
		}

		return true; // ParseInner() called
	}

	return false; // ParseInner() not called
}


bool SjSkinMlTagHandler::ParseColourTarget(const wxHtmlTag& tag, long& colourTargetId)
{
	if( tag.HasParam(wxT("TARGET")) )
	{
		wxString param = tag.GetParam(wxT("TARGET"));
		param.MakeLower();

		long test = m_skinMlParser->m_data->s_colourHash->Lookup(param);
		if( test )
		{
			colourTargetId = test-1;
			return TRUE;
		}

	}

	if( m_skinMlParser->m_data->m_skin->m_debugInfo )
	{
		m_skinMlParser->m_data->LogError(wxT("Colour target not found")/*n/t*/, &tag);
	}

	return FALSE;
}


void SjSkinMlTagHandler::ParseTarget(const wxHtmlTag& tag, SjSkinItem* item)
{
	if( tag.HasParam(wxT("DOUBLECLICKTARGET")) )
	{
		wxString target = tag.GetParam(wxT("DOUBLECLICKTARGET"));
		target.MakeLower();

		unsigned long test = m_skinMlParser->m_data->s_targetHash->Lookup(target);
		if( test )
		{
			item->m_doubleClickTargetId    = test & 0x0000FFFFL;

			wxASSERT(item->m_doubleClickTargetId > 0);
			wxASSERT(item->m_doubleClickTargetId <= IDT_LAST);
		}

		if( target.StartsWith(wxT("layout:")) )
		{
			item->m_doubleClickTargetId = IDT_LAYOUT_FIRST; // corrected later
			item->m_doubleClickTargetName = new wxString(target);
		}
	}

	if( tag.HasParam(wxT("TARGET")) )
	{
		wxString target = tag.GetParam(wxT("TARGET"));
		target.MakeLower();

		unsigned long test = m_skinMlParser->m_data->s_targetHash->Lookup(target);
		if( test )
		{
			item->m_targetId    = test & 0x0000FFFFL;
			item->m_targetFlags = test & 0xFFFF0000L;

			wxASSERT(item->m_targetId > 0);
			wxASSERT(item->m_targetId <= IDT_LAST);
		}

		// the target may also be a layout - however, at the moment we
		// don't know all layouts. remember the target to be parsed later

		if( target.StartsWith(wxT("layout:")) )
		{
			item->m_targetId = IDT_LAYOUT_FIRST; // corrected later
			item->m_targetName = new wxString(target);
		}
	}

	#ifdef SJ_SKIN_USE_BELONGSTO
		if( tag.HasParam(wxT("BELONGSTO")) )
		{
			wxString target = tag.GetParam(wxT("BELONGSTO"));
			target.MakeLower();
			unsigned long test = m_skinMlParser->m_data->s_targetHash->Lookup(target);
			if( test )
			{
				item->m_belongsToId = test & 0x0000FFFFL;
			}
		}
	#endif
}


long SjSkinMlTagHandler::ParseLong(const wxHtmlTag& tag, const wxString& paramName, long def)
{
	long val;

	if( tag.HasParam(paramName) )
	{
		if( tag.GetParam(paramName).ToLong(&val, 10) )
		{
			return val;
		}
		else
		{
			m_skinMlParser->m_data->LogError(wxString::Format(wxT("Invalid value for \"%s\"")/*n/t*/, paramName.c_str()), &tag);
		}
	}

	return def;
}


void SjSkinMlTagHandler::ParseImgSrc(const wxHtmlTag& tag, SjSkinItem* item)
{
	if( tag.HasParam(wxT("SRC")) )
	{
		item->m_image = m_skinMlParser->m_data->LoadSkinImage(tag.GetParam(wxT("SRC")), &tag);
		if( item->m_image )
		{
			item->m_imageIndex = ParseLong(tag, wxT("SRCINDEX"), 0);
		}
	}
}


void SjSkinMlTagHandler::ParsePos(const wxHtmlTag& tag, const wxString& paramName, SjSkinPos& pos)
{
	wxASSERT(m_skinMlParser);

	if( tag.HasParam(paramName) )
	{
		wxString param = tag.GetParam(paramName);
		if( !pos.Parse(param) )
		{
			m_skinMlParser->m_data->LogError(wxString::Format(wxT("Invalid position or size for \"%s\"")/*n/t*/, paramName.Lower().c_str()), &tag);
			pos.Init();
		}
	}
}


/*******************************************************************************
 * SjSkinMlParser
 ******************************************************************************/


SjSkinMlParser::SjSkinMlParser(SjSkinMlParserData* data, long conditions)
{
	if( data )
	{
		// use existing data (for handling <include...>)
		m_data = data;
		m_deleteData = FALSE;
	}
	else
	{
		// create new data
		m_data = new SjSkinMlParserData;
		m_data->m_skin = NULL;
		m_deleteData = TRUE;

		if( m_data->s_targetHash == NULL )
		{
			wxASSERT( IDT_WORKSPACE_GOTO_A+25 == IDT_WORKSPACE_GOTO_Z );
			wxASSERT( IDT_WORKSPACE_GOTO_A+26 == IDT_WORKSPACE_GOTO_0_9 );

			m_data->s_targetHash = m_data->HashInit(s_target2IdTable);
			int i;
			wxString str;
			for( i = IDT_DISPLAY_LINE_FIRST; i <= IDT_DISPLAY_LINE_LAST; i++ )
			{
				m_data->s_targetHash->Insert(wxString::Format(wxT("line%02i"), i-IDT_DISPLAY_LINE_FIRST), i);
			}
		}

		if( m_data->s_colourHash == NULL )
		{
			m_data->s_colourHash  = m_data->HashInit(s_colour2IdTable);
		}

		// init basic condition
		m_data->m_currCond = (conditions&~SJ_OP_OS_MASK) |
						#if defined(__WXMSW__)
		                     SJ_OP_OS_WIN
						#elif defined(__WXMAC__)
		                     SJ_OP_OS_MAC
						#elif defined(__WXGTK__)
		                     SJ_OP_OS_GTK
						#else
							#error "your OS here!"
						#endif
		                     ;

		// add "toggle view" flag if more than one view is allowed
		long availViews = 0;
		if( conditions&SJ_OP_ALBUM_VIEW ) availViews++;
		if( conditions&SJ_OP_COVER_VIEW ) availViews++;
		if( conditions&SJ_OP_LIST_VIEW  ) availViews++;
		SjTools::SetFlag(m_data->m_currCond, SJ_OP_TOGGLE_VIEW__, availViews>1);

		// done
		wxASSERT(m_data->m_currCond!=0);
	}

	// add our tag handler
	SjSkinMlTagHandler* tagHandler = new SjSkinMlTagHandler(this);
	if( tagHandler )
	{
		AddTagHandler(tagHandler);
	}
}


SjSkinMlParser::~SjSkinMlParser()
{
	if( m_deleteData )
	{
		if( m_data->m_skin )
		{
			delete m_data->m_skin;
		}

		delete m_data;
	}
}


SjSkinSkin* SjSkinMlParser::ParseFile(const wxString& url,
                                      bool loadNameOnly)
{
	wxASSERT(m_deleteData==TRUE);

	// In versions before 1.11 silverjuke crashed if an erroneous skin was loaded;
	// the reason for this was an uninitialized m_currTag pointer used in LogError,
	// so we do not save a "current" tag any longer.
	// Anyway, it seems to be a good idea to call CrashPrecaution() before loading
	// a skin because there may be much stuff to go wrong (ZIP, XML, Images..)
	if( !g_tools->CrashPrecaution(wxT("skin"), wxT("ParseFile"), url) )
	{
		if( !SjMainApp::s_cmdLine->Found(wxT("skiperrors")) )
			return NULL;
	}

	// delete previous skin (if any)
	if( m_data->m_skin )
	{
		delete m_data->m_skin;
		m_data->m_skin = NULL;
	}

	// create and setup new skin
	m_data->m_skin = new SjSkinSkin();
	if( !m_data->m_skin )
	{
		return NULL;
	}

	// check if the skin is a ZIP file or a simple path
	m_data->m_skin->m_givenUrl = url;

	m_data->m_loadNameOnly = loadNameOnly;
	wxString xmlCmdFile(SjTools::GetFileNameFromUrl(url, &m_data->m_skin->m_baseDir));

	// read the content
	wxString content = m_data->LoadFile_(xmlCmdFile);

	// parse the read content (return code is also "false" if StopParsing() is called for loadNameOnly - so check this later)
	bool parseRet = wxHtmlParser::Parse(content)? true : false;

	// a skin without name cannot exist
	if( m_data->m_skin->m_name.IsEmpty() )
	{
		wxLogError(wxT("Skin name not set [%s]"), m_data->GetUrl().c_str());
		return NULL;
	}

	if( !loadNameOnly )
	{
		// parsing okay?
		if( !parseRet )
		{
			wxLogError(wxT("Unspecified parsing error [%s]"), m_data->GetUrl().c_str());
			return NULL;
		}

		// any layouts found?
		if( m_data->m_skin->m_layoutList.GetCount() <= 0 )
		{
			wxLogError(wxT("No selectable layouts found [%s]"), m_data->GetUrl().c_str());
			return NULL;
		}

		// set the layout targets,
		// copy item trees if items are used multiple times
		{
			SjSkinLayoutList::Node*  layoutnode;
			SjSkinLayout*            layout;
			wxString                    layoutName;
			SjSkinLayout*            layoutTarget;

			SjSkinItemList::Node*    itemnode;
			SjSkinItem*              item;

			layoutnode = m_data->m_skin->m_layoutList.GetFirst();
			while( layoutnode )
			{
				layout = layoutnode->GetData();
				wxASSERT(layout);

				itemnode = layout->m_itemList.GetFirst();
				while( itemnode )
				{
					item = itemnode->GetData();
					wxASSERT(item);

					if( item->m_targetId == IDT_LAYOUT_FIRST
					 && item->m_targetName
					 && item->m_targetName->StartsWith(wxT("layout:"), &layoutName) )
					{
						layoutTarget = m_data->m_skin->GetLayout(layoutName);
						if( layoutTarget )
						{
							item->m_targetId = IDT_LAYOUT_FIRST + layoutTarget->m_index;
						}
						else
						{
							item->m_targetId = IDT_LAYOUT_FIRST + layout->m_index;
							if( m_data->m_skin->m_debugInfo )
							{
								wxLogError(wxT("Target \"%s\" not found [%s]")/*n/t*/, layoutName.c_str(), m_data->GetUrl().c_str());
							}
						}
					}

					if( item->m_doubleClickTargetId == IDT_LAYOUT_FIRST
					 && item->m_doubleClickTargetName != NULL
					 && item->m_doubleClickTargetName->StartsWith(wxT("layout:"), &layoutName) )
					{
						layoutTarget = m_data->m_skin->GetLayout(layoutName);
						if( layoutTarget )
						{
							item->m_doubleClickTargetId = IDT_LAYOUT_FIRST + layoutTarget->m_index;
						}
						else
						{
							item->m_targetId = IDT_LAYOUT_FIRST + layout->m_index;
							if( m_data->m_skin->m_debugInfo )
							{
								wxLogError(wxT("Target \"%s\" not found [%s]")/*n/t*/, layoutName.c_str(), m_data->GetUrl().c_str());
							}
						}
					}

					itemnode = itemnode->GetNext();
				}

				layoutnode = layoutnode->GetNext();
			}
		}

		// done so far
		if( m_data->m_skin->m_debugInfo )
		{
			wxLogWarning(wxT("Skin opened in debug mode [%s]"), m_data->GetUrl().c_str());
		}
	}

	// done
	SjSkinSkin* skin = m_data->m_skin;
	m_data->m_skin = NULL;
	g_tools->NotCrashed();
	return skin; // the result is now owned by the caller!!
}


void SjSkinMlParser::InitParser(const wxString& source)
{
	wxASSERT(m_data);
	wxASSERT(m_data->m_skin);

	if( m_deleteData )
	{
		m_data->m_includeRecursion  = 0;
		m_data->m_skinCount         = 0;
		m_data->m_currLayout        = NULL;
		m_data->m_itemStack.Clear();
	}

	wxHtmlParser::InitParser(source);
}


void SjSkinMlParser::AddText(const wxChar* txt)
{
}


wxObject* SjSkinMlParser::GetProduct()
{
	return m_data->m_skin;
}


/*******************************************************************************
 * SjSkinPos
 ******************************************************************************/


bool SjSkinPos::Parse(const wxString& str__)
{
	wxString left = str__, right;

	// get rid of all spaces
	left.Replace(wxT(" "), wxT(""));
	left.MakeLower();

	// special sizes?
	if( left.StartsWith(wxT("same")) )
	{
		m_special = SJ_SKINPOS_SAME;
		left = left.Mid(4);
		if( left.IsEmpty() ) return TRUE;
	}
	else if( left.StartsWith(wxT("next")) )
	{
		m_special = SJ_SKINPOS_NEXT;
		left = left.Mid(4);
		if( left.IsEmpty() ) return TRUE;
	}
	else if( left.StartsWith(wxT("opposite")) )
	{
		m_special = SJ_SKINPOS_OPPOSITE;
		m_relTo10000 = 10000;
		left = left.Mid(8);
		if( left.IsEmpty() ) return TRUE;
	}

	// find percent sign
	size_t pos = left.First(wxT("%"));
	if( pos == (size_t)wxNOT_FOUND )
	{
		// not found, set absolute value
		m_relTo10000 = 0;
		return left.ToLong(&m_absPixels, 10);
	}
	else
	{
		// found, set relative value and (optional) absolute value
		right = left.Mid(pos+1);
		if( right.IsEmpty() )
		{
			m_absPixels = 0;
		}
		else if( !right.ToLong(&m_absPixels, 10) )
		{
			return FALSE;
		}

		left = left.Left(pos);
		left += wxT("00");
		pos = left.First(wxT(".")); // we allow a percentage precision of 0.01
		if( pos != (size_t)wxNOT_FOUND )
		{
			left.Remove(pos, 1);
			left = left.Left(pos+2);
		}

		return left.ToLong(&m_relTo10000, 10);
	}
}


long SjSkinPos::CalcAbs(long totalAbsPixels, long prevAbsPos, long prevAbsSize) const
{
	long ret;

	if( m_relTo10000 > 0 && m_relTo10000<5000 )
	{
		ret = (totalAbsPixels-((totalAbsPixels * (10000-m_relTo10000)) / 10000)) + m_absPixels;
	}
	else
	{
		ret = ((totalAbsPixels * m_relTo10000) / 10000) + m_absPixels;
	}

	if( m_special == SJ_SKINPOS_SAME )
	{
		ret += prevAbsPos;
	}
	else if( m_special == SJ_SKINPOS_NEXT )
	{
		ret += prevAbsPos + prevAbsSize;
	}

	return ret;
}


/*******************************************************************************
 * SjSkinImage
 ******************************************************************************/


SjSkinImage::SjSkinImage()
{
	int i;

	for( i = 0; i < SJ_SKIN_SUBIMAGES_MAX; i++ )
	{
		m_subimages[i] = NULL;
		m_subimageWidths[i] = 0;
		m_subimageHeights[i] = 0;
	}

	m_subimageXCount = 0;
	m_subimageYCount = 0;
	m_hasMaskOrAlpha = false;
}


SjSkinImage::~SjSkinImage()
{
	int i;

	for( i = 0; i < SJ_SKIN_SUBIMAGES_MAX; i++ )
	{
		if( m_subimages[i] )
		{
			delete m_subimages[i];
		}
	}
}


wxBitmap* SjSkinImage::GetSubimage(int indexX, int indexY)
{
	if( indexX >= 0
	 && indexX < m_subimageXCount
	 && indexY >= 0
	 && indexY < m_subimageYCount )
	{
		return m_subimages[indexY*m_subimageXCount + indexX]; // may be null!
	}

	return NULL;
}


SjSkinImage* SjSkinMlParserData::LoadSkinImage(const wxString& file, const wxHtmlTag* tagPosition)
{
	wxFSFile*       fsFile = NULL; // must be deleted on return
	SjSkinImage* image;

	wxASSERT(m_skin);

	// check if we have already loaded the given file
	{
		SjSkinImageList::Node* node = m_skin->m_imageList.GetFirst();
		while( node )
		{
			image = node->GetData();
			if( file == image->m_file )
			{
				return image; // yupp, the file is already there
			}
			node = node->GetNext();
		}
	}

	// open the file
	{
		wxFileSystem fs;
		fsFile = fs.OpenFile(m_skin->m_baseDir + file, wxFS_READ|wxFS_SEEKABLE);
		if( !fsFile )
		{
			LogError(wxString::Format(wxT("Cannot open image \"%s\".")/*n/t*/, file.c_str()), tagPosition);
			return NULL;
		}
	}

	// load image
	wxImage wimage(*(fsFile->GetStream()));
	if( !wimage.IsOk() )
	{
		LogError(wxString::Format(wxT("Cannot read image data from \"%s\".")/*n/t*/, file.c_str()), tagPosition);
		delete fsFile;
		return NULL; // cannot read image
	}

	// find out the images beside each other
	int w = wimage.GetWidth();
	int h = wimage.GetHeight();

	unsigned char controlR = wimage.GetRed(0, 0);
	unsigned char controlG = wimage.GetGreen(0, 0);
	unsigned char controlB = wimage.GetBlue(0, 0);

	int             subimageXCount = 0;
	int             controlX[SJ_SKIN_SUBIMAGES_MAX], x;
	unsigned char*  data = wimage.GetData();
	data += w * 3; // seek to second line
	int wBytes = w*3;
	for( x = 0; x < wBytes; x+=3 )
	{
		if( (x==0 || (data[x]==controlR && data[x+1]==controlG && data[x+2]==controlB))
		 && subimageXCount < SJ_SKIN_SUBIMAGES_MAX )
		{
			controlX[subimageXCount++] = x / 3;
		}
	}

	// find out the images below each other
	int  subimageYCount = 0;
	int  controlY[SJ_SKIN_SUBIMAGES_MAX], y;
	data = wimage.GetData();
	data += 3; // seek to second row
	for( y = 0; y < h; y++ )
	{
		if( (y==0 || (data[y*wBytes]==controlR && data[y*wBytes+1]==controlG && data[y*wBytes+2]==controlB))
		        && subimageYCount < SJ_SKIN_SUBIMAGES_MAX )
		{
			controlY[subimageYCount++] = y;
		}
	}

	subimageXCount--;
	subimageYCount--;

	// check borders
	if( subimageXCount < 1
	 || subimageYCount < 1
	 || subimageXCount*subimageYCount>SJ_SKIN_SUBIMAGES_MAX )
	{
		if( subimageXCount < 0 ) subimageXCount = 0;
		if( subimageYCount < 0 ) subimageYCount = 0;
		LogError(wxString::Format(wxT("%i x %i = %i Subimages found in \"%s\". The minimum is 1 and the maximum is %i.")/*n/t*/,
		                          (int)subimageXCount, (int)subimageYCount, (int)subimageXCount*subimageYCount, file.c_str(), (int)SJ_SKIN_SUBIMAGES_MAX), tagPosition);

		delete fsFile;
		return NULL;
	}


	// get image object to hold the subimages
	image = new SjSkinImage();
	if( !image )
	{
		delete fsFile;
		return NULL; // cannot read image
	}

	// get alpha channel / mask colour
	if( wimage.HasAlpha() )
	{
		image->m_hasMaskOrAlpha = true;
	}
	else
	{
		unsigned char maskR = wimage.GetRed(1, 0);
		unsigned char maskG = wimage.GetGreen(1, 0);
		unsigned char maskB = wimage.GetBlue(1, 0);
		if( maskR != controlR
		 || maskG != controlG
		 || maskB != controlB )
		{
			wimage.SetMaskColour(maskR, maskG, maskB);
			image->m_hasMaskOrAlpha = true;
		}
	}

	// get skip colour
	unsigned char skipR = wimage.GetRed(2, 0);
	unsigned char skipG = wimage.GetGreen(2, 0);
	unsigned char skipB = wimage.GetBlue(2, 0);
	bool          skipOk = FALSE;
	if( skipR != controlR
	 || skipG != controlG
	 || skipB != controlB )
	{
		skipOk = TRUE;
	}

	int skippedCount = 0;
	data = wimage.GetData();
	for( y = 0; y < subimageYCount; y++ )
	{
		for( x = 0; x < subimageXCount; x++ )
		{
			wxRect  rect(controlX[x]+1, controlY[y]+1,
			             controlX[x+1]-controlX[x]-1, controlY[y+1]-controlY[y]-1);

			if( rect.width > 0 && rect.height > 0 )
			{
				if( skipOk
				 && data[rect.y*wBytes + rect.x*3    ] == skipR
				 && data[rect.y*wBytes + rect.x*3 + 1] == skipG
				 && data[rect.y*wBytes + rect.x*3 + 2] == skipB )
				{
					skippedCount++;
				}
				else
				{
					wxImage wsubimage = wimage.GetSubImage(rect);
					if( wsubimage.IsOk() )
					{
						if( wimage.HasAlpha() && !wsubimage.HasAlpha() )
						{
							// we have to copy the alpha channel manually
							const unsigned char *srcStart = wimage.GetAlpha(), *srcPtr;
							wsubimage.SetAlpha();
							unsigned char *destStart = wsubimage.GetAlpha(), *destPtr, *destLineEnd;
							wxASSERT( srcStart && destStart );
							long aline;
							for( aline = 0; aline < rect.height; aline++ )
							{
								srcPtr      = srcStart + (rect.y+aline)*w + rect.x;
								destPtr     = destStart + aline*rect.width;
								destLineEnd = destPtr + rect.width;
								while( destPtr < destLineEnd )
								{
									*destPtr = *srcPtr;
									destPtr++;
									srcPtr++;
								}
							}
						}

						wxBitmap* currBitmap = new wxBitmap(wsubimage);
						if( currBitmap
						 && currBitmap->IsOk() )
						{
							image->m_subimages[y*subimageXCount + x] = currBitmap;
						}
					}
				}
			}
			else
			{
				LogError(wxString::Format(wxT("\"%s\": Subimage at %i/%i has a width or a height of null."), file.c_str(), (int)x, (int)y), tagPosition);
			}

			if( y == 0 )
			{
				image->m_subimageWidths[x] = controlX[x+1]-controlX[x]-1;
			}
		}

		image->m_subimageHeights[y] = controlY[y+1]-controlY[y]-1;
	}

	// check memory (we've done many pointer operations above ...)
	#ifdef __WXMSW__
		wxASSERT( _CrtCheckMemory() );
	#endif

	// done
	image->m_subimageXCount = subimageXCount;
	image->m_subimageYCount = subimageYCount;
	image->m_file          = file;
	m_skin->m_imageList.Append(image);
	delete fsFile;

	if( m_skin->m_debugInfo )
	{
		wxLogInfo(wxT("%s loaded with %ix%i subimage(s), %i skipped subimage(s), %i mask or alpha channel(s) [%s]")/*n/t*/,
		          file.c_str(), (int)subimageXCount, (int)subimageYCount, (int)skippedCount, (int)image->m_hasMaskOrAlpha, GetUrl().c_str());
	}

	return image;
}


/*******************************************************************************
 * SjSkinLayout
 ******************************************************************************/


SjSkinLayout::SjSkinLayout(SjSkinSkin* skin, const wxString& name, int index)
{
	m_skin          = skin;
	m_index         = index;
	m_name          = name;
	m_hasWorkspace  = false;
	m_hasInput      = false;
	m_linesCount    = 0;
	m_alwaysOnTop   = false;

	m_minSize.x     = 32;
	m_minSize.y     = 32;
	m_maxSize.x     = 32000;
	m_maxSize.y     = 32000;
}


SjSkinLayout::~SjSkinLayout()
{
	SjSkinItemList::Node* node = m_itemList.GetFirst();
	while( node )
	{
		SjSkinItem* item = node->GetData();

		wxASSERT(item);
		wxASSERT(item->m_colours);

		if( item->m_colours != m_skin->m_itemDefColours )
		{
			delete [] item->m_colours;
		}

		delete item;
		delete node;
		node = m_itemList.GetFirst();
	}
}


/*******************************************************************************
 * SjSkinSkin
 ******************************************************************************/


SjSkinSkin::SjSkinSkin()
{
	#if SJ_USE_SCRIPTS
	m_see               = NULL;
	#endif
	m_hasScripts        = FALSE;
	m_debugInfo         = FALSE;
	m_debugOutline      = FALSE;
	m_tooltipColoursSet = FALSE;
}


SjSkinSkin::~SjSkinSkin()
{
	// delete the scripting engine
	#if SJ_USE_SCRIPTS
	delete m_see;
	#endif

	// delete all layouts
	SjSkinLayoutList::Node* layoutnode = m_layoutList.GetFirst();
	while( layoutnode )
	{
		wxASSERT(layoutnode->GetData());
		delete layoutnode->GetData();
		delete layoutnode;
		layoutnode = m_layoutList.GetFirst();
	}

	// delete all images, this should be done AFTER deleting the
	// layouts as the layouts and items have references to the images
	SjSkinImageList::Node* imagenode = m_imageList.GetFirst();
	while( imagenode )
	{
		wxASSERT(imagenode->GetData());
		delete imagenode->GetData();
		delete imagenode;
		imagenode = m_imageList.GetFirst();
	}
}


SjSkinLayout* SjSkinSkin::GetLayout(int index)
{
	SjSkinLayoutList::Node* node = m_layoutList.Item(index);
	if( node )
	{
		SjSkinLayout* layout = node->GetData();
		wxASSERT(layout);
		wxASSERT(layout->m_index == index);
		return layout;
	}

	return NULL;
}


SjSkinLayout* SjSkinSkin::GetLayout(const wxString& name)
{
	SjSkinLayoutList::Node* node = m_layoutList.GetFirst();
	while( node )
	{
		SjSkinLayout* layout = node->GetData();
		wxASSERT(layout);

		if( name.CmpNoCase(layout->m_name)==0 )
		{
			return layout;
		}

		node = node->GetNext();
	}

	return NULL;
}


void SjSkinSkin::ConnectToSkinWindow(SjSkinWindow* skinWindow)
{
	SjSkinLayoutList::Node*  layoutnode;
	SjSkinLayout*            layout;

	SjSkinItemList::Node*    itemnode;
	SjSkinItem*              item;

	layoutnode = m_layoutList.GetFirst();
	while( layoutnode )
	{
		layout = layoutnode->GetData();
		wxASSERT(layout);

		itemnode = layout->m_itemList.GetFirst();
		while( itemnode )
		{
			item = itemnode->GetData();
			wxASSERT(item);

			item->m_skinWindow = skinWindow;

			itemnode = itemnode->GetNext();
		}

		layoutnode = layoutnode->GetNext();
	}
}

