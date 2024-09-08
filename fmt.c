// BadWolf: Minimalist and privacy-oriented WebKitGTK+ browser
// SPDX-FileCopyrightText: 2019-2022 Badwolf Authors <https://hacktivis.me/projects/badwolf>
// SPDX-License-Identifier: BSD-3-Clause

#include "fmt.h"

/* flawfinder: ignore. `alpha_digits` is never modified */
static const char alpha_digits[26] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
void
fmt_context_id(uint64_t num, char *out)
{
	/* flawfinder: ignore. bound checks are done */
	char buf[BADWOLF_CTX_SIZ] = {0, 0, 0, 0, 0, 0, 0};
	int len                   = 0;

	buf[++len] = ' ';
	buf[++len] = ':';

	buf[++len] = alpha_digits[num % 26];
	num /= 26;
	while(num > 0 && len < (BADWOLF_CTX_SIZ - 1))
	{
		buf[++len] = alpha_digits[(num - 1) % 26];
		num /= 26;
	}

	for(int i = 0; i < len; i++)
	{
		out[i] = buf[len - i];
	}
}
