/*
 * Mosquitto authentication plugin: checks a broker login against the data
 * server's Apache "users" file, so a central server has one account list.
 *
 *   plugin /path/to/ecce_users_auth.so
 *   plugin_opt_users_file /path/to/dataserver/users
 *
 * Hashes are verified by apr_password_validate, which reads every format
 * htpasswd writes. The file is re-read when its mtime, size or inode
 * changes, so ecce-dataserver-adduser needs no broker restart. An unknown
 * user is deferred (the broker then refuses it, as it does with allow_anonymous
 * false); a wrong password is refused outright. Passwords are never logged.
 * Access rules stay in the broker's acl_file.
 */
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include <apr_general.h>
#include <apr_md5.h>
#include <mosquitto.h>
#include <mosquitto_broker.h>
#include <mosquitto_plugin.h>

struct entry { char *user; char *hash; };

static mosquitto_plugin_id_t *plugin_id;
static char *users_path;
static struct entry *entries;
static size_t nentries;
static struct stat loaded;
static int have_loaded;

static void free_entries(void)
{
	for (size_t i = 0; i < nentries; i++) { free(entries[i].user); free(entries[i].hash); }
	free(entries);
	entries = NULL;
	nentries = 0;
}

/* Reread the file if it changed; on a read error keep the last good list
 * only when the file still exists, otherwise nobody can log in. */
static void refresh(void)
{
	struct stat st;
	if (stat(users_path, &st) != 0) {
		if (have_loaded) {
			mosquitto_log_printf(MOSQ_LOG_WARNING, "ecce_users_auth: cannot read %s: %s", users_path, strerror(errno));
			free_entries();
			have_loaded = 0;
		}
		return;
	}
	if (have_loaded && st.st_mtim.tv_sec == loaded.st_mtim.tv_sec &&
	    st.st_mtim.tv_nsec == loaded.st_mtim.tv_nsec &&
	    st.st_size == loaded.st_size && st.st_ino == loaded.st_ino)
		return;

	FILE *f = fopen(users_path, "r");
	if (!f) {
		mosquitto_log_printf(MOSQ_LOG_WARNING, "ecce_users_auth: cannot open %s: %s", users_path, strerror(errno));
		return;
	}
	free_entries();
	size_t cap = 0;
	char *line = NULL;
	size_t linecap = 0;
	ssize_t n;
	while ((n = getline(&line, &linecap, f)) > 0) {
		while (n > 0 && (line[n - 1] == '\n' || line[n - 1] == '\r')) line[--n] = '\0';
		char *colon = strchr(line, ':');
		if (line[0] == '#' || !colon || colon == line) continue;
		*colon = '\0';
		if (nentries == cap) {
			cap = cap ? cap * 2 : 64;
			struct entry *e = realloc(entries, cap * sizeof *e);
			if (!e) break;
			entries = e;
		}
		entries[nentries].user = strdup(line);
		entries[nentries].hash = strdup(colon + 1);
		if (!entries[nentries].user || !entries[nentries].hash) break;
		nentries++;
	}
	free(line);
	fclose(f);
	loaded = st;
	have_loaded = 1;
}

static int basic_auth(int event, void *event_data, void *userdata)
{
	struct mosquitto_evt_basic_auth *ed = event_data;
	(void)event; (void)userdata;
	if (!ed->username || !ed->password) return MOSQ_ERR_PLUGIN_DEFER;
	/* The account name is a topic level in the ACL (ecce/%u/#): one with a
	 * separator or wildcard would reach into another account's topics. */
	if (strpbrk(ed->username, "/+#")) return MOSQ_ERR_AUTH;
	refresh();
	for (size_t i = 0; i < nentries; i++) {
		if (strcmp(entries[i].user, ed->username) != 0) continue;
		return apr_password_validate(ed->password, entries[i].hash) == APR_SUCCESS
			? MOSQ_ERR_SUCCESS : MOSQ_ERR_AUTH;
	}
	return MOSQ_ERR_PLUGIN_DEFER;
}

int mosquitto_plugin_version(int supported_version_count, const int *supported_versions)
{
	for (int i = 0; i < supported_version_count; i++)
		if (supported_versions[i] == 5) return 5;
	return -1;
}

int mosquitto_plugin_init(mosquitto_plugin_id_t *identifier, void **user_data,
                          struct mosquitto_opt *options, int option_count)
{
	(void)user_data;
	plugin_id = identifier;
	for (int i = 0; i < option_count; i++)
		if (strcmp(options[i].key, "users_file") == 0) {
			free(users_path);
			users_path = strdup(options[i].value);
		}
	if (!users_path) {
		mosquitto_log_printf(MOSQ_LOG_ERR, "ecce_users_auth: plugin_opt_users_file is required");
		return MOSQ_ERR_INVAL;
	}
	apr_initialize();
	refresh();
	return mosquitto_callback_register(plugin_id, MOSQ_EVT_BASIC_AUTH, basic_auth, NULL, NULL);
}

int mosquitto_plugin_cleanup(void *user_data, struct mosquitto_opt *options, int option_count)
{
	(void)user_data; (void)options; (void)option_count;
	mosquitto_callback_unregister(plugin_id, MOSQ_EVT_BASIC_AUTH, basic_auth, NULL);
	free_entries();
	free(users_path);
	users_path = NULL;
	apr_terminate();
	return MOSQ_ERR_SUCCESS;
}
