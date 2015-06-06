#include "redis.h"
#include <regex.h>
#include <yaml.h>

// dbname://tablename:rowkey
#define HBASE_COMMAND_PATTERN "(\\w+)://(\\w+):(\\w+)"
#define MAX_ERROR_MSG 0x1000

hedisConfig *hedis_config;

char **parse_hbase_protocol(const char * to_match){
  regex_t * r = malloc(sizeof(regex_t));

  int status = regcomp(r, HBASE_COMMAND_PATTERN, REG_EXTENDED|REG_NEWLINE);

  if (status != 0) {
    char error_message[MAX_ERROR_MSG];

    regerror(status, r, error_message, MAX_ERROR_MSG);

    printf("Regex error compiling '%s': %s\n", HBASE_COMMAND_PATTERN, error_message);

    return NULL;
  }

  char **str = malloc(sizeof(char *) * 3);

  /* "P" is a pointer into the string which points to the end of the
   *        previous match. */
  const char * p = to_match;
  /* "N_matches" is the maximum number of matches allowed. */
  const int n_matches = 10; 
  /* "M" contains the matches found. */
  regmatch_t m[n_matches];

  int i = 0;
  int nomatch = regexec(r, p, n_matches, m, 0);

  if (nomatch) {
    printf("No more matches.\n");

    return NULL;
  }

  for (i = 0; i < n_matches; i++) {
    int start;
    int finish;

    if (m[i].rm_so == -1) {
      break;
    }

    start = m[i].rm_so + (p - to_match);
    finish = m[i].rm_eo + (p - to_match);

    if (i != 0) {
      int size = finish - start;

      str[i - 1] = malloc(sizeof(char) * size);

      sprintf(str[i - 1], "%.*s", size, to_match + start);
    }
  }

  p += m[0].rm_eo;

  return str;
}

int count_entries(FILE *file) {
  yaml_parser_t parser;
  yaml_token_t token;
  int counts = 0;

  /* Initialize parser */
  if(!yaml_parser_initialize(&parser)) {
      fputs("Failed to initialize parser!\n", stderr);
  }

  if(file == NULL) {
      fputs("Failed to open file!\n", stderr);
  }

  /* Set input file */
  yaml_parser_set_input_file(&parser, file);

  /* BEGIN new code */
  // calculate entry counts
  do {
    yaml_parser_scan(&parser, &token);

    if (token.type == YAML_BLOCK_ENTRY_TOKEN){
      counts++;
    }

    if(token.type != YAML_STREAM_END_TOKEN) {
      yaml_token_delete(&token);
    }
  } while(token.type != YAML_STREAM_END_TOKEN);

  /* Cleanup */
  yaml_parser_delete(&parser);

  fseek(file, 0, SEEK_SET);

  return counts;
}