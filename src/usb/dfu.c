
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "tusb.h"
#include "ch32v30x.h"
#include "util.h"
#include "led_blinker.h"
#include "vkart_flash.h"


// DFU -- internal state

struct state {
	uint32_t offset;
	uint32_t maxlen;
	uint32_t crcacc;
	enum action { act_none = 0, act_upload = 1, act_download = 2 } curact;
	bool stop;
} state;

#define CRC32_INITIAL (~(uint32_t)0)

// DFU -- internal fuctions

static bool init_base(void) {
	state.offset = 0;
	state.maxlen = VKART_MEMORY_WORDSZ<<1;
	state.crcacc = CRC32_INITIAL;
	state.curact = act_none;
	state.stop = false;

	return true;
}
static bool init_upload(void) {
	if (state.curact != act_none) {
		tud_dfu_finish_flashing(DFU_STATUS_ERR_UNKNOWN);
		return false;
	}

	if (!init_base()) goto err;

	iprintf("[DFU] init upload\r\n");
	state.curact = act_upload;
	led_blinker_set(led_reading);
	return true;

err:
	iprintf("[DFU] init upload FAIL\r\n");
	tud_dfu_finish_flashing(DFU_STATUS_ERR_FILE);
	return false;
}
static void deinit_upload(void) {
	iprintf("[DFU] deinit upload\r\n");
	led_blinker_set(led_waiting);
	state.curact = act_none;
}
static bool init_download(void) {
	if (state.curact != act_none) {
		tud_dfu_finish_flashing(DFU_STATUS_ERR_UNKNOWN);
		return false;
	}

	if (!init_base()) goto err;

	iprintf("[DFU] init download\r\n");

	if (!vkart_wrimage_start()) {
		iprintf("[DFU] can't start DL!\r\n");

		goto err;
	}

	state.curact = act_download;
	led_blinker_set(led_writing);
	return true;

err:
	iprintf("[DFU] init download FAIL\r\n");
	tud_dfu_finish_flashing(DFU_STATUS_ERR_FILE);
	return false;
}
static void deinit_download(void) {
	iprintf("[DFU] deinit download\r\n");
	vkart_wrimage_finish();
	led_blinker_set(led_waiting);
	state.curact = act_none;
}


//--------------------------------------------------------------------+
// DFU callbacks
// Note: alt is used as the partition number, in order to support multiple partitions like FLASH, EEPROM, etc.
//--------------------------------------------------------------------+

// Invoked right before tud_dfu_download_cb() (state=DFU_DNBUSY) or tud_dfu_manifest_cb() (state=DFU_MANIFEST)
// Application return timeout in milliseconds (bwPollTimeout) for the next download/manifest operation.
// During this period, USB host won't try to communicate with us.
uint32_t tud_dfu_get_timeout_cb(uint8_t alt, uint8_t state) {
	const uint32_t timeout_busy
		= 90 /* erase time 90ms */
		+ 350/*328*/ /* double write: 20us * 16k pages */
		+ 1000 /* CRC calc? */
		;
	const uint32_t timeout_manifest = 1000; // TODO: fill this in when we actually calculate a CRC or anything

	//iprintf(" [DFU] get timeout alt=%u state=%u\r\n", alt, state);
	if (state == DFU_DNBUSY) {
		return timeout_busy;
	} else if (state == DFU_MANIFEST) {
		return timeout_busy + timeout_manifest; // may need final data flush here
	}

	return 0;
}

// Invoked when received DFU_DNLOAD (wLength>0) following by DFU_GETSTATUS (state=DFU_DNBUSY) requests
// This callback could be returned before flashing op is complete (async).
// Once finished flashing, application must call tud_dfu_finish_flashing()
void tud_dfu_download_cb(uint8_t alt, uint16_t block_num, uint8_t const* data, uint16_t len) {
	(void)alt;

	//iprintf("[DFU] download alt=%u block=%u length=%u\r\n", alt, block_num, len);

	if (len & 1) { // no unaligned writes, sorry
		tud_dfu_finish_flashing(DFU_STATUS_ERR_ADDRESS);
		return;
	}
	if (state.curact != act_download) {
		if (block_num == 0) { // first block? time to init stuff then
			if (!init_download()) return;
		} else {
			tud_dfu_finish_flashing(DFU_STATUS_ERR_UNKNOWN);
			return;
		}
	}

	if (state.offset + len >= state.maxlen) {
		// too much, truncate
		int64_t llen = state.maxlen - state.offset;
		if (llen < 0 || len > UINT16_MAX) {
			tud_dfu_finish_flashing(DFU_STATUS_ERR_ADDRESS);
			return;
		}
		len = (uint16_t)len;
	}

	if (!state.stop) {
		state.crcacc = crc32(state.crcacc, data, len);
		//iprintf("[DFU] CRC at %08lx is: %08lx\r\n", state.offset, state.crcacc);
		state.stop = vkart_wrimage_next((const uint16_t*)data, len >> 1);
		state.offset += len;
	}
	if (state.stop) {
		iprintf("[DFU] STOP!\r\n");
	}

	// flashing op for download complete without error
	tud_dfu_finish_flashing(DFU_STATUS_OK);
	return;
}

// Invoked when download process is complete, received DFU_DNLOAD (wLength=0) following by DFU_GETSTATUS (state=Manifest)
// Application can do checksum, or actual flashing if buffered entire image previously.
// Once finished flashing, application must call tud_dfu_finish_flashing()
void tud_dfu_manifest_cb(uint8_t alt) {
	(void)alt;
	iprintf("[DFU] manifest\r\n");

	if (state.curact != act_download) {
		tud_dfu_finish_flashing(DFU_STATUS_ERR_UNKNOWN);
		return;
	}

	vkart_wrimage_finish();
	//add_data_block(NULL, 0);

	uint32_t check_acc = CRC32_INITIAL;
	uint32_t total_len_b = state.offset;

	for (uint32_t block = 0, todo = 4096/*VKART_BUFFER_WORDSZ*sizeof(uint16_t)*/;
			block < total_len_b;
			block += todo) {
		if (block + todo > total_len_b) todo = total_len_b - block;

		vkart_read_data(block >> 1, vkart_data_buffer, todo >> 1);
		check_acc = crc32(check_acc, vkart_data_buffer, todo);
		//iprintf("[DFU] check CRC at %08lx is: %08lx\r\n", block, check_acc);
	}

	iprintf("[DFU] CRC manifest check: %08lx (write) vs %08lx (check)\r\n", state.crcacc, check_acc);
	bool verify_good = check_acc == state.crcacc;

	deinit_download();

	if (verify_good) {
		// flashing op for manifest is complete without error
		// Application can perform checksum, should it fail, use appropriate status such as errVERIFY.
		tud_dfu_finish_flashing(DFU_STATUS_OK);
	} else {
		tud_dfu_finish_flashing(DFU_STATUS_ERR_VERIFY);
	}
}

// Invoked when received DFU_UPLOAD request
// Application must populate data with up to length bytes and
// Return the number of written bytes
uint16_t tud_dfu_upload_cb(uint8_t alt, uint16_t block_num, uint8_t* data, uint16_t len) {
	(void)alt;
	//iprintf("[DFU] upload, alt=%u, block_num=%u, len=%u\r\n", alt, block_num, len);

	if (len & 1) { // no unaligned reads, sorry
		tud_dfu_finish_flashing(DFU_STATUS_ERR_ADDRESS);
		return 0;
	}
	if (state.curact != act_upload) {
		if (block_num == 0) {
			if (!init_upload()) return 0;
		} else tud_dfu_finish_flashing(DFU_STATUS_ERR_UNKNOWN);
	}

	bool need_exit = false;

	uint32_t len_todo = len;
	if (state.offset + len_todo >= state.maxlen) {
		len_todo = state.maxlen - state.offset;
		need_exit = true;
	}

	vkart_read_data(state.offset >> 1, (uint16_t*)data, len_todo >> 1);
	state.offset += len_todo;

	if (need_exit) deinit_upload();

	return len_todo;
}

// Invoked when the Host has terminated a download or upload transfer
void tud_dfu_abort_cb(uint8_t alt) {
	(void)alt;
	iprintf("[DFU] abort\r\n");

	if (state.curact == act_upload) deinit_upload();
	if (state.curact == act_download) deinit_download();
}

// Invoked when a DFU_DETACH request is received
void tud_dfu_detach_cb(void) {
	iprintf("[DFU] detach\r\n");
}

