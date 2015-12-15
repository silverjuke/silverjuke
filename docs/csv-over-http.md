CSV over HTTP
================================================================================

Silverjuke can read music libraries from HTTP-Servers, see select "Settings /
Music library / Add source".

This document describes how you can set up your own HTTP server with your
music library to work with Silverjuke and compatible applications.


The "music-lib-cfg.txt" File
--------------------------------------------------------------------------------

Your server must provide a simple text file with the name `music-lib-cfg.txt` in
the used root. Eg. if you use http://domain.com/dir as the server, Silverjuke
searches for http://domain.com/dir/music-lib-cfg.txt 

Each line of the text file has the format `setting=value` and the value may not
exceed one line. Spaces or tabs at the beginning or at the end of the setting
name are ignored. Same for the value.  Lines starting with a semicolon are
ignored and may be used for comments. Semicolons inside a line do not introduce
a comment.


Possible settings
--------------------------------------------------------------------------------

- `access-type=TYPE` - the type of the index, currently only CSV index are
  read and the only valid type is `csv-over-http` - if this setting is not
  provided, the server cannot be used.
  
- `csv-file=PATH` - enter the path to the CSV index file here. Eg. if your
  index file is located at "http://www.myserver.com/music/index.csv" please
  enter music/index.csv
  
- `csv-delim=CHARACTER` - The character used to separate fields from each
  other, defaults to the comma.
  
- `csv-enclose=CHARACTER` - The character used to (optionally) enclose the
  field data, defaults to the upper double quote.
  
- `csv-escape=CHARACTER` - The characted used to escape the enclosing
  character inside a field, also defaults to the upper double quote.
  
- `csv-skip-first-row=0|1` - If the first line of the CSV index just defines the
  field names and should be ignored therefore, use csv-skip-first-row=1 -
  otherwise, use csv-skip-first-row=0 or leave this setting out.
  
- `csv-columns=COLUMN_DEFINITIONS` - A comma separated list of the columns used
  in the CSV index. Eg. if your CSV index contains four columns with an ID and
  the artist/album/track names, you have to enter

      <TrackId>,<Artist>,<Album>,<Title>

  here. In common, you may use _any_ names and re-use them eg. for the 
  music-files setting as described below - however, some names are recognized as
  known fields and will be added to the database:

      <Nr>
      <DiskNr>
      <Title>
      <Artist>
      <OrgArtist>
      <Composer>
      <Album>
      <Album(DiskNr)> (the album name optionally followed by a disk number)
      <Year>
      <Comment>
      <Genre>
      <Group>
      <Seconds>
      <Milliseconds>

- `img-files=PATH_WITH_PLACEHOLDERS` - The path to an image that may be used as
  a cover, the path may contain placeholders that refer to the current record.
  
- `music-files=PATH_WITH_PLACEHOLDERS` - same as the image path but for the
  music file.


Automatic creation of the files
--------------------------------------------------------------------------------

It may be an good idea to create some server scripts that create the CSV-files
so that you need not to do this by hand.  JavaScript, Perl or PHP may help with
this purpose, however, we won't go into details about this here.


Use a local configuration file
--------------------------------------------------------------------------------

If you have a server with music files that fits the needs of CSV-over-HTTP, but
you cannot add a configuration file there, you can put it into one of your local
search paths as `SERVERNAME.txt` with SERVERNAME as the name of the server with 
all characters beside a-z and 0-9 replaced by a minus, eg. `domain-com.txt`.


Example
--------------------------------------------------------------------------------

Assume, you use the server `http://domain.com/dir` which provides the file
`http://domain.com/dir/music-lib-cfg.txt` as follows:

    access-type       = csv-over-http
    csv-file          = index.csv
    csv-skip-first-row= 1
    csv-columns       = <TrackId>,<Artist>,<Album>,<Title>,<Year>
    img-files         = myCovers/<AnyId>-foo.jpg
    music-files       = myMusic/<File>
    
The example references `http://domain.com/dir/index.csv` which may look as:

    AnyId, Artist,   Album,   Title,    File
    100,   Artist A, Album A, Someting, sth.mp3
    22,    Artist A, Album A, Just,     just.ogg
    333,   Band B,   Best Of, w.h.a.t,  what.wav
    421,   Band B,   Best Of, FR,       fr.ogg
    5001,  Band B,   Best Of, fin,      fin.mp3

Finally, the server must provide the following files:

    http://domain.com/dir/myCovers/100-foo.jpg
    http://domain.com/dir/myCovers/22-foo.jpg
    http://domain.com/dir/myCovers/333-foo.jpg
    http://domain.com/dir/myCovers/421-foo.jpg
    http://domain.com/dir/myCovers/5001-foo.jpg
    http://domain.com/dir/myMusic/sth.mp3
    http://domain.com/dir/myMusic/just.ogg
    http://domain.com/dir/myMusic/what.wav
    http://domain.com/dir/myMusic/fr.ogg
    http://domain.com/dir/myMusic/fin.mp3

That's all. As you see, the folder structure and the naming conventions are not
defined by Silverjuke and completely editable.

For further examples, please have a loot at the repository "Silverjuke SDK
Examples" at https://github.com/r10s/silverjuke-sdk-examples


Copyright (c) Björn Petersen Software Design and Development and contributors

