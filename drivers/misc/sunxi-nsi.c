// SPDX-License-Identifier: GPL-2.0
/*
 * Allwinner SUNXI Network Service Interface Driver
 * 
 * Copyright (C) 2025 HY300 Linux Porting Project
 * 
 * This driver provides the foundation ARM-MIPS communication interface 
 * with shared memory management and interrupt-driven command/response protocol.
 * 
 * Based on Task 019 specifications from MISSING_DRIVERS_IMPLEMENTATION_SPEC.md
 */

#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/of.h>
#include <linux/of_reserved_mem.h>
#include <linux/clk.h>
#include <linux/reset.h>
#include <linux/dma-mapping.h>
#include <linux/crc32.h>
#include <linux/mutex.h>
#include <linux/completion.h>
#include <linux/atomic.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/slab.h>

// Register definitions (based on factory 0x3061000 base)
#define NSI_CTRL_REG            0x0000    // Control register
#define NSI_STATUS_REG          0x0004    // Status register
#define NSI_IRQ_EN_REG          0x0008    // Interrupt enable
#define NSI_IRQ_STATUS_REG      0x000C    // Interrupt status
#define NSI_MEM_BASE_REG        0x0010    // Shared memory base
#define NSI_MEM_SIZE_REG        0x0014    // Shared memory size
#define NSI_CMD_REG             0x0018    // Command register
#define NSI_DATA_REG            0x001C    // Data register
#define NSI_SEQUENCE_REG        0x0020    // Sequence counter
#define NSI_TIMEOUT_REG         0x0024    // Timeout control

// Control register bits
#define NSI_CTRL_ENABLE         BIT(0)    // Enable NSI interface
#define NSI_CTRL_RESET          BIT(1)    // Reset NSI state machine
#define NSI_CTRL_IRQ_EN         BIT(2)    // Enable interrupts
#define NSI_CTRL_MEM_EN         BIT(3)    // Enable shared memory

// Status register bits
#define NSI_STATUS_READY        BIT(0)    // NSI ready for commands
#define NSI_STATUS_BUSY         BIT(1)    // Command in progress
#define NSI_STATUS_ERROR        BIT(2)    // Last command error
#define NSI_STATUS_MIPS_READY   BIT(3)    // MIPS co-processor ready

// Interrupt bits
#define NSI_IRQ_CMD_COMPLETE    BIT(0)    // Command completion
#define NSI_IRQ_MIPS_NOTIFY     BIT(1)    // MIPS notification
#define NSI_IRQ_ERROR           BIT(2)    // Error interrupt
#define NSI_IRQ_TIMEOUT         BIT(3)    // Timeout interrupt

// Command/Response buffer offsets in shared memory
#define NSI_CMD_BUFFER_OFFSET   0x1000    // 4KB into shared memory
#define NSI_RESP_BUFFER_OFFSET  0x1200    // 4.5KB into shared memory

struct sunxi_nsi_device {
    struct device *dev;
    void __iomem *base;
    struct clk *bus_clk;
    struct clk *mips_clk;
    struct reset_control *reset;
    int irq;
    
    // Shared memory management
    void *shared_mem_virt;
    dma_addr_t shared_mem_phys;
    size_t shared_mem_size;
    
    // Command synchronization
    struct mutex cmd_lock;
    struct completion cmd_complete;
    atomic_t sequence_counter;
    
    // Command buffer (512 bytes from ARM_MIPS_COMMUNICATION_PROTOCOL.md)
    struct nsi_command_buffer *cmd_buffer;
    struct nsi_response_buffer *resp_buffer;
};

struct nsi_command_buffer {
    uint32_t magic;              // 0x4D495053 ("MIPS")
    uint32_t command_id;         // Command identifier
    uint32_t sequence;           // Command sequence number
    uint32_t data_length;        // Length of command data
    uint8_t  data[496];          // Command payload
    uint32_t checksum;           // CRC32 checksum
};

struct nsi_response_buffer {
    uint32_t magic;              // 0x53504952 ("SPIR")
    uint32_t sequence;           // Matching sequence number
    uint32_t status;             // Response status
    uint32_t data_length;        // Response data length
    uint8_t  data[496];          // Response payload
    uint32_t checksum;           // CRC32 checksum
};

// Global device pointer for API access
static struct sunxi_nsi_device *global_nsi_device = NULL;

// Forward declarations
static int sunxi_nsi_probe(struct platform_device *pdev);
static void sunxi_nsi_remove(struct platform_device *pdev);

// Interrupt handler
static irqreturn_t sunxi_nsi_irq_handler(int irq, void *dev_id)
{
    struct sunxi_nsi_device *nsi = dev_id;
    uint32_t status;
    
    status = readl(nsi->base + NSI_IRQ_STATUS_REG);
    
    if (status & NSI_IRQ_CMD_COMPLETE) {
        // Command completion - wake up waiting thread
        complete(&nsi->cmd_complete);
        status &= ~NSI_IRQ_CMD_COMPLETE;
    }
    
    if (status & NSI_IRQ_ERROR) {
        dev_err(nsi->dev, "NSI command error occurred\n");
        complete(&nsi->cmd_complete);  // Wake up with error
        status &= ~NSI_IRQ_ERROR;
    }
    
    if (status & NSI_IRQ_TIMEOUT) {
        dev_warn(nsi->dev, "NSI command timeout\n");
        complete(&nsi->cmd_complete);  // Wake up with timeout
        status &= ~NSI_IRQ_TIMEOUT;
    }
    
    // Clear processed interrupts
    writel(status, nsi->base + NSI_IRQ_STATUS_REG);
    
    return IRQ_HANDLED;
}

// Public API for upper layer drivers
int sunxi_nsi_send_command(struct sunxi_nsi_device *nsi, uint32_t cmd_id,
                          const void *data, size_t data_len,
                          uint32_t *response_status, void *response_data,
                          size_t *response_len, int timeout_ms)
{
    struct nsi_command_buffer *cmd = nsi->cmd_buffer;
    struct nsi_response_buffer *resp = nsi->resp_buffer;
    uint32_t sequence;
    int ret = 0;
    
    if (data_len > sizeof(cmd->data))
        return -EINVAL;
    
    mutex_lock(&nsi->cmd_lock);
    
    // Check if NSI is ready
    if (!(readl(nsi->base + NSI_STATUS_REG) & NSI_STATUS_READY)) {
        ret = -EBUSY;
        goto unlock;
    }
    
    // Prepare command
    sequence = atomic_inc_return(&nsi->sequence_counter);
    cmd->magic = 0x4D495053;
    cmd->command_id = cmd_id;
    cmd->sequence = sequence;
    cmd->data_length = data_len;
    
    if (data && data_len > 0)
        memcpy(cmd->data, data, data_len);
    
    // Calculate checksum
    cmd->checksum = crc32(0, (const uint8_t*)cmd, 
                         sizeof(*cmd) - sizeof(cmd->checksum));
    
    // Clear completion
    reinit_completion(&nsi->cmd_complete);
    
    // Send command to MIPS (implementation depends on shared memory layout)
    memcpy_toio(nsi->shared_mem_virt + NSI_CMD_BUFFER_OFFSET, cmd, sizeof(*cmd));
    
    // Trigger MIPS interrupt
    writel(cmd_id, nsi->base + NSI_CMD_REG);
    writel(NSI_CTRL_ENABLE | NSI_CTRL_IRQ_EN, nsi->base + NSI_CTRL_REG);
    
    // Wait for response
    ret = wait_for_completion_timeout(&nsi->cmd_complete, 
                                     msecs_to_jiffies(timeout_ms));
    if (ret == 0) {
        dev_err(nsi->dev, "Command 0x%x timeout after %dms\n", cmd_id, timeout_ms);
        ret = -ETIMEDOUT;
        goto unlock;
    }
    
    // Read response
    memcpy_fromio(resp, nsi->shared_mem_virt + NSI_RESP_BUFFER_OFFSET, sizeof(*resp));
    
    // Validate response
    if (resp->magic != 0x53504952 || resp->sequence != sequence) {
        dev_err(nsi->dev, "Invalid response: magic=0x%x, seq=%d (expected %d)\n",
                resp->magic, resp->sequence, sequence);
        ret = -EBADMSG;
        goto unlock;
    }
    
    // Verify checksum
    uint32_t expected_checksum = crc32(0, (const uint8_t*)resp,
                                      sizeof(*resp) - sizeof(resp->checksum));
    if (resp->checksum != expected_checksum) {
        dev_err(nsi->dev, "Response checksum mismatch\n");
        ret = -EBADMSG;
        goto unlock;
    }
    
    // Return response data
    if (response_status)
        *response_status = resp->status;
    
    if (response_data && response_len && resp->data_length > 0) {
        size_t copy_len = min(*response_len, (size_t)resp->data_length);
        memcpy(response_data, resp->data, copy_len);
        *response_len = copy_len;
    }
    
    ret = 0;

unlock:
    mutex_unlock(&nsi->cmd_lock);
    return ret;
}
EXPORT_SYMBOL(sunxi_nsi_send_command);

int sunxi_nsi_setup_shared_memory(struct sunxi_nsi_device *nsi, 
                                  phys_addr_t addr, size_t size)
{
    if (!nsi)
        return -EINVAL;
    
    nsi->shared_mem_phys = addr;
    nsi->shared_mem_size = size;
    
    // Configure hardware memory base
    writel(lower_32_bits(addr), nsi->base + NSI_MEM_BASE_REG);
    writel(size, nsi->base + NSI_MEM_SIZE_REG);
    
    dev_info(nsi->dev, "Shared memory configured: 0x%llx, size %zu\n", 
             (unsigned long long)addr, size);
    
    return 0;
}
EXPORT_SYMBOL(sunxi_nsi_setup_shared_memory);

struct sunxi_nsi_device *sunxi_nsi_get_device(struct device_node *np)
{
    return global_nsi_device;
}
EXPORT_SYMBOL(sunxi_nsi_get_device);

// Module initialization
static int sunxi_nsi_probe(struct platform_device *pdev)
{
    struct sunxi_nsi_device *nsi;
    struct resource *res;
    int ret;
    
    nsi = devm_kzalloc(&pdev->dev, sizeof(*nsi), GFP_KERNEL);
    if (!nsi)
        return -ENOMEM;
    
    nsi->dev = &pdev->dev;
    
    // Map registers
    res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
    nsi->base = devm_ioremap_resource(&pdev->dev, res);
    if (IS_ERR(nsi->base))
        return PTR_ERR(nsi->base);
    
    // Get interrupt
    nsi->irq = platform_get_irq(pdev, 0);
    if (nsi->irq < 0)
        return nsi->irq;
    
    // Get clocks (optional - may not be available)
    nsi->bus_clk = devm_clk_get(&pdev->dev, "bus");
    if (IS_ERR(nsi->bus_clk)) {
        dev_warn(&pdev->dev, "Failed to get bus clock, continuing without\n");
        nsi->bus_clk = NULL;
    }
    
    nsi->mips_clk = devm_clk_get(&pdev->dev, "mips");
    if (IS_ERR(nsi->mips_clk)) {
        dev_warn(&pdev->dev, "Failed to get mips clock, continuing without\n");
        nsi->mips_clk = NULL;
    }
    
    // Get reset control (optional)
    nsi->reset = devm_reset_control_get(&pdev->dev, NULL);
    if (IS_ERR(nsi->reset)) {
        dev_warn(&pdev->dev, "Failed to get reset control, continuing without\n");
        nsi->reset = NULL;
    }
    
    // Initialize synchronization primitives
    mutex_init(&nsi->cmd_lock);
    init_completion(&nsi->cmd_complete);
    atomic_set(&nsi->sequence_counter, 0);
    
    // Enable clocks if available
    if (nsi->bus_clk) {
        ret = clk_prepare_enable(nsi->bus_clk);
        if (ret) {
            dev_err(&pdev->dev, "Failed to enable bus clock\n");
            return ret;
        }
    }
    
    if (nsi->mips_clk) {
        ret = clk_prepare_enable(nsi->mips_clk);
        if (ret) {
            dev_err(&pdev->dev, "Failed to enable mips clock\n");
            goto err_bus_clk;
        }
    }
    
    // Release reset if available
    if (nsi->reset)
        reset_control_deassert(nsi->reset);
    
    // Setup shared memory (from reserved memory region)
    ret = of_reserved_mem_device_init(&pdev->dev);
    if (ret) {
        dev_warn(&pdev->dev, "Failed to initialize reserved memory, using fallback\n");
    }
    
    // Allocate command/response buffers in coherent memory
    nsi->cmd_buffer = dmam_alloc_coherent(&pdev->dev, sizeof(*nsi->cmd_buffer),
                                         &nsi->shared_mem_phys, GFP_KERNEL);
    if (!nsi->cmd_buffer) {
        ret = -ENOMEM;
        goto err_reserved_mem;
    }
    
    nsi->resp_buffer = dmam_alloc_coherent(&pdev->dev, sizeof(*nsi->resp_buffer),
                                          &nsi->shared_mem_phys, GFP_KERNEL);
    if (!nsi->resp_buffer) {
        ret = -ENOMEM;
        goto err_reserved_mem;
    }
    
    // Map the MIPS reserved memory region for command/response communication
    nsi->shared_mem_virt = ioremap(0x4b100000, 0x2800000); // 40MB MIPS region
    if (!nsi->shared_mem_virt) {
        dev_err(&pdev->dev, "Failed to map MIPS shared memory\n");
        ret = -ENOMEM;
        goto err_reserved_mem;
    }
    
    // Request interrupt
    ret = devm_request_irq(&pdev->dev, nsi->irq, sunxi_nsi_irq_handler,
                          IRQF_SHARED, dev_name(&pdev->dev), nsi);
    if (ret) {
        dev_err(&pdev->dev, "Failed to request IRQ %d\n", nsi->irq);
        goto err_shared_mem;
    }
    
    // Initialize hardware
    writel(NSI_CTRL_RESET, nsi->base + NSI_CTRL_REG);
    udelay(10);
    writel(NSI_CTRL_ENABLE | NSI_CTRL_IRQ_EN | NSI_CTRL_MEM_EN, 
           nsi->base + NSI_CTRL_REG);
    
    // Enable interrupts
    writel(NSI_IRQ_CMD_COMPLETE | NSI_IRQ_ERROR | NSI_IRQ_TIMEOUT,
           nsi->base + NSI_IRQ_EN_REG);
    
    platform_set_drvdata(pdev, nsi);
    global_nsi_device = nsi;
    
    dev_info(&pdev->dev, "SUNXI NSI driver initialized successfully\n");
    return 0;

err_shared_mem:
    iounmap(nsi->shared_mem_virt);
err_reserved_mem:
    of_reserved_mem_device_release(&pdev->dev);
    if (nsi->mips_clk)
        clk_disable_unprepare(nsi->mips_clk);
err_bus_clk:
    if (nsi->bus_clk)
        clk_disable_unprepare(nsi->bus_clk);
    return ret;
}

static void sunxi_nsi_remove(struct platform_device *pdev)
{
    struct sunxi_nsi_device *nsi = platform_get_drvdata(pdev);
    
    global_nsi_device = NULL;
    
    // Disable hardware
    writel(0, nsi->base + NSI_IRQ_EN_REG);
    writel(NSI_CTRL_RESET, nsi->base + NSI_CTRL_REG);
    
    // Cleanup memory mapping
    if (nsi->shared_mem_virt)
        iounmap(nsi->shared_mem_virt);
    
    of_reserved_mem_device_release(&pdev->dev);
    
    // Disable clocks
    if (nsi->mips_clk)
        clk_disable_unprepare(nsi->mips_clk);
    if (nsi->bus_clk)
        clk_disable_unprepare(nsi->bus_clk);
}

static const struct of_device_id sunxi_nsi_of_match[] = {
    { .compatible = "allwinner,sun50i-h713-nsi" },
    { .compatible = "allwinner,sunxi-nsi" },
    { }
};
MODULE_DEVICE_TABLE(of, sunxi_nsi_of_match);

static struct platform_driver sunxi_nsi_driver = {
    .probe = sunxi_nsi_probe,
    .remove_new = sunxi_nsi_remove,
    .driver = {
        .name = "sunxi-nsi",
        .of_match_table = sunxi_nsi_of_match,
    },
};

module_platform_driver(sunxi_nsi_driver);

MODULE_DESCRIPTION("Allwinner SUNXI Network Service Interface Driver");
MODULE_AUTHOR("HY300 Linux Porting Project");
MODULE_LICENSE("GPL v2");