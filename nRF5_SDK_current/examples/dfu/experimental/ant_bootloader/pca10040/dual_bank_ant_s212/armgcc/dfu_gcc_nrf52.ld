/* Linker script to configure memory regions. */

SEARCH_DIR(.)
GROUP(-lgcc -lc -lnosys)

MEMORY
{
  FLASH (rx) : ORIGIN = 0x79000, LENGTH = 0x5000
  RAM (rwx) :  ORIGIN = 0x20002800, LENGTH = 0xd800
  boot_settings_pending (r) : ORIGIN = 0x0007E000, LENGTH = 0x1000
  boot_settings (r) : ORIGIN = 0x0007F000, LENGTH = 0x1000
  uicr_boot_start_address (r) : ORIGIN = 0x10001014, LENGTH = 0x4
  uicr_mbr_retaining_address (r) : ORIGIN = 0x10001018, LENGTH = 0x4
}

SECTIONS
{
  .boot_settings_pending(NOLOAD) :
  {
    PROVIDE(__start_boot_settings_pending = .);
    KEEP(*(SORT(.boot_settings_pending*)))
    PROVIDE(__stop_boot_settings_pending = .);
  } > boot_settings_pending
  .boot_settings(NOLOAD) :
  {
    PROVIDE(__start_boot_settings = .);
    KEEP(*(SORT(.boot_settings*)))
    PROVIDE(__stop_boot_settings = .);
  } > boot_settings
  .uicr_boot_start_address :
  {
    PROVIDE(__start_uicr_boot_start_address = .);
    KEEP(*(SORT(.uicr_boot_start_address*)))
    PROVIDE(__stop_uicr_boot_start_address = .);
  } > uicr_boot_start_address
  .uicr_mbr_retaining_address :
  {
    PROVIDE(__start_uicr_mbr_retaining_address = .);
    KEEP(*(SORT(.uicr_mbr_retaining_address*)))
    PROVIDE(__stop_uicr_mbr_retaining_address = .);
  } > uicr_mbr_retaining_address
}

SECTIONS
{
  . = ALIGN(4);
  .mem_section_dummy_ram :
  {
  }
  .log_dynamic_data :
  {
    PROVIDE(__start_log_dynamic_data = .);
    KEEP(*(SORT(.log_dynamic_data*)))
    PROVIDE(__stop_log_dynamic_data = .);
  } > RAM

} INSERT AFTER .data;

SECTIONS
{
  .mem_section_dummy_rom :
  {
  }
  .sdh_ant_observers :
  {
    PROVIDE(__start_sdh_ant_observers = .);
    KEEP(*(SORT(.sdh_ant_observers*)))
    PROVIDE(__stop_sdh_ant_observers = .);
  } > FLASH
  .sdh_soc_observers :
  {
    PROVIDE(__start_sdh_soc_observers = .);
    KEEP(*(SORT(.sdh_soc_observers*)))
    PROVIDE(__stop_sdh_soc_observers = .);
  } > FLASH
  .log_const_data :
  {
    PROVIDE(__start_log_const_data = .);
    KEEP(*(SORT(.log_const_data*)))
    PROVIDE(__stop_log_const_data = .);
  } > FLASH
    .nrf_balloc :
  {
    PROVIDE(__start_nrf_balloc = .);
    KEEP(*(.nrf_balloc))
    PROVIDE(__stop_nrf_balloc = .);
  } > FLASH
  .sdh_state_observers :
  {
    PROVIDE(__start_sdh_state_observers = .);
    KEEP(*(SORT(.sdh_state_observers*)))
    PROVIDE(__stop_sdh_state_observers = .);
  } > FLASH
  .sdh_stack_observers :
  {
    PROVIDE(__start_sdh_stack_observers = .);
    KEEP(*(SORT(.sdh_stack_observers*)))
    PROVIDE(__stop_sdh_stack_observers = .);
  } > FLASH
  .sdh_req_observers :
  {
    PROVIDE(__start_sdh_req_observers = .);
    KEEP(*(SORT(.sdh_req_observers*)))
    PROVIDE(__stop_sdh_req_observers = .);
  } > FLASH

} INSERT AFTER .text

INCLUDE "nrf_common.ld"
