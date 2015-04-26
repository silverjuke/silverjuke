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
 * File:    searchsj.h
 * Authors: Björn Petersen
 * Purpose: searching
 *
 ******************************************************************************/



#ifndef __SJ_SEARCHSJ_H__
#define __SJ_SEARCHSJ_H__


/*******************************************************************************
 * Enums
 ******************************************************************************/


enum SjField
{
    // do not modify the order of this enum as the values are stored to disk!
    // add new values to the end (before SJ_FIELD_COUNT)!

     SJ_PSEUDOFIELD_LIMIT = 0
    ,SJ_PSEUDOFIELD_RANDOM
    ,SJ_PSEUDOFIELD_SQL
    ,SJ_PSEUDOFIELD_TRACKARTISTALBUM
    ,SJ_PSEUDOFIELD_FILETYPE
    ,SJ_PSEUDOFIELD_QUEUEPOS
    ,SJ_PSEUDOFIELD_INCLUDE
    ,SJ_PSEUDOFIELD_EXCLUDE

    ,SJ_FIELD_URL = 100
    ,SJ_FIELD_TIMEADDED
    ,SJ_FIELD_TIMEMODIFIED
    ,SJ_FIELD_TIMESPLAYED
    ,SJ_FIELD_LASTPLAYED
    ,SJ_FIELD_DATABYTES
    ,SJ_FIELD_BITRATE
    ,SJ_FIELD_SAMPLERATE
    ,SJ_FIELD_CHANNELS
    ,SJ_FIELD_PLAYTIME
    ,SJ_FIELD_AUTOVOL

    ,SJ_FIELD_TRACKNAME = 200
    ,SJ_FIELD_TRACKNR
    ,SJ_FIELD_TRACKCOUNT
    ,SJ_FIELD_DISKNR
    ,SJ_FIELD_DISKCOUNT

    ,SJ_FIELD_LEADARTISTNAME = 300
    ,SJ_FIELD_ORGARTISTNAME
    ,SJ_FIELD_COMPOSERNAME

    ,SJ_FIELD_ALBUMNAME = 400

    ,SJ_FIELD_GENRENAME = 500
    ,SJ_FIELD_GROUPNAME
    ,SJ_FIELD_COMMENT
    ,SJ_FIELD_BEATSPERMINUTE
    ,SJ_FIELD_RATING
    ,SJ_FIELD_YEAR

    ,SJ_FIELD_COUNT

    ,SJ_FIELDFLAG_DESC = 0x10000000L
};


enum SjFieldOp
{
    // do not modify the order or the values of this enum as the values are stored to disk!
    // add new values to the end (before SJ_FIELDOP_COUNT)!

     SJ_FIELDOP_IS_EQUAL_TO = 0
    ,SJ_FIELDOP_IS_UNEQUAL_TO
    ,SJ_FIELDOP_IS_SET
    ,SJ_FIELDOP_IS_UNSET
    ,SJ_FIELDOP_IS_GREATER_THAN
    ,SJ_FIELDOP_IS_GREATER_OR_EQUAL
    ,SJ_FIELDOP_IS_LESS_THAN
    ,SJ_FIELDOP_IS_LESS_OR_EQUAL
    ,SJ_FIELDOP_IS_IN_RANGE
    ,SJ_FIELDOP_IS_NOT_IN_RANGE
    ,SJ_FIELDOP_CONTAINS
    ,SJ_FIELDOP_DOES_NOT_CONTAIN
    ,SJ_FIELDOP_STARTS_WITH
    ,SJ_FIELDOP_DOES_NOT_START_WITH
    ,SJ_FIELDOP_ENDS_WITH
    ,SJ_FIELDOP_DOES_NOT_END_WITH
    ,SJ_FIELDOP_IS_IN_THE_LAST
    ,SJ_FIELDOP_IS_NOT_IN_THE_LAST
    ,SJ_FIELDOP_IS_SIMELAR_TO
    ,SJ_FIELDOP_STARTS_SIMELAR_TO

    ,SJ_FIELDOP_COUNT
};



enum SjFieldType
{
     SJ_FIELDTYPE_STRING = 0
    ,SJ_FIELDTYPE_NUMBER
    ,SJ_FIELDTYPE_DATE
    ,SJ_FIELDTYPE_LIST

    ,SJ_FIELDTYPE_COUNT
};



enum SjUnit
{
    // do not modify the order of this enum as the values are stored to disk!
    // add new values to the end (before SJ_UNIT_COUNT)!

     SJ_UNIT_NA = 0
    ,SJ_UNIT_SECONDS = 100
    ,SJ_UNIT_MINUTES
    ,SJ_UNIT_HOURS
    ,SJ_UNIT_DAYS
    ,SJ_UNIT_BYTE = 200
    ,SJ_UNIT_KB
    ,SJ_UNIT_MB
    ,SJ_UNIT_GB
    ,SJ_UNIT_TRACKS = 300
    ,SJ_UNIT_ALBUMS

    ,SJ_UNIT_COUNT
};



enum SjSelectScope
{
    // do not modify the order of this enum as the values are stored to disk!
    // add new values to the end (before SJ_SELECT_COUNT)!

     SJ_SELECTSCOPE_ALBUMS = 0
    ,SJ_SELECTSCOPE_TRACKS

    ,SJ_SELECTSCOPE_COUNT
};


enum SjSelectOp
{
    // do not modify the order of this enum as the values are stored to disk!
    // add new values to the end (before SJ_SELECTOP_COUNT)!

     SJ_SELECTOP_ALL = 0
    ,SJ_SELECTOP_ANY
    ,SJ_SELECTOP_NONE

    ,SJ_SELECTOP_COUNT
};


enum SjSubset
{
     SJ_SUBSET_NONE = 0
    ,SJ_SUBSET_INCLUDED_TRACKS
    ,SJ_SUBSET_EXCLUDED_TRACKS

    ,SJ_SUBSET_COUNT
};


#define SJ_SELECTSCOPE_DEFAULT  SJ_SELECTSCOPE_TRACKS
#define SJ_SELECTOP_DEFAULT     SJ_SELECTOP_ALL
#define SJ_FIELD_DEFAULT        SJ_PSEUDOFIELD_TRACKARTISTALBUM
#define SJ_FIELDOP_DEFAULT      SJ_FIELDOP_CONTAINS
#define SJ_UNIT_DEFAULT         SJ_UNIT_NA
#define SJ_QUEUEPOS_CURR_STR    wxT("QUEUEPOS()")


/*******************************************************************************
 *  SjSearchStat
 ******************************************************************************/



class SjSearchStat
{
public:
	                SjSearchStat        (const SjSearchStat& o) { CopyFrom(o); }
	                SjSearchStat        () { Clear(); }
	void            Clear               ();
	SjSearchStat&   operator +=         (const SjSearchStat& o) { Add(o); return *this; }
	SjSearchStat&   operator =          (const SjSearchStat& o) { CopyFrom(o); return *this; }

	long            m_totalResultCount;
	long            m_advResultCount; // unknown adv. result count is set to -1
	long            m_mbytes;
	long            m_seconds;

private:
	void            Add                 (const SjSearchStat& o);
	void            CopyFrom            (const SjSearchStat& o);
};




/*******************************************************************************
 *  SjRule
 ******************************************************************************/



class SjRule
{
public:
	// constructor
	SjRule              () { Clear(); }
	SjRule              (SjField         field,
	                     SjFieldOp       op,
	                     const wxString& value0 = wxT(""),
	                     const wxString& value1 = wxT(""),
	                     SjUnit          unit   = SJ_UNIT_DEFAULT);
	SjRule              (const SjRule& o) { CopyFrom(o); }
	void            Clear               ();

	// copy / compare
	SjRule&         operator =          (const SjRule& o) { CopyFrom(o); return *this; }
	bool            operator ==         (const SjRule& o) const { return  IsEqualTo(o); }
	bool            operator !=         (const SjRule& o) const { return !IsEqualTo(o); }

	// is the rule valid?
	bool            IsOk                (wxString& error1, wxString& error2, bool* addCaseWarning=NULL) const;

	// get information about the rule
	wxString        GetFieldDescr       () const { return GetFieldDescr(m_field); }

	/// get common information about rules
	static wxString GetFieldDescr       (SjField);
	static SjFieldType
	GetFieldType        (SjField);
	static bool     IsValidFieldOp      (SjField field, SjFieldOp op);
	static wxString GetFieldDbName      (SjField);
	#if 0
	static wxString GetFieldsAsHtml     ();
	static wxString GetFunctionsAsHtml  ();
	#endif

	// Convert the field or the operator of the rule.
	// The function returns TRUE if the conversion does not change
	// any types -- otherwise, the conversion is also done but some
	// fields are initialized; in this case FALSE is returned.
	bool            Convert             (SjField newField);
	bool            Convert             (SjFieldOp newOp);

	// (un-)serialize
	void            Serialize           (SjStringSerializer&) const;
	bool            Unserialize         (SjStringSerializer&);

	// special stuff for include/exclude IDs
	// (internally, the IDs are stored in a string as ",123,456,78," (note the commas at start/end!)
	long            GetInclExclCount    () const;
	wxString        GetInclExclDescr    () const;
	void            RemoveFromInclExcl  (SjLLHash* ids);
	void            AddToInclExcl       (SjLLHash* ids);
	void            AddToInclExcl       (const wxString& idsToAdd);

private:
	// private stuff
	SjField         m_field;
	SjFieldOp       m_op;
	wxString        m_value[2];
	SjUnit          m_unit;
	wxString        IsValueOk           (const wxString& value, bool* addCaseWarning=NULL) const;
	static wxString GetAsSql            (const wxString& value, SjField, SjFieldOp, SjUnit, bool forceSet, bool recursiveCall=FALSE);
	static wxString GetAsSql            (SjField, SjFieldOp, bool forceSet);
	wxString        GetAsSql            () const;
	void            CopyFrom            (const SjRule& o);
	bool            IsEqualTo           (const SjRule& o) const;

	friend class    SjRuleControls;
	friend class    SjAdvSearch;
};



WX_DECLARE_OBJARRAY(SjRule, SjArrayRule);



/*******************************************************************************
 *  SjAdvSearch
 ******************************************************************************/



class SjAdvSearch
{
public:
	// constructor
	SjAdvSearch         () { Init(); }
	void            Clear               () { Init(); }
	void            Init                (const wxString& name=wxT(""), SjSelectScope selectScope=SJ_SELECTSCOPE_DEFAULT, SjSelectOp selectOp=SJ_SELECTOP_DEFAULT);
	void            SetSubset           (SjSubset subset, long subsetId) { m_subset=subset; m_subsetId=subsetId; }

	// copy / compare
	SjAdvSearch&    operator =          (const SjAdvSearch& o) { CopyFrom(o); return *this; }
	bool            operator ==         (const SjAdvSearch& o) const { return  IsEqualTo(o); }
	bool            operator !=         (const SjAdvSearch& o) const { return !IsEqualTo(o); }

	// get information about the advanced search
	bool            IsSet               () const { return !m_rules.IsEmpty(); }
	bool            IsOk                (wxString& error, wxString& warning) const;
	wxString        GetName             () const { return m_name; }
	long            GetId               () const { return m_id; }

	// adding rules to the advanced search
	void            AddRule             (const SjRule& rule) { m_rules.Add(new SjRule(rule)); }
	void            AddRule             (SjField field=SJ_FIELD_DEFAULT, SjFieldOp op=SJ_FIELDOP_DEFAULT, const wxString& value0=wxT(""), const wxString& value1=wxT(""), SjUnit unit=SJ_UNIT_DEFAULT);
	void            AddRule             (SjField field, SjFieldOp op, const wxString& value0, long value1, SjUnit unit=SJ_UNIT_DEFAULT);

	// including/excluding IDs,
	// returns the first changed rule or -1 if nothing is changed
	long            IncludeExclude      (SjLLHash* ids, int action);

	// Get the advanved search as simple conditions for an SQL statement.
	// The given hash MAY be used in combination with the SQL-Function INFILTER()
	// which queries the hash and must be provided by the caller.
	// Normally, the function returns sth. like "(1)", "(0)" or "INFILTER(tracks.id)"
	SjSearchStat    GetAsSql            (SjLLHash*, wxString&) const;

	// (un-)serialize
	void            Serialize           (SjStringSerializer&) const;
	bool            Unserialize         (SjStringSerializer&);

private:
	// private stuff
	long            m_id;
	wxString        m_name;
	SjSelectScope   m_selectScope;
	SjSelectOp      m_selectOp;
	SjArrayRule     m_rules;

	// subsets are used eg. to show the included or excluded tracks
	// as a separate search
	SjSubset        m_subset;
	long            m_subsetId;

	void            CopyFrom            (const SjAdvSearch& o);
	bool            IsEqualTo           (const SjAdvSearch& o) const;

	friend class    SjAdvSearchDialog;
	friend class    SjAdvSearchModule;
};



/*******************************************************************************
 *  SjSimpleSearch
 ******************************************************************************/



class SjSimpleSearch
{
public:
	// c'tor
	SjSimpleSearch (const wxString& words=wxT("")) { m_words = words; }
	void Clear () { m_words.Empty(); }

	// copy / compare
	SjSimpleSearch& operator =          (const SjSimpleSearch& o) { CopyFrom(o); return *this; }
	bool            operator ==         (const SjSimpleSearch& o) const { return  IsEqualTo(o); }
	bool            operator !=         (const SjSimpleSearch& o) const { return !IsEqualTo(o); }

	// get information about the simple search
	bool            IsSet               () const { return !m_words.IsEmpty(); }
	wxString        GetWords            () const { return m_words; }
	wxString        GetHiliteWords      () const { return m_words; }

private:
	// private stuff
	wxString        m_words;
	void            CopyFrom            (const SjSimpleSearch& o) { m_words = o.m_words; }
	bool            IsEqualTo           (const SjSimpleSearch& o) const { return (m_words==o.m_words); }
};



/*******************************************************************************
 *  SjSearch
 ******************************************************************************/


class SjSearch
{
public:
	// c'tor
	SjSearch            () { }
	SjSearch            (const wxString& words) : m_simple(words) { }

	// the search
	SjSimpleSearch  m_simple;
	SjAdvSearch     m_adv;

	// copy / compare
	// the search result count is NOT copied or compared!
	SjSearch&	    operator =          (const SjSearch& o) { CopyFrom(o); return *this; }
	bool            operator ==         (const SjSearch& o) const { return  IsEqualTo(o); }
	bool            operator !=         (const SjSearch& o) const { return !IsEqualTo(o); }

	// get information
	bool            IsSet               () const { return (m_simple.IsSet()||m_adv.IsSet()); }

private:
	void            CopyFrom            (const SjSearch& o) { m_adv=o.m_adv; m_simple=o.m_simple; }
	bool            IsEqualTo           (const SjSearch& o) const { return (m_simple==o.m_simple && m_adv==o.m_adv); }
};



#endif // __SJ_SEARCHSJ_H__
