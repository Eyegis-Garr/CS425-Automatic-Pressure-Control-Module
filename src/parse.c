#include "client.h"

static struct option loptions[] = {
  {"circuit",   required_argument,  NULL, 'c'},
  {"system",    no_argument,        NULL, 's'},
  {"parameter", required_argument,  NULL, 'p'},
  {"refresh",   no_argument,        NULL, 'r'},
  {"modeset",   required_argument,  NULL, 'm'},
  {"parset",    required_argument,  NULL, 'v'},
  {"save",      no_argument,        NULL, 'a'},
  {"config",    no_argument,        NULL, 'g'},
  {"timeout",   required_argument,  NULL, 't'},
  {"ack",       no_argument,        NULL, 'k'},
  {"nping",     required_argument,  NULL, 'n'},
  {"ttl",       required_argument,  NULL, 'l'},
  { 0 }
};

const char update_opts[] = "c:p:t:rsk";
const char command_opts[] = "c:p:t:m:v:agk";
const char ping_opts[] = "n:l:t:";

int parse_input(packet_args *pargs, int ac, char *av[], const char *optstring) {
  int opidx = 0, opt, ret = 0;
  int midx, tidx;
  char *subopts, *value;

  // reset optind for reinitializing getopt state
  optind = 0;

  while ((opt = getopt_long(ac, av, optstring, loptions, &opidx)) != -1) {
    switch (opt) {
      case 's':   /* --system, -s */
        pargs->op_flags |= (1 << UP_SYSTEM);
				break;
      case 'c':   /* --circuit, -c */
        pargs->op_flags |= (1 << UP_CIRCUITS);
        subopts = optarg;
        while (*subopts != '\0') {
          tidx = getsubopt(&subopts, circuit_map, &value);

          if (tidx != -1) {
            if (tidx == C_NUM_CIRCUITS)   // 'ALL' argument
              pargs->cmask = (1 << C_NUM_CIRCUITS) - 1;
            else
              pargs->cmask |= (1 << tidx); 
          } else {
            ret = E_SUBOPT;
            break;
          }
        }
        
        break;
      case 'p':   /* --parameter, -p */
        subopts = optarg;
        while (*subopts != '\0') {
          tidx = getsubopt(&subopts, param_map, &value);
          if (tidx != -1) {
            if (tidx == C_NUM_PARAM) 
              pargs->pmask = (1 << C_NUM_PARAM) - 1;
            else
              pargs->pmask |= (1 << tidx); 
          } else {
            ret = E_SUBOPT;
            break;
          }
        }
        break;
      case 'r':   /* --refresh, -r */
        pargs->op_flags |= (1 << UP_REFRESH); break;
      case 'm':   /* --modeset, -m */
        pargs->op_flags |= (1 << CMD_MODESET);
        if ((midx = mapstr(mode_map, S_NUM_MODES, optarg)) != -1) {
          // stores modeset operation/flag and value info for later processing
          pargs->values[pargs->next_val].op = CMD_MODESET;
          pargs->values[pargs->next_val].flag = CMD_MODESET;
          pargs->values[pargs->next_val].value = midx;
          pargs->next_val += 1;
        } else {
					ret = E_OPTION;
        }
        break;
      case 'v':   /* --parset, -v */
        pargs->op_flags |= (1 << CMD_PARSET);

        subopts = optarg;
        while (*subopts != '\0') {
          tidx = getsubopt(&subopts, param_map, &value);

          if (tidx != -1 && value) {
            if (isbclr(pargs->pmask, tidx) && tidx < C_NUM_PARAM) {   // value doesn't already exist for current parameter
              pargs->pmask |= (1 << tidx);

              if (sscanf(value, "%lf", &pargs->values[pargs->next_val].value) != -1) {
                // if successfully scanned value, store parset operation and flag for later processing
                pargs->values[pargs->next_val].op = CMD_PARSET;
                pargs->values[pargs->next_val].flag = tidx;
                pargs->next_val += 1;
              }
            }
          } else {
            ret = E_SUBOPT;
						break;
          }
        }
        break;
      case 'a':   /* --save, -s */
        pargs->op_flags |= (1 << CMD_SAVE); break;
      case 'f':   /* --config, -f */
        pargs->op_flags |= (1 << CMD_DMPCFG); break;
      case 'l':   /* --ttl, -l  */
      case 't':   /* --timeout, -t */
        if (sscanf(optarg, "%hhu", &pargs->timeout) == -1) {
          pargs->timeout = 0;
          ret = E_OPTION;
        }
        break;
      case 'k':   /* --ack, -k */
        pargs->req_ack = 1; break;
      case 'n':   /* --nping, -n */
        pargs->op_flags |= (1 << ST_PING);
        if (sscanf(optarg, "%lf", &pargs->values[pargs->next_val].value) != -1) {
          pargs->values[pargs->next_val].op = ST_PING;
          pargs->values[pargs->next_val].flag = ST_PING;
          pargs->next_val += 1;
        } else {
          ret = E_OPTION;
        }
        break;
      case '?':   /* unknown/invalid option */
        ret = E_OPTION;
        break;
    }
  }

  return ret;
}

int val_seek(valset_t *base, valset_t **vhead, int op, int len) {
  int i = 0;

  // searches value array @ vhead for first valset w/ equal op
  while (*vhead < base + len && (*vhead)->op != op) {
    *vhead += 1;
  }

  // counts length of contiguous valset's from base w/ equal op
  while (*vhead + i < base + len && (*vhead + i)->op == op) {
    i += 1;
  }

  return i;
}

int val_cmp_flag(const void *a, const void *b) {
  const valset_t *av = *(const valset_t **)a;
  const valset_t *bv = *(const valset_t **)b;
  
  return av->flag - bv->flag;
}

int val_cmp_op(const void *a, const void *b) {
  const valset_t *av = (const valset_t *)a;
  const valset_t *bv = (const valset_t *)b;
  
  return av->flag - bv->flag;
}

int val_cmp_value(const void *a, const void *b) {
  const valset_t *av = (const valset_t *)a;
  const valset_t *bv = (const valset_t *)b;
  
  return av->flag - bv->flag;
}

int mapstr(char *const map[], int maplen, char *str) {
  for (int i = 0; i < maplen; i += 1) {
    if (strcmp(map[i], str) == 0) return i;
  }

  return -1;
}

int mapflag(int flag, int width, char *const map[], char *str) {
	char *print = str;
	int i;
	for (i = 0; i < width; i += 1) {
		if (map[i] == NULL) break;
		if (isbset(flag, i)) {
			print += sprintf(print, "| %s", map[i]);
		}
	}

	return print - str;
}

int tokenize_cmd(char *cmd, char **vec) {
  int ct = 0;

  char *tok = strtok(cmd, " \n\t");     // split by spaces, tabs, and new lines
  while (tok && ct < MAX_ARGS) {        // while tokens remain
    vec[ct] = malloc(strlen(tok) + 1);  // malloc space for new token in vector
    if (!vec[ct]) return -1;            // error if malloc fails

    strcpy(vec[ct], tok);               // copy token into argument vector

    tok = strtok(NULL, " \t\n");        // move to next token
    ct += 1;                            // increment counter
  }

  vec[ct] = NULL;

  return ct;
}