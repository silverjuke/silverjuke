
/*******************************************************************************
 *  Silverjuke Tagger, based upon taglib
 *******************************************************************************
 *
 *  File:       tg_a_tagger_frontend.h
 * Authors: Björn Petersen
 *  Purpose:    The Virtual (ID3) File System for reading Images eg.
 *              directly from MP3 files
 *  OS:         independent
 *
 ******************************************************************************/




#ifndef __SJ_A_TAGGER_FRONTEND_H__
#define __SJ_A_TAGGER_FRONTEND_H__


void        SjInitID3Etc                        (bool initFsHandler);
void        SjExitID3Etc                        ();
SjResult    SjGetTrackInfoFromID3Etc            (wxFSFile*, SjTrackInfo&, long flags);
void        SjGetMoreInfoFromID3Etc             (wxFSFile*, SjProp&);
bool        SjSetTrackInfoToID3Etc              (const wxString& url, const SjTrackInfo&);



#endif // __SJ_A_TAGGER_FRONTEND_H__



