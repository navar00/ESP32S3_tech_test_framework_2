#include "InputHAL.h"
#include <esp_mac.h>

extern "C"
{
#include <uni.h>
}

// --- C Callbacks from Bluepad32 ---

static void bp32_on_init_complete(void)
{
    Logger::log("BP32", "Native Init Complete");
}

static void bp32_on_device_connected(uni_hid_device_t *d)
{
    InputHAL::getInstance().onConnected(d);
}

static void bp32_on_device_disconnected(uni_hid_device_t *d)
{
    InputHAL::getInstance().onDisconnected(d);
}

static uni_error_t bp32_on_device_ready(uni_hid_device_t *d)
{
    Logger::log("BP32", "Device Ready: %s", d->name);
    return UNI_ERROR_SUCCESS;
}

static void bp32_on_controller_data(uni_hid_device_t *d, uni_controller_t *ctl)
{
    // Data is valid here.
}

static const uni_property_t *bp32_get_property(uni_property_idx_t idx)
{
    return NULL;
}

static void bp32_on_oob_event(uni_platform_oob_event_t event, void *data)
{
    if (event == UNI_PLATFORM_OOB_BLUETOOTH_ENABLED)
    {
        Logger::log("BP32", "Bluetooth Enabled - Scanning...");
    }
}

// --- Stubs for safely avoiding NULL pointer dereferences ---

static void bp32_init(int argc, const char **argv)
{
    // Stub: No extra initialization needed
}

static uni_error_t bp32_on_device_discovered(bd_addr_t bd_addr, const char *name, uint16_t cod, uint8_t rssi)
{
    // Log ALL detected devices regardless of RSSI
    Logger::log("BP32", "Discovered: %s (RSSI %d)", name, rssi);
    return UNI_ERROR_SUCCESS;
}

static void bp32_on_gamepad_data(uni_hid_device_t *d, uni_gamepad_t *gp)
{
    // Stub: We use on_controller_data instead
}

// Define the platform
static struct uni_platform my_platform = {
    .name = "ESP32_TechTest_Platform",
    .init = bp32_init,
    .on_init_complete = bp32_on_init_complete,
    .on_device_discovered = bp32_on_device_discovered,
    .on_device_connected = bp32_on_device_connected,
    .on_device_disconnected = bp32_on_device_disconnected,
    .on_device_ready = bp32_on_device_ready,
    .on_gamepad_data = bp32_on_gamepad_data,
    .on_controller_data = bp32_on_controller_data,
    .get_property = bp32_get_property,
    .on_oob_event = bp32_on_oob_event,
    .device_dump = NULL,
    .register_console_cmds = NULL};

InputHAL *InputHAL::instance = nullptr;

InputHAL &InputHAL::getInstance()
{
    if (instance == nullptr)
    {
        instance = new InputHAL();
    }
    return *instance;
}

InputHAL::InputHAL()
{
    for (int i = 0; i < BP32_MAX_CONTROLLERS; i++)
        controllers[i] = nullptr;
}

void InputHAL::init()
{
    Logger::log("BP32", "Initializing Bluepad32 Native...");

    // Set custom platform
    uni_platform_set_custom(&my_platform);

    // Initialize the library
    uni_init(0, NULL);

    // DEBUG: Clear keys to ensure fresh pairing if needed
    // uni_bt_del_keys_unsafe();
    // DEBUG: Explicitly enable new connections (Scanning)
    uni_bt_enable_new_connections_unsafe(true);

    printAddress();
}

void InputHAL::update()
{
}

void InputHAL::printAddress()
{
    Logger::log("BP32", "MAC Address: %s", getLocalMacAddress().c_str());
}

String InputHAL::getLocalMacAddress()
{
    uint8_t mac[6];
    esp_read_mac(mac, ESP_MAC_BT);
    char buf[18];
    snprintf(buf, sizeof(buf), "%02X:%02X:%02X:%02X:%02X:%02X",
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    return String(buf);
}

void InputHAL::onConnected(uni_hid_device_t *d)
{
    Logger::log("BP32", "Controller Connected: %s", d->name);
    // Store in first free slot
    for (int i = 0; i < BP32_MAX_CONTROLLERS; i++)
    {
        if (controllers[i] == nullptr)
        {
            controllers[i] = d;
            return;
        }
    }
}

void InputHAL::onDisconnected(uni_hid_device_t *d)
{
    Logger::log("BP32", "Controller Disconnected");
    for (int i = 0; i < BP32_MAX_CONTROLLERS; i++)
    {
        if (controllers[i] == d)
        {
            controllers[i] = nullptr;
            return;
        }
    }
}
