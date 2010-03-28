#include "ol_lrc_fetch_minilyrics.h"
#include "ol_intl.h"
#include "ol_md5.h"
#include "ol_utils.h"
#include "ol_debug.h"

#define BUF_SIZE 1024
#define MAX_CANDIDATE_COUNT 5
const char *XML_PATTERN =
  "<?xml version=\"1.0\" encoding='utf-8'?>\r\n"
  "<search filetype=\"lyrics\" artist=\"%s\" title=\"%s\" "
  "ClientCharEncoding=\"utf-8\"/>\r\n";
const char *DIGIST_APPEND = "Mlv1clt4.0";
const char *REQUEST_PREFIX = "\x02\x00\x04\x00\x00\x00";
const int REQUEST_PREFIX_LEN = 6;
const char *SEARCH_URL = "http://www.viewlyrics.com:1212/searchlyrics.htm";
const char *USER_AGENT = "MiniLyrics";

static OlLrcFetchEngine *engine = NULL;

static OlLrcCandidate *ol_lrc_fetch_minilyrics_search (const OlMusicInfo *info,
                                                  int *size,
                                                  const char* charset);
static int ol_lrc_fetch_minilyrics_download (OlLrcCandidate *candidate,
                                             const char *pathname,
                                             const char *charset);

static char *_get_request (const OlMusicInfo *info,
                           size_t *size);
static int _get_candidate (const char *str,
                               OlLrcCandidate *candidate);
static size_t _scan_until (const char *src,
                           char *dest,
                           char delim,
                           size_t size);

static int _get_value (const char *src,
                       const char *attr,
                       char *value,
                       size_t size);

static int
_get_value (const char *src,
            const char *attr,
            char *value,
            size_t size)
{
  ol_assert_ret (src != NULL, 0);
  ol_assert_ret (attr != NULL, 0);
  ol_assert_ret (value != NULL, 0);
  char lattr[BUF_SIZE] = "";
  strncpy (lattr, attr, BUF_SIZE);
  strncat (lattr, "=\"", BUF_SIZE);
  char *ptr = strstr (src, lattr);
  if (ptr == NULL)
    return 0;
  ptr += strlen (lattr);
  size_t len = 0;
  while (*ptr != '\0' && *ptr != '\"' && len < size - 1)
  {
    value[len] = *ptr;
    len++; ptr++;
  }
  value[len] = '\0';
  return 1;
}

static int _get_candidate (const char *str,
                           OlLrcCandidate *candidate)
{
  ol_log_func ();
  ol_assert_ret (str != NULL, -1);
  ol_assert_ret (candidate != NULL, -1);
  char buf[BUF_SIZE] = "";
  char val[BUF_SIZE] = "";
  char *end;
  const char *ptr = str;
  while (1)
  {
    end = strstr (ptr, "/>");
    if (end == NULL)
      return -1;
    end += 2;
    if (_get_value (ptr, "link", val, BUF_SIZE))
      strncpy (candidate->url, val, OL_URL_LEN_MAX);
    else
      return -1;
    if (strcmp (val + strlen (val) - 3, "lrc") != 0)
    {
      ol_debugf ("link: %s\n", val);
      ptr = end;
      continue;
    }
    if (_get_value (ptr, "title", val, BUF_SIZE))
      strncpy (candidate->title, val, OL_TS_LEN_MAX);
    else
      return -1;
    if (_get_value (ptr, "artist", val, BUF_SIZE))
      strncpy (candidate->artist, val, OL_TS_LEN_MAX);
    else
      candidate->artist[0] = '\0';
    break;
  }
  ol_debug ("asdf");
  return end - str;
}

static char
*_get_request (const OlMusicInfo *info,
               size_t *size)
{
  const char *title, *artist;
  title = ol_music_info_get_title (info);
  artist = ol_music_info_get_artist (info);
  if (artist == NULL)
    artist = "";
  char *xml = g_strdup_printf (XML_PATTERN, artist, title);
  char digest[MD5_DIGEST_SIZE + 1];
  struct md5_ctx md5;
  md5_init (&md5);
  md5_update (&md5, strlen (xml), xml);
  md5_update (&md5, strlen (DIGIST_APPEND), DIGIST_APPEND);
  md5_digest (&md5, MD5_DIGEST_SIZE, digest);
  digest[MD5_DIGEST_SIZE] = '\0';
  int request_len = REQUEST_PREFIX_LEN + MD5_DIGEST_SIZE + strlen (xml);
  char *request = g_new (char, request_len + 1);
  memcpy (request, REQUEST_PREFIX, REQUEST_PREFIX_LEN);
  memcpy (request + REQUEST_PREFIX_LEN, digest, MD5_DIGEST_SIZE);
  strcpy (request + REQUEST_PREFIX_LEN + MD5_DIGEST_SIZE, xml);
  g_free (xml);
  *size = request_len;
  return request;
}

static OlLrcCandidate *
ol_lrc_fetch_minilyrics_search (const OlMusicInfo *info,
                                int *size,
                                const char* charset)
{
  ol_assert_ret (info != NULL, NULL);
  ol_assert_ret (size != NULL, NULL);
  ol_assert_ret (ol_music_info_get_title (info) != NULL, NULL);
  static OlLrcCandidate result[MAX_CANDIDATE_COUNT];
  int count = 0;
  /* Prepare request */
  size_t request_len;
  char *request = _get_request (info, &request_len);
  /* Send request and parse result */
  struct memo lrc;
  lrc.mem_base = NULL;
  lrc.mem_len = 0;
  fetch_into_memory (SEARCH_URL, NULL, USER_AGENT, request, request_len, &lrc);
  g_free (request);
  //ol_debugf ("result:\n%s\n", lrc.mem_base);
  OlLrcCandidate candidate;
  int ptr = 0;
  int dptr = 0;
  while ((dptr = _get_candidate ((char*)lrc.mem_base + ptr, &candidate)) > 0)
  {
    ptr += dptr;
    ol_debugf ("add new candidate: title: %s, artist: %s, url:%s\n",
               candidate.title, candidate.artist, candidate.url);
    count = ol_lrc_fetch_add_candidate (info, result, count,
                                        MAX_CANDIDATE_COUNT,
                                        &candidate);
  }
  *size = count;
  return result;
}

static int
ol_lrc_fetch_minilyrics_download (OlLrcCandidate *candidate,
                                  const char *pathname,
                                  const char *charset)
{
  ol_log_func ();
  ol_debugf ("download from: %s\n", candidate->url);
  char *lrc_conv, *pathbuf;
  FILE *fp;
  int ret;
  struct memo lrc;
  lrc.mem_base = NULL;
  lrc.mem_len = 0;

  if((ret = fetch_into_memory(candidate->url,
                              NULL, /* refer */
                              NULL, /* user-agent */
                              NULL, /* postdata */
                              0,    /* postdata len */
                              &lrc)) < 0)
    return -1;
  lrc_conv = calloc(lrc.mem_len*2, sizeof(char));
  convert("GBK", charset==NULL ? "UTF-8" : charset, lrc.mem_base, lrc.mem_len, lrc_conv, lrc.mem_len*2);
  free(lrc.mem_base);
	
  pathbuf = ol_path_alloc();
  if(pathname == NULL)
    strcpy(pathbuf, "./");
  else
    strcpy(pathbuf, pathname);

  if((fp = fopen(pathbuf, "w")) == NULL)
    return -1;
  fputs(lrc_conv, fp);
  fclose(fp);
  free(pathbuf);
  free(lrc_conv);
  return 0;
}

OlLrcFetchEngine *
ol_lrc_fetch_minilyrics_engine ()
{
  if (engine == NULL)
  {
    engine = malloc (sizeof (OlLrcFetchEngine));
    engine->name = N_("Minilyrics");
    engine->search = ol_lrc_fetch_minilyrics_search;
    engine->download = ol_lrc_fetch_minilyrics_download;
  }
  return engine;
}
