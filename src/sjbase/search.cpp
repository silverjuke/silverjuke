/*********************************************************************************
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
 * File:    searchsj.cpp
 * Authors: Björn Petersen
 * Purpose: searching
 *
 ******************************************************************************/


#include <sjbase/base.h>
#include <sjbase/search.h>

#include <wx/arrimpl.cpp> // sic!
WX_DEFINE_OBJARRAY(SjArrayRule);


/*******************************************************************************
 * SjRule - Basic Operations
 ******************************************************************************/


SjRule::SjRule(SjField field, SjFieldOp op, const wxString& value0, const wxString& value1, SjUnit unit)
{
	m_field     = field;
	m_op        = op;
	m_value[0]  = value0;
	m_value[1]  = value1;
	m_unit      = unit;
}


void SjRule::Clear()
{
	m_field     = SJ_FIELD_DEFAULT;
	m_op        = SJ_FIELDOP_DEFAULT;
	m_value[0].Empty();
	m_value[1].Empty();
	m_unit      = SJ_UNIT_DEFAULT;
}


void SjRule::CopyFrom(const SjRule& o)
{
	m_field = o.m_field;
	m_op    = o.m_op;
	m_value[0]= o.m_value[0];
	m_value[1]= o.m_value[1];
	m_unit  = o.m_unit;
}


bool SjRule::IsEqualTo(const SjRule& o) const
{
	if( m_field  != o.m_field
	 || m_op     != o.m_op
	 || m_value[0] != o.m_value[0]
	 || m_value[1] != o.m_value[1]
	 || m_unit   != o.m_unit )
	{
		return FALSE;
	}

	return TRUE;
}


void SjRule::Serialize(SjStringSerializer& ser) const
{
	ser.AddLong     (5); // fields following, for future use
	ser.AddLong     (m_field);
	ser.AddLong     (m_op);
	ser.AddString   (m_value[0]);
	ser.AddString   (m_value[1]);
	ser.AddLong     (m_unit);
}


bool SjRule::Unserialize(SjStringSerializer& ser)
{
	long        fieldsFollowing = ser.GetLong();
	SjField     field           = (SjField)ser.GetLong();
	SjFieldOp   op              = (SjFieldOp)ser.GetLong();
	wxString    value0          = ser.GetString();
	wxString    value1          = ser.GetString();
	SjUnit      unit            = (SjUnit)ser.GetLong();

	if( fieldsFollowing != 5
	 || field >= SJ_FIELD_COUNT
	 || op >= SJ_FIELDOP_COUNT
	 || unit >= SJ_UNIT_COUNT
	 || ser.HasErrors() )
	{
		return FALSE;
	}

	m_field = field;
	m_op    = op;
	m_value[0]= value0;
	m_value[1]= value1;
	m_unit  = unit;

	return TRUE;
}


SjFieldType SjRule::GetFieldType(SjField field)
{
	switch( field )
	{
		case SJ_FIELD_URL:
		case SJ_FIELD_TRACKNAME:
		case SJ_FIELD_LEADARTISTNAME:
		case SJ_FIELD_ORGARTISTNAME:
		case SJ_FIELD_COMPOSERNAME:
		case SJ_FIELD_ALBUMNAME:
		case SJ_FIELD_GENRENAME:
		case SJ_FIELD_GROUPNAME:
		case SJ_FIELD_COMMENT:
		case SJ_PSEUDOFIELD_SQL:
		case SJ_PSEUDOFIELD_TRACKARTISTALBUM:
		case SJ_PSEUDOFIELD_FILETYPE:
			return SJ_FIELDTYPE_STRING;

		case SJ_FIELD_TIMEADDED:
		case SJ_FIELD_TIMEMODIFIED:
		case SJ_FIELD_LASTPLAYED:
			return SJ_FIELDTYPE_DATE;

		case SJ_PSEUDOFIELD_LIMIT:
		case SJ_PSEUDOFIELD_QUEUEPOS:
		default:
			return SJ_FIELDTYPE_NUMBER;

		case SJ_PSEUDOFIELD_INCLUDE:
		case SJ_PSEUDOFIELD_EXCLUDE:
			return SJ_FIELDTYPE_LIST;

		case SJ_FIELD_COUNT:
			return SJ_FIELDTYPE_COUNT;
	}
}


bool SjRule::IsValidFieldOp(SjField field, SjFieldOp op)
{
	SjFieldType fieldType = GetFieldType(field);

	if( field == SJ_FIELD_AUTOVOL )
	{
		return (op==SJ_FIELDOP_IS_SET || op==SJ_FIELDOP_IS_UNSET);
	}
	else
	{
		switch( op )
		{
			case SJ_FIELDOP_IS_EQUAL_TO:
			case SJ_FIELDOP_IS_UNEQUAL_TO:
			case SJ_FIELDOP_IS_SET:
			case SJ_FIELDOP_IS_UNSET:
				return TRUE;

			case SJ_FIELDOP_CONTAINS:
			case SJ_FIELDOP_DOES_NOT_CONTAIN:
			case SJ_FIELDOP_STARTS_WITH:
			case SJ_FIELDOP_DOES_NOT_START_WITH:
			case SJ_FIELDOP_ENDS_WITH:
			case SJ_FIELDOP_DOES_NOT_END_WITH:
			case SJ_FIELDOP_IS_SIMELAR_TO:
			case SJ_FIELDOP_STARTS_SIMELAR_TO:
				return (fieldType==SJ_FIELDTYPE_STRING);

			case SJ_FIELDOP_IS_GREATER_THAN:
			case SJ_FIELDOP_IS_LESS_THAN:
			case SJ_FIELDOP_IS_IN_RANGE:
			case SJ_FIELDOP_IS_NOT_IN_RANGE:
				return (fieldType==SJ_FIELDTYPE_NUMBER || fieldType==SJ_FIELDTYPE_DATE);

			case SJ_FIELDOP_IS_GREATER_OR_EQUAL:
			case SJ_FIELDOP_IS_LESS_OR_EQUAL:
				return (fieldType==SJ_FIELDTYPE_NUMBER);

			case SJ_FIELDOP_IS_IN_THE_LAST:
			case SJ_FIELDOP_IS_NOT_IN_THE_LAST:
				return (fieldType==SJ_FIELDTYPE_DATE);

			default:
				return FALSE;
		}
	}
}


wxString SjRule::GetFieldDescr(SjField field)
{
	switch( field )
	{
		case SJ_FIELD_TRACKNAME:        return _("Title");
		case SJ_FIELD_TRACKNR:          return _("Track number");
		case SJ_FIELD_TRACKCOUNT:       return _("Track count");
		case SJ_FIELD_DISKNR:           return _("Disk number");
		case SJ_FIELD_DISKCOUNT:        return _("Disk count");
		case SJ_FIELD_LEADARTISTNAME:   return _("Artist");
		case SJ_FIELD_ORGARTISTNAME:    return _("Original artist");
		case SJ_FIELD_COMPOSERNAME:     return _("Composer");
		case SJ_FIELD_ALBUMNAME:        return _("Album");
		case SJ_FIELD_GENRENAME:        return _("Genre");
		case SJ_FIELD_GROUPNAME:        return _("Group");
		case SJ_FIELD_COMMENT:          return _("Comment");
		case SJ_FIELD_BEATSPERMINUTE:   return _("BPM");
		case SJ_FIELD_RATING:           return _("Rating");
		case SJ_FIELD_YEAR:             return _("Year");
		case SJ_FIELD_PLAYTIME:         return _("Duration");
		case SJ_FIELD_AUTOVOL:          return _("Volume");
		case SJ_FIELD_CHANNELS:         return _("Channels");
		case SJ_FIELD_SAMPLERATE:       return _("Samplerate");
		case SJ_FIELD_BITRATE:          return _("Bitrate");
		case SJ_FIELD_URL:              return _("File name");
		case SJ_FIELD_DATABYTES:        return _("File size");
		case SJ_FIELD_TIMEADDED:        return _("Date added");
		case SJ_FIELD_TIMEMODIFIED:     return _("Date modified");
		case SJ_FIELD_LASTPLAYED:       return _("Last played");
		case SJ_FIELD_TIMESPLAYED:      return _("Play count");
		case SJ_PSEUDOFIELD_SQL:        return _("SQL expression");
		case SJ_PSEUDOFIELD_LIMIT:      return _("Limit result to");
		case SJ_PSEUDOFIELD_TRACKARTISTALBUM: return _("Title/artist/album");
		case SJ_PSEUDOFIELD_FILETYPE:   return _("File type");
		case SJ_PSEUDOFIELD_QUEUEPOS:   return _("Queue position");
		case SJ_PSEUDOFIELD_INCLUDE:    return _("Include tracks");
		case SJ_PSEUDOFIELD_EXCLUDE:    return _("Exclude tracks");
		case SJ_PSEUDOFIELD_RANDOM:     return _("Random");
		default:                        return wxT("***");
	}
}


wxString SjRule::GetFieldDbName(SjField field)
{
	switch( field )
	{
		case SJ_FIELD_TRACKNAME:        return wxT("trackname");
		case SJ_FIELD_TRACKNR:          return wxT("tracknr");
		case SJ_FIELD_TRACKCOUNT:       return wxT("trackcount");
		case SJ_FIELD_DISKNR:           return wxT("disknr");
		case SJ_FIELD_DISKCOUNT:        return wxT("diskcount");
		case SJ_FIELD_LEADARTISTNAME:   return wxT("leadartistname");
		case SJ_FIELD_ORGARTISTNAME:    return wxT("orgartistname");
		case SJ_FIELD_COMPOSERNAME:     return wxT("composername");
		case SJ_FIELD_ALBUMNAME:        return wxT("albumname");
		case SJ_FIELD_GENRENAME:        return wxT("genrename");
		case SJ_FIELD_GROUPNAME:        return wxT("groupname");
		case SJ_FIELD_COMMENT:          return wxT("comment");
		case SJ_FIELD_BEATSPERMINUTE:   return wxT("beatsperminute");
		case SJ_FIELD_RATING:           return wxT("rating");
		case SJ_FIELD_YEAR:             return wxT("year");
		case SJ_FIELD_PLAYTIME:         return wxT("playtimems");
		case SJ_FIELD_AUTOVOL:          return wxT("autovol");
		case SJ_FIELD_CHANNELS:         return wxT("channels");
		case SJ_FIELD_SAMPLERATE:       return wxT("samplerate");
		case SJ_FIELD_BITRATE:          return wxT("bitrate");
		case SJ_FIELD_URL:              return wxT("url");
		case SJ_PSEUDOFIELD_FILETYPE:   return wxT("FILETYPE(url)");
		case SJ_PSEUDOFIELD_QUEUEPOS:   return wxT("QUEUEPOS(url)");
		case SJ_FIELD_DATABYTES:        return wxT("databytes");
		case SJ_FIELD_TIMEADDED:        return wxT("timeadded");
		case SJ_FIELD_TIMEMODIFIED:     return wxT("timemodified");
		case SJ_FIELD_LASTPLAYED:       return wxT("lastplayed");
		case SJ_FIELD_TIMESPLAYED:      return wxT("timesplayed");
		default:                        return wxT("");
	}
}


/*******************************************************************************
 * SjRule::IsOk()
 ******************************************************************************/


wxString SjRule::IsValueOk(const wxString& valueStr__, bool* addCaseWarning) const
{
	wxString    valueStr(valueStr__);
	long        fieldType = GetFieldType(m_field);
	wxString    error;
	bool        errorPleaseEnterAValue = FALSE;

	valueStr.Trim(TRUE);
	valueStr.Trim(FALSE);

	if( fieldType == SJ_FIELDTYPE_LIST )
	{
		if( GetInclExclCount() == 0 )
		{
			error = wxString::Format(_("Press \"%s\" in the main window"),
			                         m_field==SJ_PSEUDOFIELD_INCLUDE? _("Insert") : _("Del"));
		}
	}
	else if( fieldType == SJ_FIELDTYPE_DATE )
	{
		if( m_op == SJ_FIELDOP_IS_IN_THE_LAST || m_op == SJ_FIELDOP_IS_NOT_IN_THE_LAST )
		{
			if( !SjTools::ParseNumber(valueStr) )
			{
				errorPleaseEnterAValue = TRUE;
			}
		}
		else
		{
			if( !SjTools::ParseDate_(valueStr) )
			{
				error = _("Please enter the date as \"dd.mm.yyyy\", you can also add \"hh:mm\" for the time.");
			}
		}
	}
	else if( fieldType == SJ_FIELDTYPE_NUMBER )
	{
		// first, check fields that accepts characters beside 0-9
		if( m_field == SJ_FIELD_YEAR )
		{
			if( !SjTools::ParseYear(valueStr) )
			{
				errorPleaseEnterAValue = TRUE;
			}
		}
		else if( m_field == SJ_FIELD_PLAYTIME
		         && m_unit == SJ_UNIT_MINUTES )
		{
			if( !SjTools::ParseTime(valueStr) )
			{
				errorPleaseEnterAValue = TRUE;
			}
		}
		else if( m_field == SJ_PSEUDOFIELD_QUEUEPOS )
		{
			if( valueStr != SJ_QUEUEPOS_CURR_STR
			 && !SjTools::ParseNumber(valueStr) )
			{
				errorPleaseEnterAValue = TRUE;
			}
		}
		// second, check if it is a number
		else if( !SjTools::ParseNumber(valueStr) )
		{
			errorPleaseEnterAValue = TRUE;
		}
	}
	else if( valueStr.IsEmpty() )
	{
		error = _("Please enter the text.");
	}
	else if( addCaseWarning
	      && m_field != SJ_PSEUDOFIELD_SQL
	      && (m_op==SJ_FIELDOP_IS_EQUAL_TO || m_op==SJ_FIELDOP_IS_UNEQUAL_TO)
	      && (valueStr.Lower()==valueStr || valueStr.Upper()==valueStr) )
	{
		*addCaseWarning = TRUE;
	}

	if( errorPleaseEnterAValue )
	{
		error = valueStr.IsEmpty()? _("Please enter a value.") : _("Please enter a valid value.");
	}

	return error;
}


bool SjRule::IsOk(wxString& error1, wxString& error2, bool* addCaseWarning) const
{
	error1.Clear();
	error2.Clear();

	if( m_op != SJ_FIELDOP_IS_SET && m_op != SJ_FIELDOP_IS_UNSET )
	{
		error1 = IsValueOk(m_value[0], addCaseWarning);

		if( m_op == SJ_FIELDOP_IS_IN_RANGE || m_op == SJ_FIELDOP_IS_NOT_IN_RANGE )
		{
			error2 = IsValueOk(m_value[1], addCaseWarning);
		}
	}

	return (error1.IsEmpty() && error2.IsEmpty());
}


/*******************************************************************************
 * SjRule::*InclExcl*()
 ******************************************************************************/


long SjRule::GetInclExclCount() const
{
	long count = m_value[0].Freq(',')-1;
	if( count < 0
	 || m_value[0].Len() < 3
	 || m_value[0].GetChar(0) != ','
	 || m_value[0].Last() != ',' )
	{
		count = 0;
	}
	return count;
}


wxString SjRule::GetInclExclDescr() const
{
	long count = GetInclExclCount();

	if( count == 0 )
	{
		return wxString::Format(_("Press \"%s\" in the main window"),
		                        m_field==SJ_PSEUDOFIELD_INCLUDE? _("Insert") : _("Del"));
	}
	else
	{
		wxString idsStr = m_value[0].BeforeLast(','), url, trackName, leadArtistName, albumName, descr;
		long id, idsPrinted = 0;
		while( idsPrinted <= 3 && !idsStr.IsEmpty() )
		{
			idsStr.AfterLast(',').ToLong(&id, 10);
			idsStr = idsStr.BeforeLast(',');
			url = g_mainFrame->m_libraryModule->GetUrl(id);
			if( !url.IsEmpty() )
			{
				g_mainFrame->m_columnMixer.GetQuickInfo(url, trackName, leadArtistName, albumName, id);
				if( !descr.IsEmpty() ) descr += wxT(", ");
				descr += trackName;
			}
			idsPrinted++;
		}

		if( count == 1 )
		{
			return descr;
		}
		else
		{
			if( idsPrinted != count ) descr += wxT(", ...");
			return wxString::Format(_("%s tracks"), SjTools::FormatNumber(count).c_str()) + wxT(": ") + descr;
		}
	}
}


void SjRule::AddToInclExcl(const wxString& idsToAdd)
{
	if( m_value[0].IsEmpty() )
	{
		m_value[0] = idsToAdd;
	}
	else
	{
		m_value[0] += idsToAdd.Mid(1);//skip first comma as it is the last char in m_value[0]
	}
}


void SjRule::AddToInclExcl(SjLLHash* ids)
{
	wxString strToAdd;
	long id;
	SjHashIterator iterator;
	while( ids->Iterate(iterator, &id) )
	{
		strToAdd.Printf(wxT("%i"), (int)id);
		if( m_value[0].Find(wxT(",")+strToAdd+wxT(",")) == -1 )
		{
			if( m_value[0].IsEmpty() ) m_value[0] = wxT(",");
			m_value[0].Append(strToAdd+wxT(","));
		}
	}
}



void SjRule::RemoveFromInclExcl(SjLLHash* ids)
{
	wxString strToRemove;
	long id;
	SjHashIterator iterator;
	while( ids->Iterate(iterator, &id) )
	{
		strToRemove.Printf(wxT(",%i,"), (int)id);
		m_value[0].Replace(strToRemove, wxT(","));
	}

	if( m_value[0].Len()==1 )
	{
		m_value[0].Clear();
	}
}


/*******************************************************************************
 * SjRule::GetAsSql()
 ******************************************************************************/


wxString SjRule::GetAsSql(SjField field, SjFieldOp op, bool forceNumberSet)
{
	switch( op )
	{
		case SJ_FIELDOP_IS_SET:             return field==SJ_PSEUDOFIELD_QUEUEPOS? wxT("$f IS NOT NULL") : wxT("$f!=$v");
		case SJ_FIELDOP_IS_UNSET:           return field==SJ_PSEUDOFIELD_QUEUEPOS? wxT("$f IS NULL") : wxT("$f=$v");

		case SJ_FIELDOP_IS_EQUAL_TO:        return wxT("$f=$v");
		case SJ_FIELDOP_IS_UNEQUAL_TO:      return wxT("$f!=$v");

		case SJ_FIELDOP_IS_GREATER_THAN:    return /* forceNumberSet? "($f!=0 AND $f>$v)" : not needed as we always use positive numbers : */ wxT("$f>$v");
		case SJ_FIELDOP_IS_GREATER_OR_EQUAL:return /* forceNumberSet? "($f!=0 AND $f>=$v)" :*/ wxT("$f>=$v");

		case SJ_FIELDOP_IS_LESS_THAN:       return forceNumberSet? wxT("($f!=0 AND $f<$v)") : wxT("$f<$v");
		case SJ_FIELDOP_IS_LESS_OR_EQUAL:   return forceNumberSet? wxT("($f!=0 AND $f<=$v)") : wxT("$f<=$v");

		case SJ_FIELDOP_CONTAINS:           return wxT("$f LIKE '%$vu%'");
		case SJ_FIELDOP_DOES_NOT_CONTAIN:   return wxT("NOT($f LIKE '%$vu%')");

		case SJ_FIELDOP_STARTS_WITH:        return wxT("$f LIKE '$vu%'");
		case SJ_FIELDOP_DOES_NOT_START_WITH:return wxT("NOT($f LIKE '$vu%')");

		case SJ_FIELDOP_ENDS_WITH:          return wxT("$f LIKE '%$vu'");
		case SJ_FIELDOP_DOES_NOT_END_WITH:  return wxT("NOT($f LIKE '%$vu')");

		case SJ_FIELDOP_IS_SIMELAR_TO:      return wxT("LEVENSTHEIN($f, $v)==1");
		                                  //return "SOUNDEX($f)=SOUNDEX('$vu')";

		case SJ_FIELDOP_STARTS_SIMELAR_TO:  return wxT("LEVENSTHEIN(SUBSTR($f, 1, $vl),$v)==1");
		                                  //return "SOUNDEX(SUBSTR($f,1,$vl))=SOUNDEX('$vu')";

		default:                            return wxT("***");
	}
}


wxString SjRule::GetAsSql(const wxString& value__, SjField field, SjFieldOp op, SjUnit unit,
                          bool forceSet, bool recursiveCall)
{
	wxString    value(value__), unquotedValue;
	long        fieldType = GetFieldType(field);
	bool        forceNumberSet = FALSE;

	//
	// get correct - unquoted - value based on the field type and on the basic operation
	//

	if( field == SJ_PSEUDOFIELD_SQL )
	{

		//
		// user defined SQL statement -- nothing more to do
		//

		return wxT("(") + value + wxT(")");
	}
	else if( op == SJ_FIELDOP_IS_SET || op == SJ_FIELDOP_IS_UNSET )
	{

		//
		// set/unset
		//

		value = fieldType==SJ_FIELDTYPE_STRING? wxT("''") : wxT("0");
	}
	else if( field == SJ_FIELD_DATABYTES )
	{

		//
		// convert KB/MB/GB to bytes
		//

		long longValue;
		SjTools::ParseNumber(value, &longValue);
		     if( unit == SJ_UNIT_KB ) { longValue *= 1024; }
		else if( unit == SJ_UNIT_MB ) { longValue *= 1024*1024; }
		else if( unit == SJ_UNIT_GB ) { longValue *= 1024*1024*1024; }
		value = wxString::Format(wxT("%i"), (int)longValue);
		if( longValue != 0 ) { forceNumberSet = TRUE; }
	}
	else if( field == SJ_FIELD_YEAR )
	{

		//
		// convert given year
		//

		long longValue;
		SjTools::ParseYear(value, &longValue);
		value = wxString::Format(wxT("%i"), (int)longValue);
		if( longValue != 0 ) { forceNumberSet = TRUE; }
	}
	else if( field == SJ_FIELD_PLAYTIME )
	{

		//
		// convert given playing time to milliseconds
		//

		long longValue;
		if( unit == SJ_UNIT_MINUTES )
		{
			SjTools::ParseTime(value, &longValue);
		}
		else
		{
			SjTools::ParseNumber(value, &longValue);
		}
		longValue *= 1000;
		value = wxString::Format(wxT("%i"), (int)longValue);
		if( longValue != 0 ) { forceNumberSet = TRUE; }
	}
	else if(  fieldType == SJ_FIELDTYPE_DATE
	          && (op==SJ_FIELDOP_IS_IN_THE_LAST || op==SJ_FIELDOP_IS_NOT_IN_THE_LAST) )
	{

		//
		// convert relative date value
		//

		long longValue;
		SjTools::ParseNumber(value, &longValue);
		op = op==SJ_FIELDOP_IS_IN_THE_LAST? SJ_FIELDOP_IS_GREATER_OR_EQUAL : SJ_FIELDOP_IS_LESS_THAN;
		switch( unit )
		{
			case SJ_UNIT_MINUTES:   return GetAsSql(wxString::Format(wxT("now -%i"), (int)longValue), field, op, unit, forceSet, TRUE);
			case SJ_UNIT_HOURS:     return GetAsSql(wxString::Format(wxT("now -%i"), (int)longValue*60), field, op, unit, forceSet, TRUE);
			default:                return GetAsSql(wxString::Format(wxT("today -%i"), (int)longValue), field, op, unit, forceSet, TRUE);
		}

	}
	else if( fieldType == SJ_FIELDTYPE_DATE )
	{

		//
		// convert absolute date/time value to seconds
		//

		bool timeSet;
		if( SjTools::ParseDate_(value, !recursiveCall, NULL, &timeSet) )
		{
			if( timeSet )
			{
				;
			}
			else if( op == SJ_FIELDOP_IS_EQUAL_TO )
			{
				// convert a concrete date to a timespan
				return    wxT("(") + GetAsSql(value, field, SJ_FIELDOP_IS_GREATER_OR_EQUAL, unit, FALSE, TRUE)
				          + wxT(" AND ") + GetAsSql(value+wxT(" +1"), field, SJ_FIELDOP_IS_LESS_THAN, unit, FALSE, TRUE)
				          +     wxT(")");
			}
			else if( op == SJ_FIELDOP_IS_UNEQUAL_TO )
			{
				// convert a concrete date to a timespan
				return    wxT("(") + GetAsSql(value, field, SJ_FIELDOP_IS_LESS_THAN, unit, FALSE, TRUE)
				          + wxT("  OR ") + GetAsSql(value+wxT(" +1"), field, SJ_FIELDOP_IS_GREATER_OR_EQUAL, unit, FALSE, TRUE)
				          +     wxT(")");
			}
			else if( op == SJ_FIELDOP_IS_LESS_OR_EQUAL || op == SJ_FIELDOP_IS_GREATER_THAN )
			{
				// move time to end of the day for "less or equal" or for "greater than"
				value += wxT(" 23:59:59");
			}

			forceNumberSet = TRUE;

			wxString oldValue = value;
			value = wxString::Format(wxT("TIMESTAMP('%s')"), oldValue.c_str());
		}
	}
	else if( field == SJ_PSEUDOFIELD_QUEUEPOS
	      && value == SJ_QUEUEPOS_CURR_STR )
	{

		//
		// current queue position - nothing to convert
		//

		;
	}
	else if( fieldType == SJ_FIELDTYPE_NUMBER )
	{

		//
		// convert a number to an exiting number
		//

		long longValue;
		SjTools::ParseNumber(value, &longValue);
		value = wxString::Format(wxT("%i"), (int)longValue);
		if( longValue != 0 && field != SJ_FIELD_TIMESPLAYED ) { forceNumberSet = TRUE; }
	}
	else
	{

		//
		// add quoted to string, preserve an unqoted copy
		// the second wxSqlt::QParam(value) was added doe to the report on http://www.silverjuke.net/forum/topic-2546.html
		//

		unquotedValue = wxSqlt::QParam(value);
		value = wxT("'") + wxSqlt::QParam(value) + wxT("'");
	}


	//
	// apply operator
	//

	// get the correct operation string
	wxString retSql(GetAsSql(field, op, (forceSet&&forceNumberSet)));

	// first, replace the fields and the length of the unquoted value un the operation string...
	retSql.Replace(wxT("$fl"), wxString::Format(wxT("%i"), (int)GetFieldDbName(field).Len()));
	retSql.Replace(wxT("$f"), GetFieldDbName(field));

	// ...after that, replace the unquoted or quoted value in the operation string
	// (this should be last, as the value may contain eg. "$f")
	retSql.Replace(wxT("$vl"), wxString::Format(wxT("%i"), (int)unquotedValue.Len()));
	if( retSql.Find(wxT("$vu")) != -1 )
	{
		retSql.Replace(wxT("$vu"), unquotedValue);
	}
	else
	{
		retSql.Replace(wxT("$v"), value);
	}

	return retSql;
}


wxString SjRule::GetAsSql() const
{
	if( m_field == SJ_PSEUDOFIELD_INCLUDE || m_field == SJ_PSEUDOFIELD_EXCLUDE )
	{
		wxString ret;
		long     count = GetInclExclCount();
		if( count == 0 )
		{
			ret = m_field == SJ_PSEUDOFIELD_INCLUDE? wxT("(0)") : wxT("(1)");
		}
		else
		{
			ret = m_field == SJ_PSEUDOFIELD_INCLUDE? wxT("id IN (") : wxT("id NOT IN (");
			ret += m_value[0].Mid(1, m_value[0].Len()-2);
			ret += wxT(")");
		}
		return ret;
	}
	else if( m_field == SJ_PSEUDOFIELD_LIMIT )
	{
		return wxT("");
	}
	else if( m_field == SJ_PSEUDOFIELD_TRACKARTISTALBUM )
	{
		return    wxT("(") + GetAsSql(m_value[0], SJ_FIELD_TRACKNAME,       m_op, m_unit, FALSE)
		          + wxT(" OR ") + GetAsSql(m_value[0], SJ_FIELD_LEADARTISTNAME, m_op, m_unit, FALSE)
		          + wxT(" OR ") + GetAsSql(m_value[0], SJ_FIELD_ALBUMNAME,       m_op, m_unit, FALSE)
		          +    wxT(")");
	}
	else
	{
		switch( m_op )
		{
			case SJ_FIELDOP_IS_NOT_IN_RANGE:
				return    wxT("(") + GetAsSql(m_value[0], m_field, SJ_FIELDOP_IS_LESS_THAN, m_unit, FALSE)
				          + wxT(" OR ") + GetAsSql(m_value[1], m_field, SJ_FIELDOP_IS_GREATER_THAN, m_unit, FALSE)
				          + wxT(")");

			case SJ_FIELDOP_IS_IN_RANGE:
				return    wxT("(") + GetAsSql(m_value[0], m_field, SJ_FIELDOP_IS_GREATER_OR_EQUAL, m_unit, FALSE)
				          + wxT(" AND ") + GetAsSql(m_value[1], m_field, SJ_FIELDOP_IS_LESS_OR_EQUAL, m_unit, FALSE)
				          +     wxT(")");

			default:
				return  GetAsSql(m_value[0], m_field, m_op, m_unit, TRUE/*force that the value is set, if needed*/);
		}
	}
}


/*******************************************************************************
 * SjRule::Convert()
 ******************************************************************************/


bool SjRule::Convert(SjField newField)
{
	// Convert the rule field.
	// This may modify the operator, the values and the unit.

	bool largerModifications = FALSE;
	long newFieldType = GetFieldType(newField);
	long _m_FieldType = GetFieldType(m_field);
	bool initUnitRequired = FALSE;
	bool initOperatorRequired = FALSE;

	if( newField != m_field )
	{
		if( newField == SJ_PSEUDOFIELD_SQL )
		{
			// if the new field is an SQL expression,
			// get the old field as SQL - done
			wxString sql= GetAsSql();
			m_op        = SJ_FIELDOP_IS_EQUAL_TO;
			m_value[0]  = sql;
			m_value[1].Clear();
			m_unit      = SJ_UNIT_NA;
			largerModifications = TRUE;
		}
		else if( newField == SJ_FIELD_RATING )
		{
			// init rating, the rating cannot be converted sensfully from anothe field
			m_value[0] = wxT("5");
			m_value[1].Clear();
			initUnitRequired = TRUE;
			initOperatorRequired = TRUE;
		}
		else if(  m_field == SJ_PSEUDOFIELD_SQL
		      ||  newField == SJ_FIELD_GENRENAME
		      ||  newField == SJ_FIELD_GROUPNAME
		      ||  newField == SJ_PSEUDOFIELD_FILETYPE
		      ||  newField == SJ_PSEUDOFIELD_QUEUEPOS
		      ||  newFieldType != _m_FieldType
		      || (newFieldType == SJ_FIELDTYPE_NUMBER && _m_FieldType == SJ_FIELDTYPE_NUMBER) )
		{
			// other fields that can never be converted sensefully
			// to other fields
			m_value[0].Clear();
			m_value[1].Clear();
			initUnitRequired = TRUE;
			initOperatorRequired = TRUE;
		}

		// init the operator, if required
		if( initOperatorRequired )
		{
			if( newFieldType == SJ_FIELDTYPE_STRING )
			{
				if( newField == SJ_FIELD_GENRENAME
				 || newField == SJ_FIELD_GROUPNAME
				 || newField == SJ_PSEUDOFIELD_FILETYPE )
				{
					m_op = SJ_FIELDOP_IS_EQUAL_TO;
				}
				else
				{
					m_op = SJ_FIELDOP_CONTAINS;
				}
			}
			else if( newFieldType == SJ_FIELDTYPE_DATE )
			{
				m_op = SJ_FIELDOP_IS_IN_THE_LAST;
			}
			else switch( newField )
			{
				case SJ_FIELD_TIMESPLAYED:      m_op = SJ_FIELDOP_IS_GREATER_THAN;  break;

				case SJ_FIELD_DATABYTES:
				case SJ_FIELD_PLAYTIME:         m_op = SJ_FIELDOP_IS_IN_RANGE;      break;

				case SJ_FIELD_AUTOVOL:
				case SJ_PSEUDOFIELD_QUEUEPOS:   m_op = SJ_FIELDOP_IS_SET;           break;

				default:                        m_op = SJ_FIELDOP_IS_EQUAL_TO;      break;
			}
			largerModifications = TRUE;
		}

		// init the unit, if required
		// (currently, we only set to unknown unit; SjAdvSearchDialog already does its best)
		if( initUnitRequired )
		{
			m_unit = SJ_UNIT_NA;
			largerModifications = TRUE;
		}
	}

	// done
	m_field = newField;
	return largerModifications;
}


bool SjRule::Convert(SjFieldOp newOp)
{
	// Convert the rule operator.
	// This may modify the values and the unit.

	bool largerModifications = FALSE;

	if( newOp != m_op )
	{
		if( newOp == SJ_FIELDOP_IS_SET || newOp == SJ_FIELDOP_IS_UNSET )
		{
			m_value[0].Clear();
			m_value[1].Clear();
			m_unit = SJ_UNIT_NA;
			largerModifications = TRUE;
		}
		else if( GetFieldType(m_field) == SJ_FIELDTYPE_DATE )
		{
			bool newIsRel = (newOp==SJ_FIELDOP_IS_IN_THE_LAST||newOp==SJ_FIELDOP_IS_NOT_IN_THE_LAST)? TRUE : FALSE;
			bool oldisRel = (m_op ==SJ_FIELDOP_IS_IN_THE_LAST||m_op ==SJ_FIELDOP_IS_NOT_IN_THE_LAST)? TRUE : FALSE;
			if( newIsRel != oldisRel )
			{
				m_value[0].Clear();
				m_value[1].Clear();
				m_unit = newIsRel? SJ_UNIT_DAYS : SJ_UNIT_NA;
				largerModifications = TRUE;;
			}
		}
	}

	// done
	m_op = newOp;
	return largerModifications;
}


/*******************************************************************************
 * SjAdvSearch - Basic Operations
 ******************************************************************************/


void SjAdvSearch::Init(const wxString& name, SjSelectScope selectScope, SjSelectOp selectOp)
{
	m_id = 0;
	m_name = name;
	m_selectScope = selectScope;
	m_selectOp = selectOp;
	m_rules.Empty();

	m_subset = SJ_SUBSET_NONE;
	m_subsetId = 0;
}


void SjAdvSearch::AddRule(SjField field, SjFieldOp op, const wxString& value0, const wxString& value1, SjUnit unit)
{
	m_rules.Add(new SjRule(field, op, value0, value1, unit));
}


void SjAdvSearch::AddRule(SjField field, SjFieldOp op, const wxString& value0, long value1, SjUnit unit)
{
	m_rules.Add(new SjRule(field, op, value0, wxString::Format(wxT("%i"), (int)value1), unit));
}


void SjAdvSearch::CopyFrom(const SjAdvSearch& o)
{
	if( this != &o )
	{
		m_id            = o.m_id;
		m_name          = o.m_name;
		m_selectScope   = o.m_selectScope;
		m_selectOp      = o.m_selectOp;
		m_subset        = o.m_subset;
		m_subsetId      = o.m_subsetId;

		m_rules.Empty();
		int i, iCount = o.m_rules.GetCount();
		for( i = 0; i < iCount; i++ )
		{
			AddRule(o.m_rules[i]);
		}
	}
}


bool SjAdvSearch::IsEqualTo(const SjAdvSearch& o) const
{
	int r, rulesCount = (int)m_rules.GetCount();

	if(  m_id           != o.m_id
	 ||  m_name         != o.m_name
	 ||  rulesCount     != (int)o.m_rules.GetCount()
	 ||  m_selectScope  != o.m_selectScope
	 ||  m_selectOp     != o.m_selectOp
	 ||  m_subset       != o.m_subset
	 ||  m_subsetId     != o.m_subsetId )
	{
		return FALSE;
	}

	wxASSERT( rulesCount == (int)m_rules.GetCount() );
	wxASSERT( rulesCount == (int)o.m_rules.GetCount() );
	for( r = 0; r < rulesCount; r++ )
	{
		if( m_rules[r] != o.m_rules[r] )
		{
			return FALSE;
		}
	}

	return TRUE;
}


long SjAdvSearch::IncludeExclude(SjLLHash* ids, int action)
{
	// "action" values:
	// +1   -   add "ids" to the first SJ_PSEUDOFIELD_INCLUDE, merge other includes to the first one, remove "ids" from all SJ_PSEUDOFIELD_EXCLUDE
	// -1   -   add "ids" to the first SJ_PSEUDOFIELD_EXCLUDE, merge other excludes to the first one, remove "ids" from all SJ_PSEUDOFIELD_INCLUDE
	//  0   -   remove "ids" from all SJ_PSEUDOFIELD_INCLUDE and SJ_PSEUDOFIELD_EXCLUDE

	int         r;

	SjField     fieldToAddTo            = action==1? SJ_PSEUDOFIELD_INCLUDE : SJ_PSEUDOFIELD_EXCLUDE;
	SjField     fieldToRemoveFrom       = action==1? SJ_PSEUDOFIELD_EXCLUDE : SJ_PSEUDOFIELD_INCLUDE;

	long        fieldToAddToIndex       = -1;
	long        fieldToRemoveFromIndex  = -1;

	// include/exclude to the correct rules
	if( ids->GetCount() )
	{
		for( r = 0; r < (int)m_rules.GetCount(); r++ )
		{
			if( action == 0 )
			{
				if( m_rules[r].m_field == SJ_PSEUDOFIELD_INCLUDE || m_rules[r].m_field == SJ_PSEUDOFIELD_EXCLUDE )
				{
					m_rules[r].RemoveFromInclExcl(ids);
					if( fieldToAddToIndex == -1 ) fieldToAddToIndex = r;
				}
			}
			else
			{
				if( m_rules[r].m_field == fieldToAddTo )
				{
					if( fieldToAddToIndex == -1 )
					{
						// add IDs
						fieldToAddToIndex = r;
						m_rules[r].AddToInclExcl(ids);
					}
					else
					{
						// prepare for deletion; 19.11.2008: bug fixed, see http://www.silverjuke.net/forum/topic-2936.html
						m_rules[fieldToAddToIndex].AddToInclExcl(m_rules[r].m_value[0]);
						m_rules[r].m_value[0].Clear();
					}
				}
				else if( m_rules[r].m_field == fieldToRemoveFrom )
				{
					if( fieldToRemoveFromIndex == -1 )
					{
						// remove IDs
						fieldToRemoveFromIndex = r;
						m_rules[r].RemoveFromInclExcl(ids);
					}
					else
					{
						// prepare for deletion
						m_rules[fieldToRemoveFromIndex].AddToInclExcl(m_rules[r].m_value[0]);
						m_rules[r].m_value[0].Clear();
					}
				}
			}
		}

		// no include/exclude rule presend, add one
		if( action != 0 && fieldToAddToIndex == -1 )
		{
			AddRule(fieldToAddTo, SJ_FIELDOP_IS_EQUAL_TO);
			fieldToAddToIndex = m_rules.GetCount()-1;
			m_rules[fieldToAddToIndex].AddToInclExcl(ids);
		}

		// remove all empty incl./excl. rules
		for( r = 0; r < (int)m_rules.GetCount(); r++ )
		{
			if( (m_rules[r].m_field == SJ_PSEUDOFIELD_INCLUDE || m_rules[r].m_field == SJ_PSEUDOFIELD_EXCLUDE)
			        &&  m_rules[r].GetInclExclCount() == 0 )
			{
				m_rules.RemoveAt(r);
				r--;
			}
		}
	}

	// done
	return fieldToAddToIndex;
}


void SjAdvSearch::Serialize(SjStringSerializer& ser) const
{
	int     r, ruleCount = (int)m_rules.GetCount();

	ser.AddLong     (5);            // fields following, for future use
	ser.AddLong     (m_id);         // just for verification, if unknown, please set this to 0
	ser.AddString   (m_name);       // just for verification, if unknown, please set this to an empty string
	ser.AddLong     (m_selectScope);
	ser.AddLong     (m_selectOp);
	ser.AddLong     (ruleCount);

	for( r = 0; r < ruleCount; r++ )
	{
		m_rules[r].Serialize(ser);
	}
}


bool SjAdvSearch::Unserialize(SjStringSerializer& ser)
{
	long fieldsFollowing=                ser.GetLong     ();
	m_id                =                ser.GetLong     ();
	m_name              =                ser.GetString   ();
	m_selectScope       = (SjSelectScope)ser.GetLong     ();
	m_selectOp          = (SjSelectOp)   ser.GetLong     ();
	long rulesFollowing =                ser.GetLong     ();

	if( fieldsFollowing != 5
	 || m_selectScope >= SJ_SELECTSCOPE_COUNT
	 || m_selectOp >= SJ_SELECTOP_COUNT
	 || ser.HasErrors() )
	{
		Clear();
		return FALSE;
	}

	SjRule ruleToAdd;
	m_rules.Empty();
	while( rulesFollowing )
	{
		if( !ruleToAdd.Unserialize(ser) )
		{
			Clear();
			return FALSE;
		}
		AddRule(ruleToAdd);
		rulesFollowing--;
	}

	return TRUE;
}


/*******************************************************************************
 * SjAdvSearch::IsOk()
 ******************************************************************************/


bool SjAdvSearch::IsOk(wxString& retError, wxString& retWarning) const
{
	wxString    ruleError1, ruleError2;
	size_t      r;
	int         criteriaCount = 0;
	bool        addCaseWarning = FALSE;

	retError.Clear();
	retWarning.Clear();

	for( r = 0; r < m_rules.GetCount(); r++ )
	{
		m_rules[r].IsOk(ruleError1, ruleError2, &addCaseWarning);

		if( !ruleError1.IsEmpty() )
		{
			retError += wxT("\n- ") + m_rules[r].GetFieldDescr() + wxT(": ") + ruleError1;
		}

		if( !ruleError2.IsEmpty() )
		{
			retError += wxT("\n -") + m_rules[r].GetFieldDescr();
			if( !ruleError1.IsEmpty() ) retError += wxString(wxT(" ")) + _("(second field)");
			retError += wxT(": ") + ruleError2;
		}

		criteriaCount++;
	}

	if( criteriaCount == 0 )
	{
		retError += wxT("\n -") + wxString(_("No valid criteria defined."));;
	}

	if( !retError.IsEmpty() )
	{
		retError.Prepend(_("The search cannot be started; please check the following fields:")+wxString(wxT("\n")));
	}

	if( addCaseWarning )
	{
		retWarning = _("Note: The operators \"equal\" and \"unequal\" are case-sensitive.");
	}

	return retError.IsEmpty();
}


/*******************************************************************************
 *  SjAdvSearch::GetAsSql() and the SjLimitValues help class
 ******************************************************************************/


class LimitValues
{
public:
	LimitValues         () { m_fieldIndex = 0; m_max = 0; m_curr = 0; }
	bool            Set                 (int fieldIndex, long max__, long multiplier);
	bool            IsSet               () const { return (m_fieldIndex!=0); }

	int             m_fieldIndex;
	wxLongLong      m_max, m_curr;
};


bool LimitValues::Set(int fieldIndex, long max__, long multiplier)
{
	// function returns TRUE if the limit is initialized and FALSE if it is changed
	wxASSERT( fieldIndex );
	wxLongLong max = max__;
	max *= multiplier;

	if( m_fieldIndex )
	{
		if( max < m_max )
		{
			m_max = max;
		}
		return FALSE;
	}
	else
	{
		m_max = max;
		m_fieldIndex = fieldIndex;
		return TRUE;
	}
}


SjSearchStat SjAdvSearch::GetAsSql(SjLLHash* retHash, wxString& retSql) const
{
	SjSearchStat stat;

	//
	// init the return values
	//

	retHash->Clear();
	if( !IsSet() )
	{
		retSql = wxT("(1)"); // no filter - select all
		stat.m_advResultCount = -1;
		return stat;
	}

	//
	// go through all rules and collect the intermediate conditions
	//

	wxString        fields = wxT("id");
	int             fieldsCount = 1;

	wxString        where;
	wxString        excl;

	wxString        orderBy;

	#define         LIMIT_BYTES     0
	#define         LIMIT_MS        1
	#define         LIMIT_TRACKS    2
	#define         LIMIT_ALBUMS    3
	#define         LIMIT_COUNT     4
	LimitValues     limit[LIMIT_COUNT];

	{
		size_t          r;
		SjRule*         rule;
		wxArrayString   orderByArray;
		wxString        incl;

		for( r = 0; r < m_rules.GetCount(); r++ )
		{
			rule = &m_rules[r];

			if( rule->m_field == SJ_PSEUDOFIELD_INCLUDE )
			{
				//
				// MANUAL INCLUDE/EXCLUDE track ids; this is handled separatly from the select
				// operation as these conditions should have a higher priority
				//
				if( !incl.IsEmpty() ) { incl += wxT(" OR "); }
				incl += rule->GetAsSql();
			}
			else if( rule->m_field == SJ_PSEUDOFIELD_EXCLUDE )
			{
				if( !excl.IsEmpty() ) { excl += wxT(" AND "); }
				excl += rule->GetAsSql();
			}
			else if( rule->m_field == SJ_PSEUDOFIELD_LIMIT )
			{

				//
				// Add a LIMIT / ORDER BY condition.
				// Several LIMIT / ORDER BY rules are okay: The smallest value of each unit
				// is taken as the limit and the fields are ordered using several conditions.
				//

				long ruleLimitAmount; SjTools::ParseNumber(rule->m_value[0], &ruleLimitAmount);
				long ruleOrderById;   SjTools::ParseNumber(rule->m_value[1], &ruleOrderById);

				// apply LIMIT
				long multiplier = 1;
				switch( rule->m_unit )
				{
					case SJ_UNIT_HOURS:     multiplier = 60;
					case SJ_UNIT_MINUTES:   if( limit[LIMIT_MS].Set(fieldsCount,
						                            ruleLimitAmount, multiplier*60*1000) )
						{
							fields += wxT(",playtimems"); fieldsCount++;
						}
						break;

					case SJ_UNIT_GB:        multiplier = 1024;
					case SJ_UNIT_MB:        if( limit[LIMIT_BYTES].Set(fieldsCount,
						                            ruleLimitAmount, multiplier*1024*1024) )
						{
							fields += wxT(",databytes"); fieldsCount++;
						}
						break;

					case SJ_UNIT_TRACKS:    limit[LIMIT_TRACKS].Set(fieldsCount,
						        ruleLimitAmount, 1);
						// no fields needed for this limit type
						break;

					case SJ_UNIT_ALBUMS:    if( limit[LIMIT_ALBUMS].Set(fieldsCount,
						                            ruleLimitAmount, 1) )
						{
							if( m_selectScope == SJ_SELECTSCOPE_TRACKS )
							{
								fields += wxT(",albumid"); fieldsCount++;
							}   // no else: when selecting albums (SJ_SELECTSCOPE_ALBUMS),
						}       // the album limit is just counted in the outer loop, see below
						break;

					default:                break;
				}

				// apply ORDER BY by adding the condition to the array, if not exists
				wxString ruleOrderByField;
				if( ruleOrderById == SJ_PSEUDOFIELD_RANDOM )
				{
					ruleOrderByField = wxT("random()");
				}
				else
				{
					ruleOrderByField = rule->GetFieldDbName((SjField)(ruleOrderById&~SJ_FIELDFLAG_DESC))
					                   + wxString(ruleOrderById&SJ_FIELDFLAG_DESC? wxT(" DESC") : wxT(""));
				}

				if( orderByArray.Index(ruleOrderByField) == wxNOT_FOUND )
				{
					orderByArray.Add(ruleOrderByField);
				}
			}
			else
			{

				//
				// Add a (from this point of view) simple WHERE condition.
				//

				if( !where.IsEmpty() )
				{
					where += m_selectOp==SJ_SELECTOP_ALL? wxT(" AND ") : wxT(" OR ");
				}

				where += rule->GetAsSql();
			}

		}

		// add include / exclude conditions to WHERE
		if( !incl.IsEmpty() )
		{
			if( !where.IsEmpty() ) { where = wxT("(") + where + wxT(") OR "); }
			where += incl;
		}

		if( !excl.IsEmpty() )
		{
			if( !where.IsEmpty() ) { where = wxT("(") + where + wxT(") AND "); }
			where += excl;
		}

		// serialize the ORDER BY array to a string
		for( r = 0; r < orderByArray.GetCount(); r++ )
		{
			if( r ) { orderBy += wxT(","); }
			orderBy += orderByArray[r];
		}
	}

	//
	// apply NOT on WHERE clause?
	//

	if(  m_selectOp == SJ_SELECTOP_NONE
	        && !where.IsEmpty() )
	{
		where = wxT("NOT(") + where + wxT(")");
	}

	//
	// limit macros for the select
	//

	#define CHECK_LIMIT(i)      if( limit[(i)].m_fieldIndex ) \
								 { \
									 limit[(i)].m_curr += innerSql.GetLong(limit[(i)].m_fieldIndex); \
									 if( limit[(i)].m_curr > limit[(i)].m_max ) \
									 { \
										 limit[(i)].m_curr -= innerSql.GetLong(limit[(i)].m_fieldIndex); \
										 limitReached = TRUE; \
										 break; \
									 } \
								 }

	#define CHECK_LIMIT_INC(i)  if( limit[(i)].m_fieldIndex ) \
								 { \
									 limit[(i)].m_curr++; \
									 if( limit[(i)].m_curr > limit[(i)].m_max ) \
									 { \
										 limit[(i)].m_curr--; \
										 limitReached = TRUE; \
										 break; \
									 } \
								 }

	//
	// do the select
	//

	{
		wxSqlt  innerSql;
		bool    limitReached = FALSE;

		if( m_selectScope == SJ_SELECTSCOPE_TRACKS )
		{
			//
			// select single tracks
			//

			bool        hasInnerLimit = (limit[LIMIT_BYTES].IsSet()||limit[LIMIT_MS].IsSet()||limit[LIMIT_ALBUMS].IsSet());
			SjLLHash    uniqueAlbumIds; // needed when limiting by albums

			// build inner query
			wxString                              query = wxT("SELECT ") + fields + wxT(" FROM tracks");
			if( !where.IsEmpty() )              { query += wxT(" WHERE ") + where;          }
			if( !orderBy.IsEmpty() )            { query += wxT(" ORDER BY ") + orderBy; }
			if( limit[LIMIT_TRACKS].IsSet() )   { query += wxString::Format(wxT(" LIMIT %i"), (int)limit[LIMIT_TRACKS].m_max.GetLo()); }
			query += wxT(";");
			#ifdef __WXDEBUG__
				{ wxString queryDebug__(query); queryDebug__.Replace(wxT("%"), wxT("%%")); wxLogDebug(queryDebug__); }
			#endif

			// query
			innerSql.Query(query);
			while( innerSql.Next() )
			{
				if( hasInnerLimit )
				{
					// check limits (limiting by tracks is done by the LIMIT command in the SQL query)
					CHECK_LIMIT     (LIMIT_BYTES);
					CHECK_LIMIT     (LIMIT_MS);

					if( limit[LIMIT_ALBUMS].m_fieldIndex )
					{
						// limiting for albums: tracks are okay as long as the maximum is not reached;
						// if the maximum is reached, only tracks with already existing album IDs are okay.

						if( limitReached )
						{
							if( uniqueAlbumIds.Lookup(innerSql.GetLong(limit[LIMIT_ALBUMS].m_fieldIndex)) == 0 )
							{
								continue; // don't add this track
							}
						}
						else if( uniqueAlbumIds.Insert(innerSql.GetLong(limit[LIMIT_ALBUMS].m_fieldIndex), 1) == 0 )
						{
							limit[LIMIT_ALBUMS].m_curr++;
							if( limit[LIMIT_ALBUMS].m_curr >= limit[LIMIT_ALBUMS].m_max )
							{
								limitReached = TRUE;
								// from now on, only tracks with existing album IDs are added
							}
						}
					}
				}

				// add the track to the result
				retHash->Insert(innerSql.GetLong(0), 1);
			}
		}
		else
		{
			//
			// select complete albums
			//

			bool        hasInnerLimit = (limit[LIMIT_BYTES].IsSet()||limit[LIMIT_MS].IsSet()||limit[LIMIT_TRACKS].IsSet());
			wxSqlt      outerSql;

			// build outer query
			wxString                            outerQuery =  wxT("SELECT DISTINCT albumid FROM tracks");
			if( !where.IsEmpty() )            { outerQuery += wxT(" WHERE ") + where;       }
			if( !orderBy.IsEmpty() )          { outerQuery += wxT(" ORDER BY ") + orderBy;  }
			outerQuery += wxT(";");
			#ifdef __WXDEBUG__
				{ wxString queryDebug__(outerQuery); queryDebug__.Replace(wxT("%"), wxT("%%")); wxLogDebug(queryDebug__); }
			#endif

			// build inner query
			wxString                            innerQuery =  wxT("SELECT ") + fields + wxT(" FROM tracks WHERE albumid=%i");
			if( !excl.IsEmpty() )             { innerQuery += wxT(" AND ") + excl; }
			innerQuery += wxT(";");

			// outer query
			outerSql.Query(outerQuery);
			while( !limitReached && outerSql.Next() )
			{
				CHECK_LIMIT_INC (LIMIT_ALBUMS);

				// inner query
				innerSql.Query(wxString::Format(innerQuery, (int)outerSql.GetLong(0)));
				while( innerSql.Next() )
				{
					if( hasInnerLimit )
					{
						CHECK_LIMIT     (LIMIT_BYTES);
						CHECK_LIMIT     (LIMIT_MS);
						CHECK_LIMIT_INC (LIMIT_TRACKS);
					}

					// add the track to the result
					retHash->Insert(innerSql.GetLong(0), 1);
				}
			}
		}
	}

	//
	// done so far
	//

	stat.m_advResultCount = retHash->GetCount();
	if( limit[LIMIT_BYTES].IsSet() )
	{
		limit[LIMIT_BYTES].m_curr /= (1024*1024);
		stat.m_mbytes = limit[LIMIT_BYTES].m_curr.GetLo();
	}
	else
	{
		stat.m_mbytes = -1;
	}

	if( limit[LIMIT_MS].IsSet() )
	{
		limit[LIMIT_MS].m_curr /= 1000;
		stat.m_seconds = limit[LIMIT_MS].m_curr.GetLo();
	}
	else
	{
		stat.m_seconds = -1;
	}

	if( stat.m_advResultCount <= 0 )
	{
		wxASSERT( stat.m_advResultCount == 0 );
		retSql = wxT("(0)");
	}
	/*else if( stat.m_advResultCount < 500 )
	{
	    // not used as we modify the given hash
	    // for easy filter manipulation
	    retSql = "tracks.id IN("+ retHash->GetKeysAsString() +")";
	}
	else */
	{
		retSql = wxT("INFILTER(tracks.id)");
	}

	return stat;
}


/*******************************************************************************
 * SjSearchStat
 ******************************************************************************/


void SjSearchStat::Clear()
{
	m_totalResultCount  = 0;
	m_advResultCount    = 0;
	m_mbytes            = 0;
	m_seconds           = 0;
}


void SjSearchStat::Add(const SjSearchStat& o)
{
	#define ADD_ELEM(a,b) if( (a)!=-1 && (b)!=-1 ) { (a) += (b); }

	ADD_ELEM(m_totalResultCount, o.m_totalResultCount);
	ADD_ELEM(m_advResultCount,   o.m_advResultCount);
	ADD_ELEM(m_mbytes,           o.m_mbytes);
	ADD_ELEM(m_seconds,          o.m_seconds);
}


void SjSearchStat::CopyFrom(const SjSearchStat& o)
{
	m_totalResultCount  = o.m_totalResultCount;
	m_advResultCount    = o.m_advResultCount;
	m_mbytes            = o.m_mbytes;
	m_seconds           = o.m_seconds;
}
