//=======================================================================
//			Copyright (C) XashXT Group 2006
//=======================================================================

#include "hud.h"
#include "cl_util.h"
#include "parsemsg.h"

/*
====================
FMOD definitions

====================
*/

enum FSOUND_OUTPUTTYPES
{
	FSOUND_OUTPUT_NOSOUND,    /* NoSound driver, all calls to this succeed but do nothing. */
	FSOUND_OUTPUT_WINMM,      /* Windows Multimedia driver. */
	FSOUND_OUTPUT_DSOUND,     /* DirectSound driver.  You need this to get EAX or EAX2 support. */
	FSOUND_OUTPUT_A3D,        /* A3D driver.  You need this to get geometry support. */
	FSOUND_OUTPUT_OSS,        /* Linux/Unix OSS (Open Sound System) driver, i.e. the kernel sound drivers. */
	FSOUND_OUTPUT_ESD,        /* Linux/Unix ESD (Enlightment Sound Daemon) driver. */
	FSOUND_OUTPUT_ALSA        /* Linux Alsa driver. */
};

enum FMOD_ERRORS 
{
    FMOD_ERR_NONE,             /* No errors */
    FMOD_ERR_BUSY,             /* Cannot call this command after FSOUND_Init.  Call FSOUND_Close first. */
    FMOD_ERR_UNINITIALIZED,    /* This command failed because FSOUND_Init or FSOUND_SetOutput was not called */
    FMOD_ERR_INIT,             /* Error initializing output device. */
    FMOD_ERR_ALLOCATED,        /* Error initializing output device, but more specifically, the output device is already in use and cannot be reused. */
    FMOD_ERR_PLAY,             /* Playing the sound failed. */
    FMOD_ERR_OUTPUT_FORMAT,    /* Soundcard does not support the features needed for this soundsystem (16bit stereo output) */
    FMOD_ERR_COOPERATIVELEVEL, /* Error setting cooperative level for hardware. */
    FMOD_ERR_CREATEBUFFER,     /* Error creating hardware sound buffer. */
    FMOD_ERR_FILE_NOTFOUND,    /* File not found */
    FMOD_ERR_FILE_FORMAT,      /* Unknown file format */
    FMOD_ERR_FILE_BAD,         /* Error loading file */
    FMOD_ERR_MEMORY,           /* Not enough memory or resources */
    FMOD_ERR_VERSION,          /* The version number of this file format is not supported */
    FMOD_ERR_INVALID_PARAM,    /* An invalid parameter was passed to this function */
    FMOD_ERR_NO_EAX,           /* Tried to use an EAX command on a non EAX enabled channel or output. */
    FMOD_ERR_CHANNEL_ALLOC,    /* Failed to allocate a new channel */
    FMOD_ERR_RECORD,           /* Recording is not supported on this machine */
    FMOD_ERR_MEDIAPLAYER,      /* Windows Media Player not installed so cannot play wma or use internet streaming. */
    FMOD_ERR_CDDEVICE          /* An error occured trying to open the specified CD device */
};

static char *FMOD_ErrorString(int errcode)
{
	switch( errcode )
	{
	case FMOD_ERR_NONE:		return "No errors";
	case FMOD_ERR_BUSY:		return "Cannot call this command after FSOUND_Init.  Call FSOUND_Close first.";
	case FMOD_ERR_UNINITIALIZED:	return "This command failed because FSOUND_Init was not called";
	case FMOD_ERR_PLAY:		return "Playing the sound failed.";
	case FMOD_ERR_INIT:		return "Error initializing output device.";
	case FMOD_ERR_ALLOCATED:	return "The output device is already in use and cannot be reused.";
	case FMOD_ERR_OUTPUT_FORMAT:	return "Soundcard does not support the features needed for this soundsystem (16bit stereo output)";
	case FMOD_ERR_COOPERATIVELEVEL: return "Error setting cooperative level for hardware.";
	case FMOD_ERR_CREATEBUFFER:	return "Error creating hardware sound buffer.";
	case FMOD_ERR_FILE_NOTFOUND:	return "File not found";
	case FMOD_ERR_FILE_FORMAT:	return "Unknown file format";
	case FMOD_ERR_FILE_BAD:	return "Error loading file";
	case FMOD_ERR_MEMORY:	return "Not enough memory ";
	case FMOD_ERR_VERSION:	return "The version number of this file format is not supported";
	case FMOD_ERR_INVALID_PARAM:	return "An invalid parameter was passed to this function";
	case FMOD_ERR_NO_EAX:	return "Tried to use an EAX command on a non EAX enabled channel or output.";
	case FMOD_ERR_CHANNEL_ALLOC:	return "Failed to allocate a new channel";
	case FMOD_ERR_RECORD:	return "Recording not supported on this device";
	case FMOD_ERR_MEDIAPLAYER:	return "Required Mediaplayer codec is not installed";
	default :			return "Unknown error";
	};
}

#define FSOUND_LOOP_OFF	0x00000001
#define FSOUND_LOOP_NORMAL	0x00000002
#define FSOUND_LOADMEMORY	0x00008000
#define FSOUND_MPEGACCURATE	0x00020000

#define FSOUND_ALL		-3    /* for a channel index, this flag will affect ALL channels available!*/

enum FSOUND_MIXERTYPES
{
    FSOUND_MIXER_AUTODETECT,        /* CE/PS2/GC Only - Non interpolating/low quality mixer. */
    FSOUND_MIXER_BLENDMODE,         /* Removed / obsolete. */
    FSOUND_MIXER_MMXP5,             /* Removed / obsolete. */
    FSOUND_MIXER_MMXP6,             /* Removed / obsolete. */

    FSOUND_MIXER_QUALITY_AUTODETECT,/* All platforms - Autodetect the fastest quality mixer based on your cpu. */
    FSOUND_MIXER_QUALITY_FPU,       /* Win32/Linux only - Interpolating/volume ramping FPU mixer.  */
    FSOUND_MIXER_QUALITY_MMXP5,     /* Win32/Linux only - Interpolating/volume ramping P5 MMX mixer.  */
    FSOUND_MIXER_QUALITY_MMXP6,     /* Win32/Linux only - Interpolating/volume ramping ppro+ MMX mixer. */

    FSOUND_MIXER_MONO,              /* CE/PS2/GC only - MONO non interpolating/low quality mixer. For speed*/
    FSOUND_MIXER_QUALITY_MONO,      /* CE/PS2/GC only - MONO Interpolating mixer.  For speed */

    FSOUND_MIXER_MAX
};

/*
====================
Cl definitions

====================
*/

static int	(_stdcall *qfmod_geterror)();
static float	(_stdcall *qfmod_getversion)();
static signed char	(_stdcall *qfmod_setoutput)(int outputtype);
static signed char	(_stdcall *qfmod_setdriver)(int driver);
static signed char	(_stdcall *qfmod_setmixer)(int mixer);
static signed char	(_stdcall *qfmod_setbuffersize)(int len_ms);
static signed char	(_stdcall *qfmod_init)(int mixrate, int maxsoftwarechannels, unsigned int flags);
static void	(_stdcall *qfmod_close)();
static int	(_stdcall *qfmod_getmixer)();
static signed char	(_stdcall *qfmod_freesong)(void *mod);
static signed char	(_stdcall *qfmod_playsong)(void *mod);
static signed char	(_stdcall *qfmod_stopsong)(void *mod);
static void	(_stdcall *qfmod_stopallsongs)();
static void*	(_stdcall *qfmod_loadsong)(const char *name);
static void*	(_stdcall *qfmod_loadsongmemory)(void *data, int length);
static signed char	(_stdcall *qfmod_setmodpause)(void *data, signed char pause);
static signed char	(_stdcall *qfmod_getmodpause)(void *data);
static void*	(_stdcall *qfmod_loadstream)(const char *data, unsigned int mode, int memlength);
static int	(_stdcall *qfmod_playstream)(int channel, void *data);
static signed char	(_stdcall *qfmod_freestream)(void *data);
static signed char	(_stdcall *qfmod_getstreampause)(int channel);
static int	(_stdcall *qfmod_getstreampos)(void *data);
static signed char	(_stdcall *qfmod_setstreampause)(int channel, signed char paused);
static signed char	(_stdcall *qfmod_setstreampos)(void *data, unsigned int pos);
static signed char	(_stdcall *qfmod_stopstream)(void *data);
static signed char	(_stdcall *qfmod_setvolume)(int channel, int vol );
static signed char	(_stdcall *qfmod_setmodvolume)( void *mod, int volume );

static dllfunction_t fmodfuncs[] =
{
	{"_FSOUND_GetError@0",	(void **) &qfmod_geterror},
	{"_FSOUND_GetVersion@0",	(void **) &qfmod_getversion},
	{"_FSOUND_SetOutput@4",	(void **) &qfmod_setoutput},
	{"_FSOUND_SetDriver@4",	(void **) &qfmod_setdriver},
	{"_FSOUND_SetMixer@4",	(void **) &qfmod_setmixer},
	{"_FSOUND_SetBufferSize@4",	(void **) &qfmod_setbuffersize},
	{"_FSOUND_Init@12",		(void **) &qfmod_init},
	{"_FSOUND_Close@0",		(void **) &qfmod_close},
	{"_FSOUND_GetMixer@0",	(void **) &qfmod_getmixer},
	{"_FMUSIC_FreeSong@4",	(void **) &qfmod_freesong},
	{"_FMUSIC_PlaySong@4",	(void **) &qfmod_playsong},
	{"_FMUSIC_StopSong@4",	(void **) &qfmod_stopsong},
	{"_FMUSIC_StopAllSongs@0",	(void **) &qfmod_stopallsongs},
	{"_FMUSIC_LoadSong@4",	(void **) &qfmod_loadsong},
	{"_FMUSIC_LoadSongMemory@8",	(void **) &qfmod_loadsongmemory},
	{"_FMUSIC_SetPaused@8",	(void **) &qfmod_setmodpause},
	{"_FMUSIC_GetPaused@4",	(void **) &qfmod_getmodpause},
	{"_FMUSIC_SetMasterVolume@8",	(void **) &qfmod_setmodvolume},
	{"_FSOUND_Stream_OpenFile@12",(void **) &qfmod_loadstream},
	{"_FSOUND_Stream_Play@8",	(void **) &qfmod_playstream},
	{"_FSOUND_Stream_Close@4",	(void **) &qfmod_freestream},
	{"_FSOUND_GetPaused@4",	(void **) &qfmod_getstreampause},
	{"_FSOUND_Stream_GetPosition@4",(void **) &qfmod_getstreampos},
	{"_FSOUND_SetPaused@8",	(void **) &qfmod_setstreampause},
	{"_FSOUND_Stream_SetPosition@8",(void **) &qfmod_setstreampos},
	{"_FSOUND_Stream_Stop@4",	(void **) &qfmod_stopstream},
	{"_FSOUND_SetVolume@8",	(void **) &qfmod_setvolume},
	{NULL, NULL}
};

DECLARE_MESSAGE( m_Sound, Fsound )

#define STREAM	1
#define TRACK	2

// Handle for fmod
static dllhandle_t fmod_dll = NULL;
void *fmod_data = NULL;
char *song_data = NULL;
static char songname[256];
static char barename[256];
int last_state = 0;

static int CheckFormat( qboolean skip_buffer )
{
	// check buffer
	if( !fmod_data && !skip_buffer )
		return false;

	// detect of music type
	if( !_stricmp( UTIL_FileExtension( songname ), "mp3" ))
		return STREAM;
	if( !_stricmp( UTIL_FileExtension( songname ), "wma" ))
		return STREAM;
	if( !_stricmp( UTIL_FileExtension( songname ), "ogg" ))
		return STREAM;	
	if( !_stricmp( UTIL_FileExtension( songname ), "xm" ))
		return TRACK;
	if( !_stricmp( UTIL_FileExtension( songname ), "it" ))
		return TRACK;
	if( !_stricmp( UTIL_FileExtension( songname ), "s3m" ))
		return TRACK;
	return false;
}

int CHudSound::Init( void )
{
	m_iStatus = 0;

	HOOK_MESSAGE(Fsound);

	m_iFlags |= HUD_ACTIVE;
	gHUD.AddHudElem(this);

	// Already loaded?
	if (fmod_dll) return 1;

	if( Sys_LoadLibrary("fmod.dll", &fmod_dll, fmodfuncs ))
	{
		if( qfmod_getversion() < 3.3f || qfmod_getversion() > 3.4f )
		{
			CONPRINT( "Warning: Invalid fmod version: %g\n", qfmod_getversion( ));
			Sys_UnloadLibrary( &fmod_dll ); // free library
			return 1;
          	}

		qfmod_setbuffersize( 100 );
          	qfmod_setoutput( FSOUND_OUTPUT_DSOUND );
		qfmod_setdriver( 0 );

		// NOTE: 32 channels needs for correctly playing tracker music
		if( !qfmod_init( 44100, 32, 0 ))
		{	
			CONPRINT( "%s\n", FMOD_ErrorString( qfmod_geterror( )));
			return 1;
		}

		qfmod_setmixer( FSOUND_MIXER_AUTODETECT );
	}
	else CONPRINT( "fmod.dll not installed\n" );
          
	return 1;
}

int CHudSound::VidInit( void )
{
	if( fmod_dll )
	{
		if( CheckFormat( FALSE ) == TRACK )
			qfmod_freesong( fmod_data );
		else if( CheckFormat( FALSE ) == STREAM )
			qfmod_freestream( fmod_data );

		songname[0] = barename[0] = '\0';
		fmod_data = song_data = NULL;
		gEngfuncs.COM_FreeFile( song_data );
	}

	m_flVolume = -1.0f;
	return 1;
}

int CHudSound::MsgFunc_Fsound( const char *pszName, int iSize, void *pbuf )
{
	if ( !fmod_dll ) return 1; //check handler

	BEGIN_READ( pbuf, iSize );
	char readname[256];

	strncpy( readname, READ_STRING( ), sizeof( readname ));
	m_iStatus = READ_BYTE();

	if( !strcmp( barename, readname ) && m_iStatus != 0 )
	{
		// ForceClientDll update or somewhat, ignore it
		return 1;
	}

	sprintf( songname, "media/%s", readname );
	strncpy( barename, readname, sizeof( barename ) - 1 );	// used for checks
	songname[sizeof( songname ) - 1] = '\0';
	
	if( m_iStatus & 1 )
	{
		PlayStream( songname );
	}
	else if ( fmod_data )
	{
		if( CheckFormat( 0 ) == TRACK )
			qfmod_freesong( fmod_data );
		else if( CheckFormat( 0 ) == STREAM )
			qfmod_freestream( fmod_data );

		songname[0] = barename[0] = '\0';
		gEngfuncs.COM_FreeFile( song_data );
		fmod_data = song_data = NULL;
		m_iStatus = 0;
	}
	return 1;
}

int CHudSound::PlayStream( const char* name )
{
	int filesize;
	
	if (!fmod_dll) return false; //check handler
	// Load the file
	song_data = (char *)gEngfuncs.COM_LoadFile((char *)name, 5, &filesize);

	if (!song_data) 
	{
		CONPRINT( "Couldn't load %s\n", name );
		return false;
          }

	// try to open this file as stream default
	int flags = FSOUND_LOADMEMORY | FSOUND_MPEGACCURATE;

	if( m_iStatus & 2 )
		flags |=  FSOUND_LOOP_NORMAL;
	else
		flags |= FSOUND_LOOP_OFF;

	fmod_data = qfmod_loadstream( song_data, flags, filesize );
	m_flVolume = -1.0f;

	if ( !fmod_data ) //may be it's tracker?
	{
		//check for .xm .it .s3m headers
		if ( memcmp( song_data, "Extended Module:", 16 ) && memcmp( song_data, "IMPM", 4 ) && memcmp( song_data + 44, "SCRM", 4 ))
		{
			CONPRINT( "%s is not a %s file\n", name, UTIL_FileExtension( name ));
			gEngfuncs.COM_FreeFile( song_data );
			song_data = NULL;
			return 1;
		}

		// it's tracker, try to load
		fmod_data = qfmod_loadsongmemory( song_data, filesize );

		if (!fmod_data) //what's hell?
		{
			CONPRINT( "%s\n", FMOD_ErrorString( qfmod_geterror( )));
			gEngfuncs.COM_FreeFile( song_data );
			song_data = NULL;
			return 1;
		}

		qfmod_playsong( fmod_data );
		gEngfuncs.COM_FreeFile( song_data );
		song_data = NULL;
		return 1;
	}

	qfmod_playstream( 0, fmod_data );
	m_iStatus = 1;
	return 1;
}

int CHudSound::Draw( float flTime )
{
	if( !fmod_dll || !fmod_data )
		return 0;

	if (last_state != pause)
	{
		// detect of music type
		if( CheckFormat( 0 ) == TRACK )
			qfmod_setmodpause( fmod_data, !qfmod_getmodpause( fmod_data ));
		else if( CheckFormat( 0 ) == STREAM )
			qfmod_setstreampause( 0, !qfmod_getstreampause( 0 ) );
		last_state = pause;
	}

	// g-cont. this is compatible with GoldSrc and Xash3D
	float vol = CVAR_GET_FLOAT( "musicvolume" );	// sound engine cvar

	if( vol != m_flVolume )
	{
		if( CheckFormat( FALSE ) == TRACK )
			qfmod_setmodvolume( fmod_data, (int)(vol * 256.f ));
		else if( CheckFormat( FALSE ) == STREAM )
			qfmod_setvolume( FSOUND_ALL, (int)(vol * 256.f ));
		m_flVolume = vol;
	}

	return 1;
}

int CHudSound::Close( void )
{
	if ( fmod_dll )
	{
		qfmod_stopallsongs(); //stop all songs
		qfmod_close();
		Sys_UnloadLibrary( &fmod_dll ); //free library
	}	
	return 1;
}