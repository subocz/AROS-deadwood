/*
    Copyright � 2013-2020, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Low-level definitions for i8042 controller.
    Lang: English.
*/

#include <asm/io.h>

#define KBD_NO_DATA                     (-1)
#define KBD_BAD_DATA                    (-2)

/****************************************************************************************/

#define KBD_STATUS_REG                  0x64
#define KBD_CONTROL_REG                 0x64
#define KBD_DATA_REG                    0x60

#define KBD_STATUS_OBF                  (1 << 0)        /* keyboard output buffer full */
#define KBD_STATUS_IBF                  (1 << 1)        /* keyboard input buffer full */
#define KBD_STATUS_MOUSE_OBF            (1 << 5)        /* Mouse output buffer full */
#define KBD_STATUS_GTO                  (1 << 6)        /* General receive/xmit timeout */
#define KBD_STATUS_PERR                 (1 << 7)        /* Parity error */

#define KBD_CTRLCMD_READ_MODE           0x20
#define KBD_CTRLCMD_WRITE_MODE          0x60
#define KBD_CTRLCMD_GET_VERSION         0xA1
#define KBD_CTRLCMD_MOUSE_DISABLE       0xA7
#define KBD_CTRLCMD_MOUSE_ENABLE        0xA8
#define KBD_CTRLCMD_TEST_MOUSE          0xA9
#define KBD_CTRLCMD_SELF_TEST           0xAA
#define KBD_CTRLCMD_KBD_TEST            0xAB
#define KBD_CTRLCMD_KBD_DISABLE         0xAD
#define KBD_CTRLCMD_KBD_ENABLE          0xAE
#define KBD_CTRLCMD_WRITE_AUX_OBUF      0xD3
#define KBD_CTRLCMD_WRITE_MOUSE         0xD4

#define KBD_REPLY_POR                   0xAA            /* Power on reset */
#define KBD_REPLY_ACK                   0xFA            /* Command ACK */
#define KBD_REPLY_RESEND                0xFE            /* Command NACK, send the cmd again */

#define KBD_MODE_KBD_INT                0x01            /* Keyboard data generate IRQ1 */
#define KBD_MODE_MOUSE_INT              0x02            /* Mouse data generate IRQ12 */
#define KBD_MODE_SYS                    0x04            /* The system flag (?) */
#define KBD_MODE_NO_KEYLOCK             0x08            /* The keylock doesn't affect the keyboard if set */
#define KBD_MODE_DISABLE_KBD            0x10            /* Disable keyboard interface */
#define KBD_MODE_DISABLE_MOUSE          0x20            /* Disable mouse interface */
#define KBD_MODE_KCC                    0x40            /* Scan code conversion to PC format */
#define KBD_MODE_RFU                    0x80

static unsigned char kbd_read_input(void)
{
    return inb(KBD_DATA_REG);
}
static unsigned char kbd_read_status(void)
{
    return inb(KBD_STATUS_REG);
}
static void kbd_write_output(unsigned char val)
{
    outb(val, KBD_DATA_REG);
}
static void kbd_write_command(unsigned char val)
{
    outb(val, KBD_CONTROL_REG);
}

/****************************************************************************************/

int kbd_read_data(void);
void kb_wait(ULONG timeout);
void kbd_write_cmd(int cmd);
void aux_write_ack(int val);
void aux_write_noack(int val);
void kbd_write_output_w(int data);
int kbd_clear_input(void);
void kbd_write_command_w(int data); 
void kbd_usleep(LONG usec);

static int wait_for_input(ULONG timeout)
{
    do
    {
        int retval = kbd_read_data();
        if (retval >= 0)
            return retval;
        kbd_usleep(1000);
    } while(--timeout);
    return -1;
}
static int kbd_wait_for_input(void)
{
    return wait_for_input(100);
}

static int aux_wait_for_input(void)
{
    return wait_for_input(1000);
}
