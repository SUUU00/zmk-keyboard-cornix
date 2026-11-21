/*
 * SPDX-License-Identifier: MIT
 *
 * Keep one keymap layer in sync with the selected BLE profile:
 * the layer is on while profile CONFIG_CORNIX_PROFILE_LAYER_PROFILE is
 * selected and off for every other profile. Runs on the central (left) half.
 */

#include <zephyr/kernel.h>
#include <zephyr/init.h>
#include <zephyr/logging/log.h>

#include <zmk/ble.h>
#include <zmk/keymap.h>
#include <zmk/event_manager.h>
#include <zmk/events/ble_active_profile_changed.h>

LOG_MODULE_REGISTER(cornix_profile_layer, CONFIG_ZMK_LOG_LEVEL);

static void sync_layer(uint8_t profile_index) {
    zmk_keymap_layer_id_t layer = zmk_keymap_layer_index_to_id(CONFIG_CORNIX_PROFILE_LAYER_LAYER);

    if (layer == ZMK_KEYMAP_LAYER_ID_INVAL) {
        return;
    }

    bool want = profile_index == CONFIG_CORNIX_PROFILE_LAYER_PROFILE;

    if (want == zmk_keymap_layer_active(layer)) {
        return;
    }

    LOG_DBG("profile %d -> layer %d %s", profile_index, layer, want ? "on" : "off");

    if (want) {
        zmk_keymap_layer_activate(layer, false);
    } else {
        zmk_keymap_layer_deactivate(layer, false);
    }
}

static int profile_layer_listener(const zmk_event_t *eh) {
    const struct zmk_ble_active_profile_changed *ev = as_zmk_ble_active_profile_changed(eh);

    if (ev != NULL) {
        sync_layer(ev->index);
    }

    return ZMK_EV_EVENT_BUBBLE;
}

ZMK_LISTENER(cornix_profile_layer, profile_layer_listener);
ZMK_SUBSCRIPTION(cornix_profile_layer, zmk_ble_active_profile_changed);

/* After boot the saved profile is restored without an event until a host
 * connects, so check it once shortly after startup as well. */
static void boot_sync_work_cb(struct k_work *work) { sync_layer(zmk_ble_active_profile_index()); }

static K_WORK_DELAYABLE_DEFINE(boot_sync_work, boot_sync_work_cb);

static int profile_layer_init(void) {
    k_work_schedule(&boot_sync_work, K_SECONDS(2));
    return 0;
}

SYS_INIT(profile_layer_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);
