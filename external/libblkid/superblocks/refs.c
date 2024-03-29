/*
 * Copyright (C) 2013 Karel Zak <kzak@redhat.com>
 *
 * This file may be redistributed under the terms of the
 * GNU Lesser General Public License.
 */

#include <string.h>
#include "superblocks.h"


const struct blkid_idinfo refs_idinfo =
{
	.name		= "ReFS",
	.usage		= BLKID_USAGE_FILESYSTEM,
	.magics		=
	{
		{ .magic = "\000\000\000ReFS\000", .len = 8 },
		{ .magic = NULL, .len = 0 }
	}
};

