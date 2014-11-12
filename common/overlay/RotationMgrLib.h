/**
 * Public API for the Mobile Stream Rotation extension.
 *
 * Copyright (c) 2005 Mobile Stream (http://www.mobile-stream.com)
 *
 * Applicable:
 *  - Garmin iQue 3600 - will be in v1.2
 *  - Garmin iQue 3600a  - may be TBD (PalmOS 5.4 but apparently no rotation support?)
 *  - Sony CLIE PEG-NX60
 *  - Sony CLIE PEG-NX70V
 *  - Sony CLIE PEG-NX73
 *  - Sony CLIE PEG-NX80
 *  - Sony CLIE PEG-NZ90
 *  - Sony CLIE PEG-TH55
 *  - Sony CLIE PEG-UX40
 *  - Sony CLIE PEG-UX50
 *  - Sony CLIE PEG-VZ90 - may be TBD in v1.3 (missing PenInputMgr API)
 *
 * Last edited: 2 May 2005 (v1.1).
 */

#ifndef __ROTATIONMGRLIB_H__
#define __ROTATIONMGRLIB_H__

#include <PalmTypes.h>
#include <SystemMgr.h>


/************************************************************
 * Library type and creator.
 *************************************************************/

#ifndef sysFileTExtensionARM
#define sysFileTExtensionARM		'aext'
#endif

#define kRotationMgrLibCreator		'RotM'
#define kRotationMgrLibType		sysFileTExtensionARM
#define kRotationMgrLibName		"RotationMgr"

/* This has nothing to do with PalmSource Pen Input Manager API version. */
#define kRotationMgrLibAPIVersion11	sysMakeROMVersion(1, 1, 0, sysROMStageRelease, 0)
#define kRotationMgrLibAPIVersion12	sysMakeROMVersion(1, 2, 0, sysROMStageRelease, 0)

/* Current API version. */
#define kRotationMgrLibAPIVersion	kRotationMgrLibAPIVersion11


/************************************************************
 * ARM entry point numbers.
 *************************************************************/

#define entryNumRotationMgrLibOpen		0x00	/* 0 */
#define entryNumRotationMgrLibClose		0x01	/* 4 */
#define entryNumRotationMgrGetLibAPIVersion	0x02	/* 8 */
#define entryNumRotationMgrAppPrefsSet		0x03	/* 12 */
#define entryNumRotationMgrAppPrefsGet		0x04	/* 16 */
#define entryNumRotationMgrAppPrefsEnumerate	0x05	/* 20 */
#define entryNumRotationMgrAttributeSet		0x06	/* 24 */
#define entryNumRotationMgrAttributeGet		0x07	/* 28 */


/************************************************************
 * 68K library trap numbers.
 *************************************************************/

#define kRotationMgrLibTrapOpen			(sysLibTrapOpen)
#define kRotationMgrLibTrapClose		(sysLibTrapClose)
#define kRotationMgrLibTrapSleep		(sysLibTrapSleep)
#define kRotationMgrLibTrapWake			(sysLibTrapWake)
#define kRotationMgrLibTrapGetLibAPIVersion	(sysLibTrapCustom + 0)
#define kRotationMgrLibTrapAppPrefsSet		(sysLibTrapCustom + 1)
#define kRotationMgrLibTrapAppPrefsGet		(sysLibTrapCustom + 2)
#define kRotationMgrLibTrapAppPrefsEnumerate	(sysLibTrapCustom + 3)
#define kRotationMgrLibTrapAttributeSet		(sysLibTrapCustom + 4)
#define kRotationMgrLibTrapAttributeGet		(sysLibTrapCustom + 5)


/************************************************************
 * Notifications. Same as on Tungsten|T3, T|T5 and T|X.
 *************************************************************/

/* This is sent right before rotation change. */
#define kRotationMgrBeforeRotationChangeEvent	'Rchg'

/* This is sent right after rotation change. */
#define kRotationMgrAfterRotationChangeEvent	'RChg'

/*
 * notifyDetailsP of above notifications will point to this structure.
 * All fields are in big-endian format.
 */
typedef struct RotationMgrRotationChangeDetailsType {
	UInt16	newRotation;
	UInt16	oldRotation;
} RotationMgrRotationChangeDetailsType;


/************************************************************
 * Error codes.
 *************************************************************/

/* FIXME */
#define kRotationMgrErrorClass		(oemErrorClass + 0x700)

#define kRotationMgrErrBadParam		(kRotationMgrErrorClass + 0)
#define kRotationMgrErrNotAllowed	(kRotationMgrErrorClass + 1)
#define kRotationMgrErrNotEnoughSpace	(kRotationMgrErrorClass + 2)
#define kRotationMgrErrNotFound		(kRotationMgrErrorClass + 3)


/************************************************************
 * Flags for applications.
 *************************************************************/

/* Preferred orientation for the application. */
#define kRotationMgrAppPrefLaunchDefault	0x0000
#define kRotationMgrAppPrefLaunchLandscape	0x0001
#define kRotationMgrAppPrefLaunchPortrait	0x0002
#define kRotationMgrAppPrefLaunchAuto		0x0003
#define kRotationMgrAppPrefLaunchCW0		0x0004
#define kRotationMgrAppPrefLaunchCW90		0x0005
#define kRotationMgrAppPrefLaunchCW180		0x0006
#define kRotationMgrAppPrefLaunchCW270		0x0007
#define kRotationMgrAppPrefLaunchMask		0x0007

/* Disable orientation trigger for the application. */
#define kRotationMgrAppPrefLocked		0x0008

/*
 * Allow Sony Virtual Silkscreen API for the application.
 * Ignored on Garmin handhelds.
 */
#define kRotationMgrAppPrefSonyAPI		0x0010

/*
 * Allow PalmSource Pen Input Manager API for the application.
 * Garmin iQue 3600 is special - TBD.
 */
#define kRotationMgrAppPrefPalmSourceAPI	0x0020

/* Enable WinDrawBitmap optimisation for fullscreen games. */
#define kRotationMgrAppPrefFastFullscreen	0x0040

/* User should not change rotation settings for this application. */
#define kRotationMgrAppPrefDangerous		0x8000

/* Default preferences. */
#define kRotationMgrAppPrefsDefault			\
		(kRotationMgrAppPrefLaunchDefault |	\
		 kRotationMgrAppPrefPalmSourceAPI |	\
		 kRotationMgrAppPrefSonyAPI)


/************************************************************
 * Preferenced applications iterator start and stop constants.
 *************************************************************/

#define kRotationMgrAppPrefsIteratorStart	0
#define kRotationMgrAppPrefsIteratorStop	0xffffffffL


/************************************************************
 * Global attributes of RotationMgr extension.
 *************************************************************/

/* Trigger type. */
#define kRotationMgrAttrTriggerType		0

enum {
	kRotationMgrTriggerNone,
	kRotationMgrTriggerStatusBar,

	/* Not currently implemented and probably never will. */
	kRotationMgrTriggerButton
};

/* Trigger will switch between 0 and x degrees. */
#define kRotationMgrAttrTriggerRotation		1
enum {
	kRotationMgrTriggerLeft,		/* 270 degrees */
	kRotationMgrTriggerRight,		/* 90 degrees */
	kRotationMgrTriggerBoth			/* two triggers */
};

/* Real framebuffer base address. Not settable. */
#define kRotationMgrAttrDisplayAddress		0x800

/* Physical display size. Not settable */
#define kRotationMgrAttrDisplayWidth		0x801
#define kRotationMgrAttrDisplayHeight		0x802
#define kRotationMgrAttrDisplayRowBytes		0x803

/* Retrieve the registration ID/pass the registration code. */
#define kRotationMgrAttrRegistration		0x8000

#if CPU_TYPE == CPU_68K

/************************************************************
 * Standard library APIs.
 *************************************************************/

Err RotationMgrLibOpen(UInt16 refNum)	SYS_TRAP(kRotationMgrLibTrapOpen);
Err RotationMgrLibClose(UInt16 refNum)	SYS_TRAP(kRotationMgrLibTrapClose);
Err RotationMgrLibSleep(UInt16 refNum)	SYS_TRAP(kRotationMgrLibTrapSleep);
Err RotationMgrLibWake(UInt16 refNum)	SYS_TRAP(kRotationMgrLibTrapWake);


/************************************************************
 * Get Mobile Stream RotationMgr API version.
 *************************************************************/

UInt32 RotationMgrGetLibAPIVersion(UInt16 refNum)
					SYS_TRAP(kRotationMgrLibTrapGetLibAPIVersion);


/************************************************************
 * Application preferences API.
 *************************************************************/

/*
 * appType is either sysFileTPanel or sysFileTApplication.
 */
Err RotationMgrAppPrefsSet(UInt16 refNum,
		UInt32 appType, UInt32 appCreator, UInt32 prefs)
					SYS_TRAP(kRotationMgrLibTrapAppPrefsSet);

Err RotationMgrAppPrefsGet(UInt16 refNum,
		UInt32 appType, UInt32 appCreator, UInt32 *prefsP)
					SYS_TRAP(kRotationMgrLibTrapAppPrefsGet);

Err RotationMgrAppPrefsEnumerate(UInt16 refNum, UInt32 *appPrefsIteratorP,
		UInt32 *appTypeP, UInt32 *appCreatorP, UInt32 *prefsP)
					SYS_TRAP(kRotationMgrLibTrapAppPrefsEnumerate);


/************************************************************
 * RotationMgr global attributes API.
 *************************************************************/

Err RotationMgrAttributeSet(UInt16 refNum, UInt32 attribute, UInt32 value)
					SYS_TRAP(kRotationMgrLibTrapAttributeSet);

Err RotationMgrAttributeGet(UInt16 refNum, UInt32 attribute, UInt32 *valueP)
					SYS_TRAP(kRotationMgrLibTrapAttributeGet);

#else /* CPU_TYPE != CPU_68K */

extern UInt32 RotationMgrGetLibAPIVersion(void);
extern Err RotationMgrAppPrefsSet(UInt32 appType, UInt32 appCreator, UInt32 prefs);
extern Err RotationMgrAppPrefsGet(UInt32 appType, UInt32 appCreator, UInt32 *prefsP);
extern Err RotationMgrAppPrefsEnumerate(Int32 *appPrefsIteratorP,
		UInt32 *appTypeP, UInt32 *appCreatorP, UInt32 *prefsP);
extern Err RotationMgrAttributeSet(UInt32 attribute, UInt32 value);
extern Err RotationMgrAttributeGet(UInt32 attribute, UInt32 *valueP);

#endif /* CPU_TYPE == CPU_68K */

#endif
