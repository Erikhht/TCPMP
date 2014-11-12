/*****************************************************************************
 *
 * This program is free software ; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
 *
 * $Id: resources.h 615 2006-01-26 16:57:51Z picard $
 *
 * The Core Pocket Media Player
 * Copyright (c) 2004-2005 Gabor Kovacs
 *
 ****************************************************************************/

#define MainForm                                1000
#define MainBenchmark							1001
#define MainBenchmarkForm						1002
#define MainPrev			                    1003
#define MainStop			                    1004
#define MainPlay			                    1005
#define MainNext			                    1006
#define MainVolume								1007
#define MainPosition							1008
#define MainPause			                    1011
#define MainAbout			                    1015
#define MainMenu								1019
#define MainFullScreen							1020
#define MainViewport							1025
#define MainMediaInfo							1026
#define MainTitle								1028
#define MainOpenFiles							1037
#define MainSearchFiles							1038
#define MainPref								1039
#define MainQuit			                    1040
#define MainOpen			                    1041
#define MainTimer								1042
#define MainMenuPref							1043
#define MainWinTitle							1044
#define MainClear			                    1045
#define MainMenuTweaks							1046
#define MainMenuAdv								1047
#define MainMenuEq								1048

#define AboutForm								1100
#define AboutOSVersion							1102
#define AboutOEMInfo							1103
#define AboutCPU								1104
#define AboutMem1								1105
#define AboutMem2								1106
#define AboutVersion							1107

#define MediaInfoForm		1200
#define MediaInfoURL		1201
#define MediaVideoCodec		1203
#define MediaVideoFormat	1204
#define MediaVideoTotal		1205 
#define MediaVideoDropped	1206 
#define MediaAudioCodec		1207 
#define MediaAudioFormat	1208 
#define MediaInfoFormat		1209

#define BenchmarkForm		1300
#define BenchmarkSpeed		1302
#define BenchmarkTime		1303
#define BenchmarkOrigTime	1304
#define BenchmarkFPS		1305
#define BenchmarkOrigFPS	1306

#define OpenForm			1400
#define OpenURL				1401
#define OpenList			1402
#define OpenScroll			1405
#define OpenNone			1403
#define OpenAll				1404
#define OpenMime			1406
#define OpenMimeList		1407
#define OpenOk				1408
#define OpenCancel			1409
#define OpenImageUp			1410	
#define OpenImageDir		1411
#define OpenImageFile		1412
#define OpenImageAudio		1413
#define OpenImageVideo		1414
#define OpenImagePlayList	1415
#define OpenImageChecked	1416
#define OpenImageUnchecked	1417
#define OpenLine			1418

#define PrefForm			1500
#define PrefGeneral			1501
#define PrefVideo			1502
#define PrefAudio			1503
#define PrefClose			1504

#define PrefGeneralRepeat	1520
#define PrefGeneralShuffle	1521
#define PrefGeneralSpeedLabel 1522
#define PrefGeneralSpeed	1523
#define PrefGeneralSpeedOrig	1524
#define PrefGeneralSpeedCustom	1525
#define PrefGeneralMoveFFwdLabel 1526
#define PrefGeneralMoveFFwd		 1527
#define PrefGeneralMoveFFwdLabel2 1532
#define PrefGeneralMoveBackLabel 1528
#define PrefGeneralMoveBack		 1529
#define PrefGeneralMoveBackLabel2 1533
#define PrefGeneralAVOffsetLabel 1530
#define PrefGeneralAVOffset		 1531
#define PrefGeneralAVOffsetLabel2 1534
#define PrefGeneralSpeedValue	1535

#define PrefAudioDisable	 1540
#define PrefAudioVolume		 1541
#define PrefAudioVolumeLabel 1542
#define PrefAudioPanLabel	 1543
#define PrefAudioPan		 1544
#define PrefAudioPreamp		 1545
#define PrefAudioPreampLabel 1546
#define PrefAudioVolumeValue 1547
#define PrefAudioPanValue	 1548
#define PrefAudioPreampValue 1549
#define PrefAudioStereo		 1550
#define PrefAudioStereoList	 1551
#define PrefAudioPanLabel2	 1552
#define PrefAudioPanReset	 1553
#define PrefAudioPreampReset 1554

#define PrefVideoDriver		1560
#define PrefVideoFullZoom	1561
#define PrefVideoFullRotate	1562
#define PrefVideoSkinZoom	1563
#define PrefVideoSkinRotate	1564
#define PrefVideoDither		1565
#define PrefVideoBrightness	1566
#define PrefVideoContrast	1567
#define PrefVideoSaturation	1568
#define PrefVideoRed		1569
#define PrefVideoGreen		1570
#define PrefVideoBlue		1571
#define PrefVideoFullLabel			1572
#define PrefVideoSkinLabel			1573
#define PrefVideoZoomLabel			1574
#define PrefVideoRotateLabel		1575
#define PrefVideoBrightnessLabel	1576
#define PrefVideoContrastLabel		1577
#define PrefVideoSaturationLabel	1578
#define PrefVideoRedLabel			1579
#define PrefVideoGreenLabel			1580
#define PrefVideoBlueLabel			1581
#define PrefVideoFullZoomList		1582
#define PrefVideoFullRotateList		1583
#define PrefVideoSkinZoomList		1584
#define PrefVideoSkinRotateList		1585
#define PrefVideoBrightnessValue	1590
#define PrefVideoContrastValue		1591
#define PrefVideoSaturationValue	1592
#define PrefVideoRedValue			1593
#define PrefVideoGreenValue			1594
#define PrefVideoBlueValue			1595
#define PrefVideoDriverList			1596
#define PrefVideoQuality			1597
#define PrefVideoQualityList		1598
#define PrefVideoAspect				1700
#define PrefVideoAspectList			1701

#define TweakForm					1600
#define TweakBattery				1601
#define TweakBatteryLabel1			1602
#define TweakBatteryLabel2			1603
#define TweakBatteryLabel3			1610
#define TweakTrippleBuffer			1604
#define TweakTrippleBufferLabel1	1605
#define TweakBorderless				1606
#define TweakBorderlessLabel1		1607
#define TweakBorderlessLabel2		1608
#define TweakMemory					1609

#define	DummyOk				1900
#define DummyCancel			1901

#define SliderBackground	2000
#define SliderThumb			2001

#define AdvForm				2100
#define AdvNoBatteryWarning 2101
#define AdvKeyFollowDir		2102
#define AdvBenchFromPos		2103
#define AdvAVIFrameRate		2104
#define AdvNoEventChecking	2105
#define AdvSmoothZoom		2106
#define AdvCardPlugins		2107
#define AdvPreRotate		2108
#define AdvNoDeblocking		2109
#define AdvBlinkLED			2110

#define EqForm				2200
#define EqEnable			2201
#define EqAttenuate			2202
#define EqPreamp			2203
#define Eq1					2204
#define Eq2					2205
#define Eq3					2206
#define Eq4					2207
#define Eq5					2208
#define Eq6					2209
#define Eq7					2210
#define Eq8					2211
#define Eq9					2212
#define Eq10				2213
#define EqPreampValue		2214
#define Eq1Value			2215
#define Eq2Value			2216
#define Eq3Value			2217
#define Eq4Value			2218
#define Eq5Value			2219
#define Eq6Value			2220
#define Eq7Value			2221
#define Eq8Value			2222
#define Eq9Value			2223
#define Eq10Value			2224
#define EqReset				2225
