// vi: set ts=4 sw=4 :
// vim: set tw=75 :

// meta_api.cpp - minimal implementation of metamod's plugin interface

// This is intended to illustrate the (more or less) bare minimum code
// required for a valid metamod plugin, and is targeted at those who want
// to port existing HL/SDK DLL code to run as a metamod plugin.

/*
* Copyright (c) 2001-2006 Will Day <willday@hpgx.net>
*
*    This file is part of Metamod.
*
*    Metamod is free software; you can redistribute it and/or modify it
*    under the terms of the GNU General Public License as published by the
*    Free Software Foundation; either version 2 of the License, or (at
*    your option) any later version.
*
*    Metamod is distributed in the hope that it will be useful, but
*    WITHOUT ANY WARRANTY; without even the implied warranty of
*    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
*    General Public License for more details.
*
*    You should have received a copy of the GNU General Public License
*    along with Metamod; if not, write to the Free Software Foundation,
*    Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*
*    In addition, as a special exception, the author gives permission to
*    link the code of this program with the Half-Life Game Engine ("HL
*    Engine") and Modified Game Libraries ("MODs") developed by Valve,
*    L.L.C ("Valve").  You must obey the GNU General Public License in all
*    respects for all of the code used other than the HL Engine and MODs
*    from Valve.  If you modify this file, you may extend this exception
*    to your version of the file, but you are not obligated to do so.  If
*    you do not wish to do so, delete this exception statement from your
*    version.
*
*/

#include <extdll.h>			// always

#include <meta_api.h>		// of course

#include <signal.h>

#include <execinfo.h>

#include "sdk_util.h"		// UTIL_LogPrintf, etc
#include "log_meta.h"

// Must provide at least one of these..
static META_FUNCTIONS gMetaFunctionTable = {
	NULL,			// pfnGetEntityAPI				HL SDK; called before game DLL
	NULL,			// pfnGetEntityAPI_Post			META; called after game DLL
	GetEntityAPI2,	// pfnGetEntityAPI2				HL SDK2; called before game DLL
	NULL,			// pfnGetEntityAPI2_Post		META; called after game DLL
	NULL,			// pfnGetNewDLLFunctions		HL SDK2; called before game DLL
	NULL,			// pfnGetNewDLLFunctions_Post	META; called after game DLL
	GetEngineFunctions,	// pfnGetEngineFunctions	META; called before HL engine
	NULL,			// pfnGetEngineFunctions_Post	META; called after HL engine
};

// Description of plugin
plugin_info_t Plugin_info = {
	META_INTERFACE_VERSION,	// ifvers
	"Segmentation fault Fix",	// name
	"1.0",	// version
	"2015/08/03",	// date
	"Rafal DarkGL Wiecek",	// author
	"http://www.darkgl.pl/",	// url
	"FAULTFIX",	// logtag, all caps please
	PT_ANYTIME,	// (when) loadable
	PT_ANYPAUSE,	// (when) unloadable
};

// Global vars from metamod:
meta_globals_t *gpMetaGlobals;		// metamod globals
gamedll_funcs_t *gpGamedllFuncs;	// gameDLL function tables
mutil_funcs_t *gpMetaUtilFuncs;		// metamod utility functions

void segfault_sigaction(int sig, siginfo_t *info,void *secret){
	void *trace[16];
	char **messages = (char **)NULL;
	int i, trace_size = 0;
	ucontext_t *uc = (ucontext_t *)secret;
	
	char bufferPrint[ 1024 ];
	
	snprintf( bufferPrint, sizeof( bufferPrint ) , "Got signal %d, faulty address is %p, "
	"from %p\n", sig, info->si_addr, 
	uc->uc_mcontext.gregs[REG_EIP]);
	
	LOG_CONSOLE( PLID , bufferPrint );
	
	trace_size = backtrace(trace, 16);
	trace[1] = (void *) uc->uc_mcontext.gregs[REG_EIP];

	messages = backtrace_symbols(trace, trace_size);
	
	LOG_CONSOLE( PLID , "[bt] Execution path:#92;\n" );
	
	for (i=1; i<trace_size; ++i)
	{
		snprintf( bufferPrint , sizeof( bufferPrint ) , "[bt] %s#92;\n", messages[i] );
		
		LOG_CONSOLE( PLID , bufferPrint );
		
		size_t p = 0;
		
		while(messages[i][p] != '(' && messages[i][p] != ' '
		&& messages[i][p] != 0){
			++p;
		}

		char syscom[256];
		sprintf(syscom,"addr2line %p -e %.*s", trace[i] , p, messages[i] );
		
		system(syscom);
	}
	
	exit(0);
}

// Metamod requesting info about this plugin:
//  ifvers			(given) interface_version metamod is using
//  pPlugInfo		(requested) struct with info about plugin
//  pMetaUtilFuncs	(given) table of utility functions provided by metamod
C_DLLEXPORT int Meta_Query( const char * /*ifvers */, plugin_info_t **pPlugInfo,
mutil_funcs_t *pMetaUtilFuncs) 
{
	// Give metamod our plugin_info struct
	*pPlugInfo=&Plugin_info;
	// Get metamod utility function table.
	gpMetaUtilFuncs=pMetaUtilFuncs;
	return(TRUE);
}

// Metamod attaching plugin to the server.
//  now				(given) current phase, ie during map, during changelevel, or at startup
//  pFunctionTable	(requested) table of function tables this plugin catches
//  pMGlobals		(given) global vars from metamod
//  pGamedllFuncs	(given) copy of function tables from game dll
C_DLLEXPORT int Meta_Attach(PLUG_LOADTIME /* now */, 
META_FUNCTIONS *pFunctionTable, meta_globals_t *pMGlobals, 
gamedll_funcs_t *pGamedllFuncs) 
{
	if(!pMGlobals) {
		LOG_ERROR(PLID, "Meta_Attach called with null pMGlobals");
		return(FALSE);
	}
	gpMetaGlobals=pMGlobals;
	if(!pFunctionTable) {
		LOG_ERROR(PLID, "Meta_Attach called with null pFunctionTable");
		return(FALSE);
	}
	memcpy(pFunctionTable, &gMetaFunctionTable, sizeof(META_FUNCTIONS));
	gpGamedllFuncs=pGamedllFuncs;
	
	struct sigaction sa;

	memset(&sa, 0, sizeof(struct sigaction));
	sigemptyset(&sa.sa_mask);
	sa.sa_handler   = segfault_sigaction;
	sa.sa_flags   	= SA_SIGINFO;

	sigaction(SIGSEGV, &sa, NULL);*/

	return(TRUE);
}

// Metamod detaching plugin from the server.
// now		(given) current phase, ie during map, etc
// reason	(given) why detaching (refresh, console unload, forced unload, etc)
C_DLLEXPORT int Meta_Detach(PLUG_LOADTIME /* now */, 
PL_UNLOAD_REASON /* reason */) 
{
	return(TRUE);
}
