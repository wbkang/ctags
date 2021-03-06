/*
*
*   Copyright (c) 2015, Red Hat, Inc.
*   Copyright (c) 2015, Masatake YAMATO
*
*   This source code is released for free distribution under the terms of the
*   GNU General Public License version 2 or (at your option) any later version.
*
*   This module contains functions for invoking external command.
*   Half of codes are derived from lregex.c.
*   Core data structure is taken from readtags.h.
*
*/

/*
*   INCLUDE FILES
*/
#include "general.h"  /* must always come first */

#include <ctype.h>
#include <string.h>

#include "entry.h"
#include "iniconf.h"
#include "read.h"
#include "routines.h"
#include "vstring.h"
#include "xtag.h"



/*
*   DATA DEFINITIONS
*/
typedef enum {
	K_UNIT,
} systemdUnitKind;

typedef enum {
	R_UNIT_Requires,
	R_UNIT_Wants,
	R_UNIT_After,
	R_UNIT_Before,
	R_UNIT_RequiredBy,
	R_UNIT_WantedBy,

} systemdUnitRole;

static roleDesc SystemdUnitUnitRoles [] = {
	{ true, "Requires", "referred in Requires key" },
	{ true, "Wants", "referred in Wants key" },
	{ true, "After", "referred in After key" },
	{ true, "Before", "referred in Before key" },
	{ true, "RequiredBy", "referred in RequiredBy key" },
	{ true, "WantedBy", "referred in WantedBy key" },
	/* ... */
};

static kindDefinition SystemdUnitKinds [] = {
	{ true, 'u', "unit", "units",
	  .referenceOnly = true, ATTACH_ROLES(SystemdUnitUnitRoles)},
};

static int roleOf (const char* key, kindDefinition* kind)
{
	int i;

	for (i = 0; i < kind->nRoles; i++)
	{
		if (strcmp (kind->roles[i].name, key) == 0)
			return i;
	}

	return -1;
}

static void makeSystemdReferencedUnit (const char *value, kindDefinition* kind, int role)
{
	vString *unit = vStringNew ();

	while (*value != '\0')
	{
		if (*value == ',')
		{
			makeSimpleRefTag (unit, kind, 0, role);
			vStringClear (unit);
		}
		else if (! isspace ((int) *value))
			vStringPut (unit, *value);

		value++;
	}

	if (vStringLength (unit) > 0)
		makeSimpleRefTag (unit, kind, 0, role);
	vStringDelete (unit);
}

static void newDataCallback (iniconfSubparser *s,
							 const char *section, const char *key, const char *value)
{
	int r;

	if (isXtagEnabled (XTAG_REFERENCE_TAGS) && value)
	{
		r = roleOf (key, SystemdUnitKinds + K_UNIT);
		if (r >= 0)
			makeSystemdReferencedUnit (value, SystemdUnitKinds + K_UNIT, r);
	}
}

static void findSystemdUnitTags (void)
{
	scheduleRunningBaseparser (0);
}

extern parserDefinition* SystemdUnitParser (void)
{
	static const char *const extensions [] = { "unit", "service", "socket", "device",
											   "mount", "automount", "swap", "target",
											   "path", "timer", "snapshot", "scope",
											   "slice", "time", /* ... */
											   NULL };
	static iniconfSubparser systemdUnitSubparser = {
		.subparser = {
			.direction = SUBPARSER_SUB_RUNS_BASE,
		},
		.newDataNotify = newDataCallback,
	};
	static parserDependency dependencies [] = {
		[0] = { DEPTYPE_SUBPARSER, "Iniconf", &systemdUnitSubparser },
	};

	parserDefinition* const def = parserNew ("SystemdUnit");

	def->dependencies = dependencies;
	def->dependencyCount = ARRAY_SIZE(dependencies);
	def->kindTable      = SystemdUnitKinds;
	def->kindCount  = ARRAY_SIZE (SystemdUnitKinds);
	def->extensions = extensions;
	def->parser     = findSystemdUnitTags;
	def->useCork    = true;
	return def;
}
