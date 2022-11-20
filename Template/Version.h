﻿#pragma once

#define PLUGIN_VERSION_DEV     0
#define PLUGIN_VERSION_BETA    1
#define PLUGIN_VERSION_RELEASE 2

// Plugin Information, used in dllmain.cpp for register plugin
#define PLUGIN_NAME                 "TreeCuttingAndMining"
#define PLUGIN_INTRODUCTION         "砍树与挖矿插件"
#define PLUGIN_AUTHOR               "CNGEGE"
#define PLUGIN_VERSION_MAJOR        2
#define PLUGIN_VERSION_MINOR        1
#define PLUGIN_VERSION_REVISION     1
#define PLUGIN_VERSION_BUILD        0
#define PLUGIN_VERSION_STATUS       PLUGIN_VERSION_RELEASE

//#define TARGET_BDS_PROTOCOL_VERSION 503
// used by github actions(TODO)
//#define TARGET_BDS_VERSION          1.18.30.04

// File Version Information, used in Resource.rc
#define __TO_VERSION_STRING(ver) #ver
#define TO_VERSION_STRING(ver) __TO_VERSION_STRING(ver)

#if PLUGIN_VERSION_STATUS == PLUGIN_VERSION_BETA
#define PLUGIN_FILE_VERSION_FLAG    VS_FF_DEBUG
#define PLUGIN_LLVERSION_STATUS     ll::Version::Beta
#define PLUGIN_FILE_VERSION_STRING  TO_VERSION_STRING(PLUGIN_VERSION_MAJOR.PLUGIN_VERSION_MINOR.PLUGIN_VERSION_REVISION.PLUGIN_VERSION_ACTIONS BETA)
#elif PLUGIN_VERSION_STATUS == PLUGIN_VERSION_DEV
#define PLUGIN_FILE_VERSION_FLAG    VS_FF_DEBUG
#define PLUGIN_LLVERSION_STATUS     ll::Version::Dev
#define PLUGIN_FILE_VERSION_STRING  TO_VERSION_STRING(PLUGIN_VERSION_MAJOR.PLUGIN_VERSION_MINOR.PLUGIN_VERSION_REVISION.PLUGIN_VERSION_ACTIONS DEV)
#else
#define PLUGIN_FILE_VERSION_FLAG    0x0L
#define PLUGIN_LLVERSION_STATUS     ll::Version::Release
#define PLUGIN_FILE_VERSION_STRING  TO_VERSION_STRING(PLUGIN_VERSION_MAJOR.PLUGIN_VERSION_MINOR.PLUGIN_VERSION_REVISION.PLUGIN_VERSION_ACTIONS)
#endif

#define FILE_VERSION_BLOCK_HEADER           0x04004B0L
#define FILE_VERSION_COMPANY_NAME           PLUGIN_AUTHOR
#define FILE_VERSION_LEGAL_COPYRIGHT        "Copyright (C) 2022"
#define FILE_VERSION_FILE_DESCRIPTION       PLUGIN_INTRODUCTION
#define FILE_VERSION_FILE_VERSION_STRING    PLUGIN_FILE_VERSION_STRING
#define FILE_VERSION_INTERNAL_NAME          PLUGIN_NAME
#define FILE_VERSION_ORIGINAL_FILENAME      PLUGIN_NAME ".dll"
#define FILE_VERSION_PRODUCT_NAME           FILE_VERSION_INTERNAL_NAME
#define FILE_VERSION_PRODUCT_VERSION_STRING PLUGIN_FILE_VERSION_STRING
#define FILE_VERSION_FILE_VERSION           PLUGIN_VERSION_MAJOR, PLUGIN_VERSION_MINOR, PLUGIN_VERSION_REVISION, PLUGIN_VERSION_BUILD
#define FILE_VERSION_PRODUCT_VERSION        FILE_VERSION_FILE_VERSION
