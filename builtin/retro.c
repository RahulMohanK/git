#define USE_THE_REPOSITORY_VARIABLE


#include "stdio.h"
#include "retro.h"
#include "builtin.h"
#include "abspath.h"
#include "config.h"
#include "environment.h"
#include "gettext.h"
#include "hash.h"
#include "hex.h"
#include "object-name.h"
#include "parse-options.h"
#include "refs.h"
#include "stash.h"
#include "lockfile.h"
#include "cache-tree.h"
#include "unpack-trees.h"
#include "merge-ort-wrappers.h"
#include "strvec.h"
#include "run-command.h"
#include "dir.h"
#include "entry.h"
#include "preload-index.h"
#include "read-cache.h"
#include "repository.h"
#include "rerere.h"
#include "revision.h"
#include "setup.h"
#include "sparse-index.h"
#include "log-tree.h"
#include "diffcore.h"
#include "reflog.h"
#include "reflog-walk.h"
#include "add-interactive.h"
#include "oid-array.h"
#include "commit.h"
#include "string-list.h"

struct branch_commits {
	char *branch;
	int commits;
};

int cmd_retro(int argc,
	      const char **argv,
	      const char *prefix,
	      struct repository *repo)
{
	struct child_process cp = CHILD_PROCESS_INIT;
	struct strbuf out = STRBUF_INIT;
	struct string_list branches = STRING_LIST_INIT_DUP;
	char *line;

	cp.git_cmd = 1;
	strvec_pushl(&cp.args, "reflog", "--all",
		     "--since=7 days ago", "--until=now",
		     "--format=%gd",
		     "--grep-reflog=branch: Created",
		     NULL);
	if (capture_command(&cp, &out, 0)){
		return error(_("could not enumerate branches"));
	}

	line = out.buf;
	for (;;) {
		char *eol = strchr(line, '\n');
		if (eol)
			*eol = '\0';
		if (*line) {
			char *index = strstr(line, "@{");   /* strip @{N} */
			if (index)
				*index = '\0';
			if (!string_list_has_string(&branches, line))
				string_list_append(&branches, line);
		}
		if (!eol)
			break;
		line = eol + 1;
	}

	if (!branches.nr){
		printf("  (none)\n");
	} else {
		int count = 0;
		struct branch_commits *branch_commits = xcalloc(branches.nr, sizeof(* branch_commits));
		for (size_t i = 0; i < branches.nr; i++) {
			struct child_process cnt = CHILD_PROCESS_INIT;
			struct strbuf n = STRBUF_INIT;
			cnt.git_cmd = 1;
			strvec_pushl(&cnt.args, "rev-list", "--count",
			     branches.items[i].string, NULL);
			if (capture_command(&cnt, &n, 0)) {
				warning(_("could not count '%s'"), branches.items[i].string);
				strbuf_release(&n);	
				continue;
			}
			branch_commits[i].branch = branches.items[i].string;
			branch_commits[i].commits = atoi(n.buf);
			count += atoi(n.buf);
			strbuf_release(&n);
		}
		printf("This weeks status : \n");
		printf("  Branches created : %s%zu%s \n",GIT_COLOR_GREEN,branches.nr,GIT_COLOR_RESET);
		printf("  Commits made : %s%d%s \n\n",GIT_COLOR_GREEN,count,GIT_COLOR_RESET);
		printf("Commits made per branch :\n");
		for (size_t i = 0; i < branches.nr; i++) {
			printf("  %s%s%s -- %d commits\n", GIT_COLOR_GREEN, branch_commits[i].branch , GIT_COLOR_RESET, branch_commits[i].commits);
		}
		free(branch_commits);
	}
	string_list_clear(&branches, 0);
	strbuf_release(&out);
	return 0;
}
