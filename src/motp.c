/*-
 * Copyright (c) 2024-2025 Rozhuk Ivan <rozhuk.im@gmail.com>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * Author: Rozhuk Ivan <rozhuk.im@gmail.com>
 *
 */

/* Based on:
 * https://motp.sourceforge.net/motp_js.html
 * https://github.com/itpey/motp/blob/main/motp.go */

#include <sys/param.h>
#include <sys/types.h>

#include <inttypes.h>
#include <stdlib.h> /* malloc, exit */
#include <stdio.h> /* snprintf, fprintf */
#include <unistd.h> /* close, write, sysconf */
#include <string.h> /* bcopy, bzero, memcpy, memmove, memset, strerror... */
#include <errno.h>
#include <err.h>
#include <getopt.h>
#include <libgen.h> /* basename */
#include <time.h> /* strptime */
#include <locale.h>
#ifdef BSD
#	include <xlocale.h> /* strptime_l */
#endif

#include "crypto/hash/md5.h"


#ifndef PACKAGE_STRING
#	define PACKAGE_STRING		"motp"
#endif
#ifndef PACKAGE_DESCRIPTION
#	define PACKAGE_DESCRIPTION	"Simple console mOTP tool"
#endif


typedef struct command_line_options_s {
	int		verbose;
	const char	*secret;
	const char	*pin;
	uint32_t	duration;
	uint8_t		length;
	const char	*time;
	const char	*tz;
} cmd_opts_t, *cmd_opts_p;

#define MOTP_DEF_PERIOD		10
#define MOTP_DEF_DIGITS		6

static struct option long_options[] = {
	{ "help",	no_argument,		NULL,	'?'	},
	{ "verbose",	no_argument,		NULL,	'v'	},
	{ "secret",	required_argument,	NULL,	's'	},
	{ "pin",	required_argument,	NULL,	'p'	},
	{ "duration",	required_argument,	NULL,	'P'	},
	{ "length",	required_argument,	NULL,	'd'	},
	{ "time",	required_argument,	NULL,	't'	},
	{ "tz",		required_argument,	NULL,	'T'	},
	{ NULL,		0,			NULL,	0	}
};

static const char *long_options_descr[] = {
	"		Show help",
	"		Be verbose",
	"<string>	Shared secret",
	"<string>	PIN",
	"seconds>	Code duration interval. Default: 10",
	"<number>	Result code length. Default: 6",
	"<string>	Time string, in one of formats: HTTP date / RFC 822, RFC 850, ANSI C, YYYY-MM-DD HH:MM:SS, Number of seconds since the Epoch (UTC)",
	"<string>	The timezone time zone offset from UTC. Will override time zone from 'time' string is set. Ex: +0100, -0500.",
	NULL
};


static int
cmd_opts_parse(int argc, char **argv, struct option *opts,
    cmd_opts_p cmd_opts) {
	int i, ch, opt_idx;
	char opts_str[1024];

	memset(cmd_opts, 0x00, sizeof(cmd_opts_t));
	cmd_opts->duration = MOTP_DEF_PERIOD;
	cmd_opts->length = MOTP_DEF_DIGITS;

	/* Process command line. */
	/* Generate opts string from long options. */
	for (i = 0, opt_idx = 0;
	    NULL != opts[i].name && (int)(sizeof(opts_str) - 1) > opt_idx;
	    i ++) {
		if (0 == opts[i].val)
			continue;
		opts_str[opt_idx ++] = (char)opts[i].val;
		switch (opts[i].has_arg) {
		case optional_argument:
			opts_str[opt_idx ++] = ':';
			__attribute__((fallthrough)); /* PASSTROUTH. */
		case required_argument:
			opts_str[opt_idx ++] = ':';
			break;
		default:
			break;
		}
	}

	opts_str[opt_idx] = 0;
	opt_idx = -1;
	while ((ch = getopt_long_only(argc, argv, opts_str, opts,
	    &opt_idx)) != -1) {
restart_opts:
		switch (opt_idx) {
		case -1: /* Short option to index. */
			for (opt_idx = 0;
			    NULL != opts[opt_idx].name;
			    opt_idx ++) {
				if (ch == opts[opt_idx].val)
					goto restart_opts;
			}
			/* Unknown option. */
			break;
		case 0: /* help */
			return (EINVAL);
		case 1: /* verbose */
			cmd_opts->verbose = 1;
			break;
		case 2: /* secret */
			cmd_opts->secret = optarg;
			break;
		case 3: /* pin */
			cmd_opts->pin = optarg;
			break;
		case 4: /* duration */
			cmd_opts->duration = atoi(optarg);
			break;
		case 5: /* length */
			cmd_opts->length = atoi(optarg);
			break;
		case 6: /* time */
			cmd_opts->time = optarg;
			break;
		case 7: /* tz */
			cmd_opts->tz = optarg;
			break;
		default:
			return (EINVAL);
		}
		opt_idx = -1;
	}

	return (0);
}

static void
print_usage(char *progname, struct option *opts,
    const char **opts_descr) {
	size_t i;
	const char *usage =
		PACKAGE_STRING"     "PACKAGE_DESCRIPTION"\n"
		"Usage: %s [options]\n"
		"options:\n";
	fprintf(stderr, usage, basename(progname));

	for (i = 0; NULL != opts[i].name; i ++) {
		if (0 == opts[i].val) {
			fprintf(stderr, "	-%s %s\n",
			    opts[i].name, opts_descr[i]);
		} else {
			fprintf(stderr, "	-%s, -%c %s\n",
			    opts[i].name, opts[i].val, opts_descr[i]);
		}
	}
}


int
main(int argc, char **argv) {
	int error = 0, tz_val, tz_sign;
	cmd_opts_t cmd_opts;
	locale_t locale_c = NULL;
	struct tm tml;
	time_t clock;
	size_t i, buf_size;
	uint64_t time_epoch;
	char buf[4096], digest_str[(MD5_HASH_STR_SIZE + 1)];
	const char *time_fmt[] = {
		"%a, %d %b %Y %H:%M:%S %Z",	/* HTTP date / RFC 822. */
		"%a, %d %b %Y %H:%M:%S %z",	/* HTTP date / RFC 822. */
		"%A, %d-%b-%y %H:%M:%S %Z",	/* RFC 850. */
		"%A, %d-%b-%y %H:%M:%S %z",	/* RFC 850. */
		"%a %b %d %H:%M:%S %Y",		/* ANSI C. */
		"%Y-%m-%d %H:%M:%S",		/* YYYY-MM-DD HH:MM:SS. */
		"@%s",				/* Number of seconds since the Epoch, UTC. */
		NULL
	};

	/* Command line processing. */
	error = cmd_opts_parse(argc, argv, long_options, &cmd_opts);
	if (0 != error) {
		if (-1 == error)
			return (0); /* Handled action. */
		print_usage(argv[0], long_options, long_options_descr);
		return (error);
	}
	/* Handle cmd line options. */
	if (NULL == cmd_opts.secret ||
	    NULL == cmd_opts.pin) {
		fprintf(stderr, "secret and pin is required options!\n");
		print_usage(argv[0], long_options, long_options_descr);
		return (-1);
	}
	if (MD5_HASH_STR_SIZE < cmd_opts.length) {
		fprintf(stderr, "length max count is %i!\n",
		    MD5_HASH_STR_SIZE);
		print_usage(argv[0], long_options, long_options_descr);
		return (-1);
	}


	/* Defaults for formats without timezone set. */
	clock = time(NULL);
	localtime_r(&clock, &tml);
	if (NULL != cmd_opts.time ||
	    NULL != cmd_opts.tz) {
		locale_c = newlocale(LC_TIME_MASK, "C", NULL);
	}

	/* Time. */
	if (NULL != cmd_opts.time) {
		/* Try to recognize time date format. */
		for (i = 0; NULL != time_fmt[i]; i ++) {
			if (NULL != strptime_l(cmd_opts.time, time_fmt[i], &tml, locale_c))
				break;
		}
		if (NULL == time_fmt[i]) {
			fprintf(stderr, "Unknown time format string!\n");
			print_usage(argv[0], long_options, long_options_descr);
			return (-1);
		}
	}

	/* Time zone. */
	if (NULL != cmd_opts.tz) {
		buf_size = strlen(cmd_opts.tz);
		if (3 != buf_size && 5 != buf_size) {
err_out_bad_tz:
			fprintf(stderr, "Unknown time zone format string!\n");
			print_usage(argv[0], long_options, long_options_descr);
			return (-1);
		}
		if ('+' == cmd_opts.tz[0]) {
			tz_sign = 1;
		} else if ('-' == cmd_opts.tz[0]) {
			tz_sign = -1;
		} else {
			goto err_out_bad_tz;
		}
		tz_val = atoi((cmd_opts.tz + 1));
		if (3 == buf_size) { /* In case short form '+09' add 00 minutes. */
			tz_val *= 100;
		}
		if (1400 < tz_val ||
		    (-1 == tz_sign && 1200 < tz_val) ||
		    60 <= (tz_val % 100))
			goto err_out_bad_tz;
		/* Apply time zone. */
		clock = mktime(&tml); /* Convert back to UNIX time, include TZ. */
		/* Add new TZ offset. */
		clock += (tz_sign * ((3600 * (tz_val / 100)) + (60 * (tz_val % 100))));
		gmtime_r(&clock, &tml); /* Convert from UNIX time, without app TZ apply. */
	}

	/* Gen result. */
	if (cmd_opts.verbose) {
		fprintf(stdout, "Time: %s", asctime(&tml));
	}

	time_epoch = (((uint64_t)mktime(&tml)) / ((uint64_t)cmd_opts.duration));
	buf_size = snprintf(buf, sizeof(buf), "%"PRIu64"%s%s",
	    time_epoch, cmd_opts.secret, cmd_opts.pin);
	md5_get_digest_str(buf, buf_size, digest_str);
	/* Limit output result length. */
	digest_str[cmd_opts.length] = 0;
	fprintf(stdout, "%s\n", digest_str);

	/* Exititng... */
	freelocale(locale_c);

	return (error);
}
