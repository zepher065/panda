bool bootkick_ign_prev = false;
BootState boot_state = BOOT_BOOTKICK;
uint8_t bootkick_harness_status_prev = HARNESS_STATUS_NC;

uint8_t boot_reset_countdown = 0;
uint8_t waiting_to_boot_countdown = 0;
bool bootkick_reset_triggered = false;
uint16_t bootkick_last_serial_ptr = 0;

void bootkick_tick(bool ignition, bool recent_heartbeat) {
  const bool harness_inserted = (harness.status != bootkick_harness_status_prev) && (harness.status != HARNESS_STATUS_NC);


  /*
    Ensure SOM boots in case it goes into QDL mode. Reset behavior:
    * shouldn't trigger on the first boot after power-on
    * only try reset once per bootkick, i.e. don't keep trying until booted
    * only try once per panda boot, since openpilot will reset panda on startup
    * once BOOT_RESET is triggered, it stays until countdown is finished
  */
  if (boot_state == BOOT_STANDBY) {
    if ((ignition && !bootkick_ign_prev) || harness_inserted) {
    // bootkick on rising edge of ignition or harness insertion,
    // start countdown if we haven't previously tried bootkick reset
    boot_state = BOOT_BOOTKICK;
    if (!bootkick_reset_triggered) {
      waiting_to_boot_countdown = 45U;
    }

  } else if (boot_state == BOOT_BOOTKICK) {
    if (recent_heartbeat) {
      // disable bootkick once openpilot is up
      waiting_to_boot_countdown = 0U;
      boot_state = BOOT_STANDBY;
    } else {
      if (waiting_to_boot_countdown > 0U) {
        waiting_to_boot_countdown--;
        bool serial_activity = uart_ring_som_debug.w_ptr_tx != bootkick_last_serial_ptr;
        if (serial_activity || current_board->read_som_gpio()) {
          waiting_to_boot_countdown = 0U;
        } else {
          // try a reset
          if (waiting_to_boot_countdown == 0U) {
            boot_reset_countdown = 5U;
            boot_state = BOOT_RESET;
          }
        }
      }
    }

  } else if (boot_state == BOOT_RESET) {
    boot_reset_countdown--;
    bootkick_reset_triggered = true;
    if (boot_reset_countdown == 0U) {
      boot_state = BOOT_STANDBY;
    }

  } else {
  }

  // update state
  bootkick_ign_prev = ignition;
  bootkick_harness_status_prev = harness.status;
  bootkick_last_serial_ptr = uart_ring_som_debug.w_ptr_tx;
  current_board->set_bootkick(boot_state);
}
