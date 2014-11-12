#!/bin/sh
xasm asap/players/cmc.asx /q /o:cmc.obx
xasm asap/players/mpt.asx /q /o:mpt.obx
xasm asap/players/rmt.asx /d:STEREOMODE=0 /q /o:rmt4.obx
xasm asap/players/rmt.asx /d:STEREOMODE=1 /q /o:rmt8.obx
xasm asap/players/tmc.asx /q /o:tmc.obx
xasm asap/players/tm2.asx /q /o:tm2.obx
perl asap/raw2c.pl cmc.obx mpt.obx rmt4.obx rmt8.obx tmc.obx tm2.obx >players.h
rm *.obx
