/**
 * commons.c -- holds different data types
 *    ______      ___
 *   / ____/___  /   | _____________  __________
 *  / / __/ __ \/ /| |/ ___/ ___/ _ \/ ___/ ___/
 * / /_/ / /_/ / ___ / /__/ /__/  __(__  |__  )
 * \____/\____/_/  |_\___/\___/\___/____/____/
 *
 * The MIT License (MIT)
 * Copyright (c) 2009-2022 Gerardo Orellana <hello @ goaccess.io>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <algorithm>

#include "commons.h"

#include "util/error.hpp"
#include "labels.h"
#include "settings.h"
#include "util/util.h"
#include "util/xmalloc.h"

/* processing time */
time_t end_proc;
time_t timestamp;
time_t start_proc;

/* list of available modules/panels */
int module_list[TOTAL_MODULES] = {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1};

/* String modules to enumerated modules */
static GEnum enum_modules[] = {
    {"VISITORS", VISITORS},
    {"REQUESTS", REQUESTS},
    {"REQUESTS_STATIC", REQUESTS_STATIC},
    {"NOT_FOUND", NOT_FOUND},
    {"HOSTS", HOSTS},
    {"OS", OS},
    {"BROWSERS", BROWSERS},
    {"VISIT_TIMES", VISIT_TIMES},
    {"VIRTUAL_HOSTS", VIRTUAL_HOSTS},
    {"REFERRERS", REFERRERS},
    {"REFERRING_SITES", REFERRING_SITES},
    {"KEYPHRASES", KEYPHRASES},
    {"STATUS_CODES", STATUS_CODES},
    {"REMOTE_USER", REMOTE_USER},
    {"CACHE_STATUS", CACHE_STATUS},
    {"GEO_LOCATION", GEO_LOCATION},
    {"MIME_TYPE", MIME_TYPE},
    {"TLS_TYPE", TLS_TYPE},
};

/* Get number of items per panel to parse.
 *
 * The number of items per panel is returned. */
int get_max_choices(void) {
  char *csv = NULL, *json = NULL, *html = NULL;
  int max = MAX_CHOICES;

  /* no max choices, return defaults */
  if (conf.max_items <= 0)
    return conf.real_time_html ? MAX_CHOICES_RT : (conf.date_spec_hr == 2 ? MAX_CHOICES_MINUTE : MAX_CHOICES);

  /* TERM */
  if (!conf.output_stdout)
    return conf.max_items > MAX_CHOICES ? MAX_CHOICES : conf.max_items;

  /* REAL-TIME STDOUT */
  /* real time HTML, display max rt choices */
  if (conf.real_time_html)
    return conf.max_items > MAX_CHOICES_RT ? MAX_CHOICES_RT : conf.max_items;

  /* STDOUT */
  /* CSV - allow n amount of choices */
  if (find_output_type(&csv, "csv", 1) == 0)
    max = conf.max_items;
  /* JSON - allow n amount of choices */
  if (find_output_type(&json, "json", 1) == 0 && conf.max_items > 0)
    max = conf.max_items;
  /* HTML - takes priority on cases where multiple outputs were given. Note that
   * we check either for an .html extension or we assume not extension was passed
   * via -o and therefore we are redirecting the output to a file. */
  if (find_output_type(&html, "html", 1) == 0 || conf.output_formats.size() == 0)
    max = conf.max_items;

  free(csv);
  free(html);
  free(json);

  return max;
}

/* Calculate a percentage.
 *
 * The percentage is returned. */
float get_percentage(unsigned long long total, unsigned long long hit) {
  return (total == 0 ? 0 : (((float)hit) / total) * 100);
}

/* Display the storage being used. */
void display_storage(void) { fprintf(stdout, "%s\n", BUILT_WITH_DEFHASH); }

/* Display the path of the default configuration file when `-p` is not used */
void display_default_config_file(void) {
  char* path = get_config_file_path();

  if (!path) {
    fprintf(stdout, "%s\n", ERR_NODEF_CONF_FILE);
    fprintf(stdout, "%s `-p /path/goaccess.conf`\n", ERR_NODEF_CONF_FILE_DESC);
  } else {
    fprintf(stdout, "%s\n", path);
    free(path);
  }
}

/* Display the current version. */
void display_version(void) {
  fprintf(stdout, "GoAccess - %s.\n", GO_VERSION);
  fprintf(stdout, "%s: %s\n", INFO_MORE_INFO, GO_WEBSITE);
  fprintf(stdout, "Copyright (C) 2009-2022 by Gerardo Orellana\n");
  fprintf(stdout, "\nBuild configure arguments:\n");
}

/* Get the enumerated value given a string.
 *
 * On error, -1 is returned.
 * On success, the enumerated module value is returned. */
int str2enum(const GEnum map[], int len, const char* str) {
  int i;

  for (i = 0; i < len; ++i) {
    if (!strcmp(str, map[i].str))
      return map[i].idx;
  }

  return -1;
}

/* Get the string value given an enum.
 *
 * On error, -1 is returned.
 * On success, the enumerated module value is returned. */
char* enum2str(const GEnum map[], int len, int idx) {
  int i;

  for (i = 0; i < len; ++i) {
    if (idx == map[i].idx)
      return xstrdup(map[i].str);
  }

  return NULL;
}

/* Get the enumerated module value given a module string.
 *
 * On error, -1 is returned.
 * On success, the enumerated module value is returned. */
int get_module_enum(const char* str) { return str2enum(enum_modules, ARRAY_SIZE(enum_modules), str); }

/* Get the module string value given a module enum value.
 *
 * On error, NULL is returned.
 * On success, the string module value is returned. */
char* get_module_str(GModule module) { return enum2str(enum_modules, ARRAY_SIZE(enum_modules), module); }

/* Instantiate a new GAgents structure.
 *
 * On success, the newly malloc'd structure is returned. */
GAgents* new_gagents(uint32_t size) {
  GAgents* agents = (GAgents*)xmalloc(sizeof(GAgents));
  memset(agents, 0, sizeof *agents);

  agents->items = (GAgentItem*)xcalloc(size, sizeof(GAgentItem));
  agents->size = size;
  agents->idx = 0;

  return agents;
}

/* Clean the array of agents. */
void free_agents_array(GAgents* agents) {
  int i;

  if (agents == NULL)
    return;

  /* clean stuff up */
  for (i = 0; i < agents->idx; ++i)
    free(agents->items[i].agent);
  if (agents->items)
    free(agents->items);
  free(agents);
}

/* Determine if the given date format is a timestamp.
 *
 * If not a timestamp, 0 is returned.
 * If it is a timestamp, 1 is returned. */
int has_timestamp(const char* fmt) {
  if (strcmp("%s", fmt) == 0 || strcmp("%f", fmt) == 0)
    return 1;
  return 0;
}

/* Determine if the given module is set to be enabled.
 *
 * If enabled, 1 is returned, else 0 is returned. */
int enable_panel(GModule mod) {
  int module;

  for (auto panel: conf.enable_panels) {
    if ((module = get_module_enum(panel.c_str())) == -1)
      continue;
    if (mod == (unsigned int)module) {
      return 1;
    }
  }

  return 0;
}

/* Determine if the given module is set to be ignored.
 *
 * If ignored, 1 is returned, else 0 is returned. */
int ignore_panel(GModule mod) {
  int module;

  for (auto panel: conf.ignore_panels) {
    if ((module = get_module_enum(panel.c_str())) == -1)
      continue;
    if (mod == (unsigned int)module) {
      return 1;
    }
  }

  return 0;
}

/* Get the number of available modules/panels.
 *
 * The number of modules available is returned. */
uint32_t get_num_modules(void) {
  size_t idx = 0;
  uint32_t num = 0;

  FOREACH_MODULE(idx, module_list) { num++; }

  return num;
}

/* Get the index from the module_list given a module.
 *
 * If the module is not within the array, -1 is returned.
 * If the module is within the array, the index is returned. */
int get_module_index(int module) {
  size_t idx = 0;

  FOREACH_MODULE(idx, module_list) {
    if (module_list[idx] == module)
      return idx;
  }

  return -1;
}

/* Remove the given module from the module_list array.
 *
 * If the module is not within the array, 1 is returned.
 * If the module is within the array, it is removed from the array and
 * 0 is returned. */
int remove_module(GModule module) {
  int idx = get_module_index(module);
  if (idx == -1)
    return 1;

  if (idx < TOTAL_MODULES - 1)
    memmove(&module_list[idx], &module_list[idx + 1], ((TOTAL_MODULES - 1) - idx) * sizeof(module_list[0]));
  module_list[TOTAL_MODULES - 1] = -1;

  return 0;
}

/* Find the next module given the current module.
 *
 * The next available module in the array is returned. */
int get_next_module(GModule module) {
  int next = get_module_index(module) + 1;

  if (next == TOTAL_MODULES || module_list[next] == -1)
    return module_list[0];

  return module_list[next];
}

/* Find the previous module given the current module.
 *
 * The previous available module in the array is returned. */
int get_prev_module(GModule module) {
  int i;
  int next = get_module_index(module) - 1;

  if (next >= 0 && module_list[next] != -1)
    return module_list[next];

  for (i = TOTAL_MODULES - 1; i >= 0; i--) {
    if (module_list[i] != -1) {
      return module_list[i];
    }
  }

  return 0;
}

/* Perform some additional tasks to panels before they are being
 * parsed.
 *
 * Note: This overwrites --enable-panel since it assumes there's
 * truly nothing to do with the panel */
void verify_panels(void) {
  /* Remove virtual host panel if no '%v' within log format */
  if (!conf.log_format)
    return;

  if (!strstr(conf.log_format, "%v")) {
    if (!conf.ignore_panels.contains("VIRTUAL_HOSTS")) {
      remove_module(VIRTUAL_HOSTS);
    }
  }
  if (!strstr(conf.log_format, "%e")) {
    if (!conf.ignore_panels.contains("REMOTE_USER")) {
      remove_module(REMOTE_USER);
    }
  }
  if (!strstr(conf.log_format, "%C")) {
    if (!conf.ignore_panels.contains("CACHE_STATUS")) {
      remove_module(CACHE_STATUS);
    }
  }
  if (!strstr(conf.log_format, "%M")) {
    if (!conf.ignore_panels.contains("MIME_TYPE")) {
      remove_module(MIME_TYPE);
    }
  }
  if (!strstr(conf.log_format, "%K")) {
    if (!conf.ignore_panels.contains("TLS_TYPE")) {
      remove_module(TLS_TYPE);
    }
  }
  if (!conf.geoip_database) {
    if (!conf.ignore_panels.contains("GEO_LOCATION")) {
      remove_module(GEO_LOCATION);
    }
  }
}

/* Build an array of available modules (ignores listed panels).
 *
 * If there are no modules enabled, 0 is returned.
 * On success, the first enabled module is returned. */
int init_modules(void) {
  int module;
  int i;

  /* init - terminating with -1 */
  for (module = 0; module < TOTAL_MODULES; ++module)
    module_list[module] = -1;

  for (i = 0, module = 0; module < TOTAL_MODULES; ++module) {
    if (!ignore_panel((GModule)module) || enable_panel((GModule)module)) {
      module_list[i++] = module;
    }
  }

  return module_list[0] > -1 ? module_list[0] : 0;
}

/* Get the logs size.
 *
 * If log was piped (from stdin), 0 is returned.
 * On success, it adds up all log sizes and its value is returned.
 * if --log-size was specified, it will be returned explicitly */
intmax_t get_log_sizes(void) {
  off_t size = 0;

  /* --log-size */
  if (conf.log_size > 0)
    return (intmax_t)conf.log_size;

  for (auto filename: conf.filenames) {
    if (filename != "-")
      size += file_size(filename.c_str());
  }

  return (intmax_t)size;
}

/* Get the log sources used.
 *
 * On success, a newly malloc'd string containing the log source either
 * from filename(s) and/or STDIN is returned. */
std::string get_log_source_str(size_t max_len) {
  std::string str;
  bool first = true;

  for (auto filename: conf.filenames) {
    if (first) {
      first = false;
    } else {
      str += "; ";
    }
    if (filename == "-")
      str += "STDIN";
    else
      str += filename;
  }

  if (max_len >= 3 && str.size() > max_len) {
    str = str.substr(0, max_len - 3);
    str += "...";
  }

  return str;
}
