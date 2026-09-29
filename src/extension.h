/**
 * vim: set ts=4 :
 * =============================================================================
 * SourceMod Dynamic Targeting Extension
 * Copyright (C) 2004-2008 AlliedModders LLC.  All rights reserved.
 * =============================================================================
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License, version 3.0, as published by the
 * Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * As a special exception, AlliedModders LLC gives you permission to link the
 * code of this program (as well as its derivative works) to "Half-Life 2," the
 * "Source Engine," the "SourcePawn JIT," and any Game MODs that run on software
 * by the Valve Corporation.  You must obey the GNU General Public License in
 * all respects for all other code used.  Additionally, AlliedModders LLC grants
 * this exception to all derivative works.  AlliedModders LLC defines further
 * exceptions, found in LICENSE.txt (as of this writing, version JULY-31-2007),
 * or <http://www.sourcemod.net/license.php>.
 *
 * Version: $Id$
 */

#ifndef _INCLUDE_SOURCEMOD_EXTENSION_PROPER_H_
#define _INCLUDE_SOURCEMOD_EXTENSION_PROPER_H_

/**
 * @file extension.h
 * @brief Dynamic Targeting extension header.
 */

#include "smsdk_ext.h"

/**
 * Watches every command target lookup through SourceMod's public
 * ICommandTargetProcessor interface. Core asks target processors about a
 * pattern right before it falls back to partial name matching, so this sees
 * every lookup made by any plugin (ProcessTargetString, FindTarget, ...)
 * without core or those plugins having to change.
 *
 * It never resolves a target itself: when the lookup is about to fail with
 * COMMAND_TARGET_AMBIGUOUS, it fires DynamicTargeting_OnAmbiguousTarget and
 * lets core carry on as usual.
 */
class DynamicTargeting :
	public SDKExtension,
	public ICommandTargetProcessor
{
public: // SDKExtension
	bool SDK_OnLoad(char *error, size_t maxlength, bool late) override;
	void SDK_OnUnload() override;
public: // ICommandTargetProcessor
	bool ProcessCommandTarget(cmd_target_info_t *info) override;
};

#endif // _INCLUDE_SOURCEMOD_EXTENSION_PROPER_H_
