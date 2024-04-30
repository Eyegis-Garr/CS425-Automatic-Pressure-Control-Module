/**
 * @file parse.c
 * @author Bradley Sullivan (bradleysullivan@nevada.unr.edu)
 * @brief Command parsing routines
 * @version 0.1
 * @date 2024-04-24
 * 
 * @copyright Copyright (c) 2024
 * 
 */
#include "client.h"

static struct option loptions[] = {
  {"CIRCUIT",   required_argument,  NULL, 'C'},
  {"SYSTEM",    no_argument,        NULL, 'S'},
  {"PARAMETER", required_argument,  NULL, 'P'},
  {"RESET",     no_argument,        NULL, 'R'},
  {"MODESET",   required_argument,  NULL, 'M'},
  {"PARSET",    required_argument,  NULL, 'V'},
  {"SAVE",      no_argument,        NULL, 'A'},
  {"CONFIG",    no_argument,        NULL, 'G'},
  {"TIMEOUT",   required_argument,  NULL, 'T'},
  {"ACK",       no_argument,        NULL, 'K'},
  {"NTIMES",    required_argument,  NULL, 'N'},
  { 0 }
};


/**
 * @brief Parses user-input 'update' commands.
 * Given tokenized input, configures packet
 * argument structure to build update request.
 * 
 * Operates similar to parse_command/ping.
 * 
 * Valid options -> :c:p:t:rsk
 * 
 * @param pargs Packet argument structure to fill
 * @param ac Number of tokenized arguments
 * @param av Tokenized command strings
 * @return int Returns 0 on success, client E_<ERROR> on failure
 */
int parse_update(packet_args *pargs, int ac, char *av[]) {
  int opidx = 0, opt, ret = 0;
  int tidx;
  char *subopts, *value;

  // reset optind for reinitializing getopt state
  optind = 0;

  while ((opt = getopt_long(ac, av, ":C:P:T:RSK", loptions, &opidx)) != -1) {
    switch (opt) {
      case 'S':   /* --system, -s */
        pargs->op_flags |= (1 << UP_SYSTEM);
				break;
      case 'C':   /* --circuit, -c */
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
      case 'P':   /* --parameter, -p */
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
      case 'R':   /* --refresh, -r */
        pargs->op_flags |= (1 << UP_RESET);
        break;
      case 'T':   /* --timeout, -t */
        if (sscanf(optarg, "%hhu", &pargs->timeout) == -1) {
          pargs->timeout = 0;
          ret = E_OPTION;
        }
        break;
      case 'K':   /* --ack, -k */
        pargs->req_ack = 1; 
        break;
      case '?':
        ret = E_OPTION;
        break;
      case ':':
        ret = E_ARGUMENT;
        break;
    }      
  }

  return ret;
}

/**
 * @brief Parses user-input 'do' commands.
 * Operates similar to parse_update/ping.
 * 
 * Valid options -> :c:p:t:m:v:agk
 * 
 * @param pargs Packet argument structure to fill
 * @param ac Number of tokenized arguments
 * @param av Tokenized command strings
 * @return int Returns 0 on success, client E_<ERROR> on failure
 */
int parse_command(packet_args *pargs, int ac, char *av[]) {
  int opidx = 0, opt, ret = 0;
  int midx, tidx;
  char *subopts, *value;

  // reset optind for reinitializing getopt state
  optind = 0;

  while ((opt = getopt_long(ac, av, ":C:P:T:M:V:AGK", loptions, &opidx)) != -1) {
    switch (opt) {
      case 'C':   /* --circuit, -c */
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
      case 'P':   /* --parameter, -p */
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
      case 'T':   /* --timeout, -t */
        if (sscanf(optarg, "%hhu", &pargs->timeout) == -1) {
          pargs->timeout = 0;
          ret = E_OPTION;
        }
        break;
      case 'M':   /* --modeset, -m */
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
      case 'V':   /* --parset, -v */
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
      case 'A':   /* --save, -s */
        pargs->op_flags |= (1 << CMD_SAVE); 
        break;
      case 'G':   /*  --config, -g  */
        pargs->op_flags |= (1 << CMD_DMPCFG);
        break;
      case 'K':   /* --ack, -k */
        pargs->req_ack = 1; 
        break;
      case '?':
        ret = E_OPTION;
        printf("optarg: %s\n", optarg);
        break;
      case ':':
        ret = E_ARGUMENT;
        break;
    }
  }
  
  return ret;
}

/**
 * @brief Parses user-input 'ping' commands.
 * Operates similar to parse_update/command.
 *  
 * Valid options -> :c:p:t:m:v:agk
 * 
 * @param pargs Packet argument structure to fill
 * @param ac Number of tokenized arguments
 * @param av Tokenized command strings
 * @return int Returns 0 on success, client E_<ERROR> on failure
 */
int parse_ping(packet_args *pargs, int ac, char *av[]) {
  int opidx, opt, ret;

  ret = 0;
  opidx = 0;

  // reset optind for reinitializing getopt state
  optind = 0;

  pargs->op_flags |= (1 << ST_PING);

  while ((opt = getopt_long(ac, av, ":N:T:", loptions, &opidx)) != -1) {
    switch (opt) {
      case 'N':   /* --nping, -n */
        if (sscanf(optarg, "%lf", &pargs->values[pargs->next_val].value) != -1) {
          pargs->values[pargs->next_val].op = ST_PING;
          pargs->values[pargs->next_val].flag = ST_PING;
          pargs->next_val += 1;
        } else {
          ret = E_OPTION;
        }
        break;
      case 'T':   /* --timeout, -t */
        if (sscanf(optarg, "%hhu", &pargs->timeout) == -1) {
          pargs->timeout = 0;
          ret = E_OPTION;
        }
        break;
      case '?':
        ret = E_OPTION;
        break;
      case ':':
        ret = E_ARGUMENT;
        break;
    }
  }

  return ret;
}

/**
 * @brief Seeks valset array for next value-context
 * with specified operation. On return, vhead points
 * to the first value-context in next op-run.
 * 
 * @param base Location to seek forward from
 * @param vhead Seek-head. On success, points to first
 * value-context with specified operation.
 * @param op Operation to seek/search for
 * @param len Length of full  valset array
 * @return int Run-length of value-contexts with op-field == op
 */
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

/**
 * @brief Valset flag comparison routine
 * 
 * @param a A
 * @param b B
 * @return int Returns A->flag - B->flag
 */
int val_cmp_flag(const void *a, const void *b) {
  const valset_t *av = *(const valset_t **)a;
  const valset_t *bv = *(const valset_t **)b;
  
  return av->flag - bv->flag;
}

/**
 * @brief Valset operation comparison routine
 * 
 * @param a A
 * @param b B
 * @return int Returns A->op - B->op
 */
int val_cmp_op(const void *a, const void *b) {
  const valset_t *av = (const valset_t *)a;
  const valset_t *bv = (const valset_t *)b;
  
  return av->op - bv->op;
}

/**
 * @brief Valset value comparison routine
 * 
 * @param a A
 * @param b B
 * @return int Returns A->value - B->value
 */
int val_cmp_value(const void *a, const void *b) {
  const valset_t *av = (const valset_t *)a;
  const valset_t *bv = (const valset_t *)b;
  
  return av->value - bv->value;
}

/**
 * @brief Matches provided string in provided string
 * map. Returns index of match.
 * 
 * @param map String map to match
 * @param maplen Number of strings in string-map
 * @param str String to match in map
 * @return int Returns index of match on success, -1 on failure
 */
int mapstr(char *const map[], int maplen, char *str) {
  for (int i = 0; i < maplen; i += 1) {
    if (strcmp(map[i], str) == 0) return i;
  }

  return -1;
}

/**
 * @brief Constructs descriptive string for specified
 * 'flag' and 'map'. Builds 'str' with each string
 * from the location in the 'map' for each bit set
 * in the 'flag' of bit-width 'width'.
 * 
 * @param flag Bit flags to check
 * @param width Bit-width of flag (max. 32 bits)
 * @param map Map for flag-index -> flag-string
 * @param str Output string
 * @return int Returns number of characters added to str
 */
int mapflag(int flag, int width, char *const map[], char *str) {
	char *print = str;
	int i;
	for (i = 0; i < width; i += 1) {
		if (map[i] == NULL) break;
		if (isbset(flag, i)) {
			print += sprintf(print, "%s%s ", (strlen(str) > 0) ? "| " : " ", map[i]);
		}
	}

	return print - str;
}

/**
 * @brief User-input preprocessing routine. Tokenizes
 * user-commands delimited with POSIX whitespace 
 * characters -> " \t\r\n\v\f"
 * 
 * @param cmd User-input string
 * @param vec Output token/argument vector
 * @return int Number of tokens in input
 */
int tokenize_cmd(char *cmd, char **vec) {
  char input[MAX_CMD_LEN] = { 0 }, *tok;
  int ct;

  if (!cmd || !vec) return -1;

  strncpy(input, cmd, strlen(cmd));

  ct = 0;
  tok = strtok(input, " \t\r\n\v\f");         // split by POSIX-whitespace
  while (tok && ct < MAX_ARGS) {              // while tokens remain
    vec[ct] = malloc(strlen(tok) + 1);        // malloc space for new token in vector
    if (!vec[ct]) return -1;                  // error if malloc fails

    strcpy(vec[ct], tok);                     // copy token into argument vector

    tok = strtok(NULL, " \t\r\n\v\f");        // move to next token
    ct += 1;                                  // increment counter
  }

  vec[ct] = NULL;

  return ct;
}

/**
 * @brief Converts string to all LOWERCASE
 * 
 * @param str Input
 */
void to_lower(char *str) {
  while (*str) { *str++ = tolower(*str); }
}

/**
 * @brief Converts string to all UPPERCASE
 * 
 * @param str Input
 */
void to_upper(char *str) {
  while (*str) { *str++ = toupper(*str); }
}
