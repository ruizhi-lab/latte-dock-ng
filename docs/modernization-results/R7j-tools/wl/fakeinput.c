/*
 * R7j scratch pointer driver (not part of the repository).
 *
 * Sends pointer events to a nested KWin session through KWin's org_kde_kwin_fake_input
 * protocol (plasma-wayland-protocols fake-input.xml). Commands come on stdin, one per line:
 *   move X Y    absolute pointer position in the compositor's global coordinates
 *   press       left button down
 *   release     left button up
 *   sync        wait until the compositor has processed everything sent so far
 *   quit        exit
 * Every command is followed by a round trip, so the next command reaches KWin after the
 * previous one has been dispatched. Each answer is echoed on stdout as "FI <command> ok".
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wayland-client.h>
#include <wayland-client-protocol.h>

#include "fake-input-client-protocol.h"

#define BTN_LEFT 0x110

static struct org_kde_kwin_fake_input *fake_input = NULL;

static void registry_global(void *data, struct wl_registry *registry, uint32_t name,
                            const char *interface, uint32_t version)
{
    (void)data;
    if (strcmp(interface, org_kde_kwin_fake_input_interface.name) == 0) {
        uint32_t use = version < 6 ? version : 6;
        fake_input = wl_registry_bind(registry, name, &org_kde_kwin_fake_input_interface, use);
        fprintf(stdout, "FI bound org_kde_kwin_fake_input version %u\n", use);
        fflush(stdout);
    }
}

static void registry_global_remove(void *data, struct wl_registry *registry, uint32_t name)
{
    (void)data;
    (void)registry;
    (void)name;
}

static const struct wl_registry_listener registry_listener = {
    registry_global,
    registry_global_remove,
};

int main(int argc, char **argv)
{
    (void)argc;
    (void)argv;

    struct wl_display *display = wl_display_connect(NULL);
    if (!display) {
        fprintf(stderr, "FI cannot connect to the Wayland display\n");
        return 2;
    }

    struct wl_registry *registry = wl_display_get_registry(display);
    wl_registry_add_listener(registry, &registry_listener, NULL);
    wl_display_roundtrip(display);

    if (!fake_input) {
        fprintf(stdout, "FI org_kde_kwin_fake_input is not offered by this compositor\n");
        fflush(stdout);
        return 3;
    }

    org_kde_kwin_fake_input_authenticate(fake_input, "R7j scratch driver",
                                         "automated check of the activity menu (R7j)");
    wl_display_roundtrip(display);
    fprintf(stdout, "FI authenticate sent\n");
    fflush(stdout);

    char line[256];
    while (fgets(line, sizeof(line), stdin)) {
        size_t length = strlen(line);
        while (length > 0 && (line[length - 1] == '\n' || line[length - 1] == '\r')) {
            line[--length] = '\0';
        }

        if (strncmp(line, "move ", 5) == 0) {
            long x = 0;
            long y = 0;
            if (sscanf(line + 5, "%ld %ld", &x, &y) != 2) {
                fprintf(stdout, "FI bad command '%s'\n", line);
                fflush(stdout);
                continue;
            }
            org_kde_kwin_fake_input_pointer_motion_absolute(fake_input, wl_fixed_from_int((int)x),
                                                            wl_fixed_from_int((int)y));
        } else if (strcmp(line, "press") == 0) {
            org_kde_kwin_fake_input_button(fake_input, BTN_LEFT, WL_POINTER_BUTTON_STATE_PRESSED);
        } else if (strcmp(line, "release") == 0) {
            org_kde_kwin_fake_input_button(fake_input, BTN_LEFT, WL_POINTER_BUTTON_STATE_RELEASED);
        } else if (strcmp(line, "sync") == 0) {
            /* the round trip below is the synchronisation */
        } else if (strcmp(line, "quit") == 0) {
            break;
        } else {
            fprintf(stdout, "FI unknown command '%s'\n", line);
            fflush(stdout);
            continue;
        }

        wl_display_roundtrip(display);
        fprintf(stdout, "FI %s ok\n", line);
        fflush(stdout);
    }

    org_kde_kwin_fake_input_destroy(fake_input);
    wl_display_roundtrip(display);
    wl_registry_destroy(registry);
    wl_display_disconnect(display);
    return 0;
}
