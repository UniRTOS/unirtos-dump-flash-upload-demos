/*****************************************************************/ /**
* @file dump_flash_upload_demo.c
* @brief Demo: set dumpcfg to 3 (save dump key info to flash),
*        check if dump info exists, read it out and POST to server via HTTP.
* @date 2026-03-26
*
* @par FLOW
* 1. Set dumpcfg to QOSA_DUMPCFG_ENABLE_FLASH (3)
* 2. Check if dump info has been saved to flash (0x3ed000, 4KB)
* 3. If saved, read dump data from flash
* 4. Upload dump data to server via HTTP POST (using qurl)
*
* @note The dump flash area is inside FOTA backup region,
*       address 0x3ed000, length 4KB.
*       Regarding the address of the DUMP flash, please refer to the partition.txt file in the DBG folder under the version firmware package.
*       As shown in the printed QUEC_FLASH_DUMP_ADDR, this address minus the offset of 0x800000 gives the actual physical address of the flash.
*       After dump occurs and device restarts, the demo will detect
*       the dump data, read it and upload to the configured server.
**********************************************************************/
#include "qosa_sys.h"
#include "qosa_built_in_flash.h"
#include "qosa_def.h"
#include "qosa_log.h"
#include "qosa_dev.h"
#include "qurl_api.h"
#include "qurl_code.h"
#include <string.h>
#include <stdio.h>
#include "unirtos_app_init_registry.h"

/*===========================================================================
 *  Macro Definition
 * ===========================================================================*/
#define QOS_LOG_TAG LOG_TAG_DEMO

#define DUMP_FLASH_ADDR         0x3ed000   /* dump key info flash address (non-XIP) */
#define DUMP_FLASH_SIZE         0x1000     /* 4KB */

/* Server configuration - modify according to actual server */
#define DUMP_UPLOAD_URL        "http://xxx.xxx.xxx.xxx:8080/api/dump/upload"  /* TODO: replace */

/*===========================================================================
 *  Variate
 * ===========================================================================*/
static qosa_task_t g_dump_upload_task = QOSA_NULL;

/*===========================================================================
 *  Static Functions
 * ===========================================================================*/

/**
 * @brief Upload dump data to server via HTTP POST (using qurl)
 *
 * @param[in] data   Pointer to dump data buffer
 * @param[in] len    Data length
 *
 * @return int
 *         - 0: success
 *         - -1: failed
 */
static int dump_flash_upload_to_server(const qosa_uint8_t *data, qosa_uint32_t len)
{
    qurl_core_t core = QOSA_NULL;
    qurl_ecode_t ret;

    /* Create qurl instance */
    ret = qurl_core_create(&core);
    if (ret != QURL_OK)
    {
        QLOGE("qurl_core_create failed, ret=0x%x", ret);
        return -1;
    }

    /* Set URL */
    ret = qurl_core_setopt(core, QURL_OPT_URL, DUMP_UPLOAD_URL);
    if (ret != QURL_OK)
    {
        QLOGE("setopt URL failed, ret=0x%x", ret);
        qurl_core_delete(core);
        return -1;
    }

    /* Set HTTP POST method with multipart form */
    ret = qurl_core_setopt(core, QURL_OPT_HTTP_POST_FORM, 1L);
    if (ret != QURL_OK)
    {
        QLOGE("setopt HTTP_POST_FORM failed, ret=0x%x", ret);
        qurl_core_delete(core);
        return -1;
    }

    /* Set POST form data (binary dump content) */
    {
        qurl_http_form_cfg_t form_data;
        memset(&form_data, 0, sizeof(form_data));
        form_data.name_ptr     = "dumpfile";
        form_data.filename_ptr = "dump_0x3ed000.bin";
        form_data.content_type = QURL_HTTP_FORM_CONTENT_DATA;
        form_data.content_ptr  = (void *)data;
        form_data.content_len  = (long)len;

        ret = qurl_core_setopt(core, QURL_OPT_FORM, &form_data);
        if (ret != QURL_OK)
        {
            QLOGE("setopt FORM failed, ret=0x%x", ret);
            qurl_core_delete(core);
            return -1;
        }
    }

    /* Set timeout (unit: ms) */
    qurl_core_setopt(core, QURL_OPT_TIMEOUT_MS, 60000L);

    QLOGD("uploading dump data (%d bytes) to %s ...", len, DUMP_UPLOAD_URL);

    /* Perform HTTP POST */
    ret = qurl_core_perform(core);
    if (ret != QURL_OK)
    {
        QLOGE("qurl_core_perform failed, ret=0x%x", ret);
        qurl_core_delete(core);
        return -1;
    }

    /* Get HTTP response code */
    {
        long http_code = 0;
        qurl_core_getinfo(core, QURL_INFO_RESP_CODE, &http_code);
        QLOGD("server response code: %ld", http_code);
        if (http_code < 200 || http_code >= 300)
        {
            QLOGE("server returned error code: %ld", http_code);
            qurl_core_delete(core);
            return -1;
        }
    }

    qurl_core_delete(core);
    QLOGD("dump data uploaded successfully");
    return 0;
}

/**
 * @brief Dump flash check and upload task
 *
 * Flow:
 * 1. Set dumpcfg to QOSA_DUMPCFG_ENABLE_FLASH (3)
 * 2. Check if dump info has been saved to flash
 * 3. If saved, read dump data and upload to server
 *
 * @param[in] ctx  Task context, unused
 */
static void dump_flash_upload_process(void *ctx)
{
    qosa_dev_error_e ret;
    qosa_bool_t dump_status;
    qosa_uint8_t *dump_buf = QOSA_NULL;
    qosa_built_nor_error_e flash_ret;

    /* Delay to ensure system is fully initialized */
    qosa_task_sleep_ms(5000);

    /* Step 1: Set dumpcfg to QOSA_DUMPCFG_ENABLE_FLASH (3)
     * When a fault/dump occurs, key info will be saved to flash at 0x3ed000 */
    ret = qosa_dev_set_dumpcfg(QOSA_DUMPCFG_ENABLE_FLASH);
    if (ret != QOSA_DEV_ERRID_SUCCESS)
    {
        QLOGE("set dumpcfg to FLASH failed, ret=%d", ret);
        return;
    }
    QLOGD("dumpcfg set to QOSA_DUMPCFG_ENABLE_FLASH (3) success");

    /* Step 2: Check if dump info has been saved to flash
     * qosa_dev_dump_occure_check returns QOSA_TRUE only when a dump occurred */
    dump_status = qosa_dev_dump_occure_check();
    if (dump_status == QOSA_FALSE)
    {
        QLOGD("no dump data found, waiting for fault to occur...");
        return;
    }
    QLOGD("dump detected! reading dump info from flash 0x%x ...", DUMP_FLASH_ADDR);

    /* Step 3: Allocate buffer and read dump data from flash */
    dump_buf = qosa_malloc(DUMP_FLASH_SIZE);
    if (dump_buf == QOSA_NULL)
    {
        QLOGE("malloc dump buffer failed");
        return;
    }

    flash_ret = qosa_builtin_flash_read(DUMP_FLASH_ADDR, dump_buf, DUMP_FLASH_SIZE);
    if (flash_ret != QOSA_BUILT_NOR_SUCESS)
    {
        QLOGE("read flash failed, ret=%d", flash_ret);
        qosa_free(dump_buf);
        return;
    }

    /* Step 4: Upload dump data to server via HTTP POST */
    if (dump_flash_upload_to_server(dump_buf, DUMP_FLASH_SIZE) != 0)
    {
        QLOGE("upload dump data to server failed");
        qosa_free(dump_buf);
        return;
    }
    else
    {
        QLOGD("upload dump data to server success");
    }

    /* Step 5: erase dump data from flash */
    flash_ret = qosa_builtin_flash_erase(DUMP_FLASH_ADDR, DUMP_FLASH_SIZE);
    if (flash_ret != QOSA_BUILT_NOR_SUCESS)
    {
        QLOGE("read flash failed, ret=%d", flash_ret);
        qosa_free(dump_buf);
        return;
    }

    qosa_free(dump_buf);
}

/*===========================================================================
 *  Public Functions
 * ===========================================================================*/

/**
 * @brief Initialize dump flash check and upload demo
 *
 * Creates a task that:
 * - Sets dumpcfg to 3 (save dump key info to flash)
 * - Checks if dump info exists after restart
 * - Reads dump data and uploads to server via HTTP POST
 */
void dump_flash_upload_demo_init(void)
{
    QLOGD("dump flash upload demo init");

    if (g_dump_upload_task == QOSA_NULL)
    {
        qosa_task_create(
            &g_dump_upload_task,
            4096,
            QOSA_PRIORITY_NORMAL,
            "dump_upload",
            dump_flash_upload_process,
            QOSA_NULL,
            1
        );
    }
}
UNIRTOS_APP_EXPORT(338, "dump_flash_upload_demo", dump_flash_upload_demo_init);
