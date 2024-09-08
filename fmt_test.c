// BadWolf: Minimalist and privacy-oriented WebKitGTK+ browser
// SPDX-FileCopyrightText: 2019-2022 Badwolf Authors <https://hacktivis.me/projects/badwolf>
// SPDX-License-Identifier: BSD-3-Clause

#include "fmt.h"

#include <glib.h>
#include <inttypes.h> // PRIu64
#include <stdint.h>   // UINT64_C

static void
fmt_context_id_test(void)
{
	struct
	{
		const char *expect;
		uint64_t context_id;
	} cases[] = {
	    //
	    {"A: ", UINT64_C(0)},
	    {"B: ", UINT64_C(1)},
	    {"Y: ", UINT64_C(24)},
	    {"Z: ", UINT64_C(25)},
	    {"AA: ", UINT64_C(26)},
	    {"AB: ", UINT64_C(27)},
	    {"AZ: ", UINT64_C(51)},
	    {"BA: ", UINT64_C(52)},
	    {"BB: ", UINT64_C(53)},
	    {"QKWW: ", UINT64_C(4294967296)},           // 2^32
	    {"JFHI: ", UINT64_C(9223372036854775808)},  // 2^63
	    {"TLPO: ", UINT64_C(18446744073709551614)}, // 2^64 -2
	    {"TLPP: ", UINT64_C(18446744073709551615)}, // 2^64 -1 (aka UINT64_MAX)
	};

	for(size_t i = 0; i < sizeof(cases) / sizeof(cases[0]); i++)
	{
		g_info("fmt_context_id(%" PRIu64 ")", cases[i].context_id);
		/* flawfinder: ignore. bound checks are done */
		char got[BADWOLF_CTX_SIZ] = {0, 0, 0, 0, 0, 0, 0};

		fmt_context_id(cases[i].context_id, got);

		if(strncmp(got, cases[i].expect, BADWOLF_CTX_SIZ) != 0)
		{
			g_error("expected: \"%s\", got: \"%s\"", cases[i].expect, got);
		}
	}
}

int
main(int argc, char *argv[])
{
	g_test_init(&argc, &argv, NULL);

	g_test_add_func("/fmt_context_id/test", fmt_context_id_test);

	return g_test_run();
}
