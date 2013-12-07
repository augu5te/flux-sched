/* flux-kvsdir.c - flux kvsdir subcommand */

#define _GNU_SOURCE
#include <getopt.h>
#include <json/json.h>
#include <assert.h>
#include <libgen.h>

#include "cmb.h"
#include "util.h"
#include "log.h"

#define OPTIONS "hvRF"
static const struct option longopts[] = {
    {"help",       no_argument,  0, 'h'},
    {"values",     no_argument,  0, 'v'},
    {"recursive",  no_argument,  0, 'R'}, /* matches ls arg */
    {"classify",   no_argument,  0, 'F'}, /* matches ls arg */
    { 0, 0, 0, 0 },
};

static void dump_kvs_dir (flux_t h, const char *path,
                          bool vopt, bool Ropt, bool Fopt);

void usage (void)
{
    fprintf (stderr, 
"Usage: flux-kvsdir [--values] [--classify] [--recursive] key\n"
);
    exit (1);
}

int main (int argc, char *argv[])
{
    flux_t h;
    int ch;
    char *key = NULL;
    bool vopt = false;
    bool Ropt = false;
    bool Fopt = false;

    log_init ("flux-kvsdir");

    while ((ch = getopt_long (argc, argv, OPTIONS, longopts, NULL)) != -1) {
        switch (ch) {
            case 'h': /* --help */
                usage ();
                break;
            case 'v': /* --values */
                vopt = true;
                break;
            case 'R': /* --recursive */
                Ropt = true;
                break;
            case 'F': /* --classify */
                Fopt = true;
                break;
            default:
                usage ();
                break;
        }
    }
    if (optind != argc - 1 && optind != argc)
        usage ();
    if (optind == argc - 1)
        key = argv[optind];

    if (!(h = cmb_init ()))
        err_exit ("cmb_init");

    dump_kvs_dir (h, key ? key : ".", vopt, Ropt, Fopt);

    flux_handle_destroy (&h);
    log_fini ();
    return 0;
}

static void dump_kvs_dir (flux_t h, const char *path,
                          bool vopt, bool Ropt, bool Fopt)
{
    kvsdir_t dir;
    kvsitr_t itr;
    const char *name, *js;
    char *key;

    if (kvs_get_dir (h, &dir, "%s", path) < 0) {
        printf ("%s: %s\n", path, strerror (errno));
        return;
    }

    itr = kvsitr_create (dir);
    while ((name = kvsitr_next (itr))) {
        key = kvsdir_key_at (dir, name);
        if (kvsdir_issymlink (dir, name)) {
            char *F = Fopt ? "@" : "";

            if (vopt) {
                char *link;
                if (kvs_get_symlink (h, key, &link) < 0) {
                    printf ("%s%s: %s\n", key, F, strerror (errno));
                    continue;
                }
                printf ("%s%s -> %s\n", key, F, link);
                free (link);
            } else
                printf ("%s%s", key, F);
        } else if (kvsdir_isdir (dir, name)) {
            char *F = Fopt ? "." : "";

            if (Ropt)
                dump_kvs_dir (h, key, vopt, Ropt, Fopt);
            else
                printf ("%s%s\n", key, F);
        } else {
            if (vopt) {
                json_object *o;
                int len, max;
                if (kvs_get (h, key, &o) < 0) {
                    printf ("%s: %s\n", key, strerror (errno));
                    continue;
                }
                js = json_object_to_json_string_ext (o, JSON_C_TO_STRING_PLAIN);
                len = strlen (js);
                max = 80 - strlen (key) - 4;
                if (len > max)
                    printf ("%s = %.*s ...\n", key, max - 4, js);
                else
                    printf ("%s = %s\n", key, js);
                json_object_put (o);
            } else
                printf ("%s\n", key);
        }
        free (key);
    }
    kvsitr_destroy (itr);
    kvsdir_destroy (dir);
}

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */