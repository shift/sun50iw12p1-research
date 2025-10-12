/*
 * Minimal FES (FEL Stage 2) Loader for Allwinner H713
 * 
 * Purpose: Bypass H713 BROM USB bulk transfer bug by providing
 *          a small (<16 KB) Stage 2 loader that handles large
 *          memory operations without BROM involvement.
 *
 * Architecture:
 *   Stage 1 (BROM FEL): Upload this 16 KB FES loader -> Success
 *   Stage 2 (FES Protocol): Upload U-Boot SPL (732 KB) -> Bypasses BROM bug
 *
 * Memory Map:
 *   - Load Address: 0x121000 (Task 027 scratch buffer)
 *   - Stack:        0x125000 (grows down)
 *   - Max Size:     16 KB (0x4000 bytes)
 *
 * Protocol: Based on PhoenixSuit V1.10 reverse engineering (Task 034)
 *   - Command/Response pattern (USB bulk transfers)
 *   - Memory read/write/execute operations
 *   - USB endpoints inherited from BROM FEL mode
 *
 * Status: SKELETON - Requires USB protocol validation via traffic capture
 */

#include <stdint.h>

/* FES Protocol Constants (HYPOTHETICAL - needs validation) */
#define FES_MAGIC_CMD       0x46455843  /* "FEXC" */
#define FES_MAGIC_RESP      0x46455852  /* "FEXR" */

/* FES Command Opcodes (inferred from PhoenixSuit strings) */
#define FES_CMD_VERIFY      0x0001      /* Device verification */
#define FES_CMD_IS_READY    0x0002      /* Ready status check */
#define FES_CMD_FEL_UP      0x0101      /* Memory write (upload) */
#define FES_CMD_FEL_DOWN    0x0102      /* Memory read (download) */
#define FES_CMD_FET_RUN     0x0103      /* Execute at address */

/* FES Response Status Codes */
#define FES_STATUS_OK       0x0000
#define FES_STATUS_ERROR    0xFFFF

/* H713 USB OTG Controller Base Address (from H616 reference) */
#define USB0_BASE           0x05100000
#define USB_OTG_BASE        USB0_BASE

/* USB Endpoint Definitions (NEEDS VALIDATION) */
#define USB_EP_OUT          0x01        /* Bulk OUT endpoint */
#define USB_EP_IN           0x81        /* Bulk IN endpoint */

/* FES Command Packet Structure (32 bytes) */
struct fes_command {
    uint32_t magic;         /* 0x46455843 "FEXC" */
    uint32_t command;       /* Command opcode */
    uint32_t address;       /* Memory address */
    uint32_t length;        /* Data length */
    uint32_t flags;         /* Operation flags */
    uint8_t  reserved[12];  /* Padding */
} __attribute__((packed));

/* FES Response Packet Structure (32 bytes) */
struct fes_response {
    uint32_t magic;         /* 0x46455852 "FEXR" */
    uint32_t status;        /* Status code */
    uint32_t data_length;   /* Following data length */
    uint8_t  reserved[20];  /* Padding */
} __attribute__((packed));

/* Device Identifier String (from PhoenixSuit eFex.dll) */
static const char fes_device_id[] = "AWUSBFEX";

/*
 * USB Communication Functions
 * 
 * CRITICAL: These are stubs requiring actual H713 USB controller implementation
 * Needs:
 *   - H713 USB OTG register definitions
 *   - Bulk transfer handlers
 *   - USB state management
 */

/* Receive command packet from USB (32 bytes) */
static int usb_receive_command(struct fes_command *cmd)
{
    /* TODO: Implement USB bulk IN transfer
     * - Read 32 bytes from USB_EP_OUT
     * - Validate magic number
     * - Return 0 on success, -1 on error
     */
    (void)cmd;
    return -1;  /* Not implemented */
}

/* Send response packet to USB (32 bytes) */
static int usb_send_response(struct fes_response *resp)
{
    /* TODO: Implement USB bulk OUT transfer
     * - Write 32 bytes to USB_EP_IN
     * - Set magic and status
     * - Return 0 on success, -1 on error
     */
    (void)resp;
    return -1;  /* Not implemented */
}

/* Receive data from USB (variable length) */
static int usb_receive_data(void *buffer, uint32_t length)
{
    /* TODO: Implement USB bulk data transfer
     * - Receive 'length' bytes from USB_EP_OUT
     * - May require chunking for large transfers
     * - Return bytes received or -1 on error
     */
    (void)buffer;
    (void)length;
    return -1;  /* Not implemented */
}

/* Send data to USB (variable length) */
static int usb_send_data(void *buffer, uint32_t length)
{
    /* TODO: Implement USB bulk data transfer
     * - Send 'length' bytes to USB_EP_IN
     * - May require chunking for large transfers
     * - Return bytes sent or -1 on error
     */
    (void)buffer;
    (void)length;
    return -1;  /* Not implemented */
}

/*
 * FES Command Handlers
 */

/* Handle verify device command - respond with device ID */
static void handle_verify(struct fes_command *cmd, struct fes_response *resp)
{
    (void)cmd;
    
    resp->magic = FES_MAGIC_RESP;
    resp->status = FES_STATUS_OK;
    resp->data_length = sizeof(fes_device_id);
    
    usb_send_response(resp);
    usb_send_data((void *)fes_device_id, sizeof(fes_device_id));
}

/* Handle ready status check */
static void handle_ready(struct fes_command *cmd, struct fes_response *resp)
{
    (void)cmd;
    
    resp->magic = FES_MAGIC_RESP;
    resp->status = FES_STATUS_OK;  /* Always ready */
    resp->data_length = 0;
}

/* Handle memory write (upload data to device) */
static void handle_write(struct fes_command *cmd, struct fes_response *resp)
{
    void *dest = (void *)(uintptr_t)cmd->address;
    
    /* Receive data from USB and write to memory */
    int received = usb_receive_data(dest, cmd->length);
    
    resp->magic = FES_MAGIC_RESP;
    resp->status = (received > 0) ? FES_STATUS_OK : FES_STATUS_ERROR;
    resp->data_length = 0;
}

/* Handle memory read (download data from device) */
static void handle_read(struct fes_command *cmd, struct fes_response *resp)
{
    void *src = (void *)(uintptr_t)cmd->address;
    
    /* Send response first, then data */
    resp->magic = FES_MAGIC_RESP;
    resp->status = FES_STATUS_OK;
    resp->data_length = cmd->length;
    
    usb_send_response(resp);
    usb_send_data(src, cmd->length);
}

/* Handle execute command (jump to address) */
static void handle_execute(struct fes_command *cmd, struct fes_response *resp)
{
    void (*entry)(void) = (void (*)(void))(uintptr_t)cmd->address;
    
    /* Send success response before jumping */
    resp->magic = FES_MAGIC_RESP;
    resp->status = FES_STATUS_OK;
    resp->data_length = 0;
    usb_send_response(resp);
    
    /* Jump to address - may not return */
    entry();
    
    /* If entry() returns, continue FES loop */
}

/*
 * Main FES Command Loop
 */
static void fes_command_loop(void)
{
    struct fes_command cmd;
    struct fes_response resp;
    
    while (1) {
        /* Receive command from host */
        if (usb_receive_command(&cmd) < 0)
            continue;  /* Wait for valid command */
        
        /* Validate magic number */
        if (cmd.magic != FES_MAGIC_CMD) {
            resp.magic = FES_MAGIC_RESP;
            resp.status = FES_STATUS_ERROR;
            resp.data_length = 0;
            usb_send_response(&resp);
            continue;
        }
        
        /* Dispatch command */
        switch (cmd.command) {
            case FES_CMD_VERIFY:
                handle_verify(&cmd, &resp);
                break;
                
            case FES_CMD_IS_READY:
                handle_ready(&cmd, &resp);
                break;
                
            case FES_CMD_FEL_UP:
                handle_write(&cmd, &resp);
                break;
                
            case FES_CMD_FEL_DOWN:
                handle_read(&cmd, &resp);
                break;
                
            case FES_CMD_FET_RUN:
                handle_execute(&cmd, &resp);
                /* May not return if execute successful */
                continue;
                
            default:
                /* Unknown command */
                resp.magic = FES_MAGIC_RESP;
                resp.status = FES_STATUS_ERROR;
                resp.data_length = 0;
                break;
        }
        
        /* Send response (if not already sent by handler) */
        if (cmd.command != FES_CMD_VERIFY && 
            cmd.command != FES_CMD_FEL_DOWN) {
            usb_send_response(&resp);
        }
    }
}

/*
 * FES Loader Entry Point
 * 
 * Called by BROM FEL after uploading this loader to 0x121000
 */
void _start(void)
{
    /* TODO: Initialize stack pointer to 0x125000 */
    /* TODO: Initialize USB controller (may already be initialized by BROM) */
    /* TODO: Setup USB endpoints (bulk IN/OUT) */
    
    /* Enter main command loop */
    fes_command_loop();
    
    /* Never returns */
    while (1);
}

/*
 * Size Validation
 * 
 * This loader MUST be < 16 KB to avoid triggering H713 BROM USB bug
 * Check with: aarch64-unknown-linux-gnu-size fes_loader.elf
 * 
 * Target size budget:
 *   text + data + bss < 16384 bytes (0x4000)
 */
