/**********************************************************************

    Rockwell 10788 General Purpose Keyboard and Display circuit

    Copyright Nicola Salmoria and the MAME Team.
    Visit http://mamedev.org for licensing and usage restrictions.


    REGISTER DESCRIPTION


                 [ Opcodes IOL, I2 ]
    NAME   W/IO   CS I/O    CMD I/O    Names
    --------------------------------------------------------------
    KTR    1      1 x x x   1 1 0 0    Transfer Keyboard Return
    KTS    1      1 x x x   1 0 1 0    Transfer Keyboard Strobe
    KLA    1      1 x x x   1 1 1 0    Load Display Register A
    KLB    1      1 x x x   1 1 0 1    Load Display Register A
    KDN    1      1 x x x   0 0 1 1    Turn On Display
    KAF    1      1 x x x   1 0 1 1    Turn Off A
    KBF    1      1 x x x   0 1 1 1    Turn Off B
    KER    1      1 x x x   0 1 1 0    Reset Keyboard Error

    Notes:
    1.) W/IO is generated by the first word of the PPS IOL instruction.
    2.) Polarities of I/O7, I/O6 and I/O5 must be the same as the
        polarities of the chip select straps SC7, SC6 and SC5.
    3.) KLA resets DA1-DA4 and DB1 and DB2 to VSS level. KLB resets
        DB3 and DB4 to VSS level.
    4.) KAF and KBF is used to blank the display without changing the
        contents of display data registers.
    5.) KAF resets output lines DA1, DA2, DA3, DA4, DB1 and DB2 to
        VSS level. KBF resets output lines DB3 and DB4 to VSS level.
    6.) KAF stops the circulation of the display register A, and KBF
        stops the circulation of the display register B.
    7.) KER takes a maximum of 10-bit times to complete (= 80 clocks)
        Therefore, there must be at least 10 bit times between KER
        and the next KTS instruction.
**********************************************************************/

#include "emu.h"
#include "machine/r10788.h"

#define	VERBOSE	1
#if VERBOSE
#define LOG(x) logerror x
#else
#define LOG(x)
#endif

/*************************************
 *
 *  Device interface
 *
 *************************************/

const device_type R10788 = &device_creator<r10788_device>;

r10788_device::r10788_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock)
    : device_t(mconfig, R10788, "Rockwell 10788", tag, owner, clock, "r10788", __FILE__),
        m_reg(),
        m_ktr(0), m_kts(0), m_kla(0), m_klb(0), m_mask_a(15), m_mask_b(15), m_ker(0),
        m_io_counter(0), m_scan_counter(0),
        m_display(*this)
{
}

/**
 * @brief r10788_device::device_start device-specific startup
 */
void r10788_device::device_start()
{
    m_display.resolve();

    save_item(NAME(m_reg));
    save_item(NAME(m_ktr));
    save_item(NAME(m_kts));
    save_item(NAME(m_kla));
    save_item(NAME(m_klb));
    save_item(NAME(m_mask_a));
    save_item(NAME(m_mask_b));
    save_item(NAME(m_ker));
    save_item(NAME(m_io_counter));
    save_item(NAME(m_scan_counter));

    m_timer = timer_alloc(TIMER_DISPLAY);
    // recurring timer every 36 cycles
    m_timer->adjust(clocks_to_attotime(36), 0, clocks_to_attotime(36));
}

/**
 * @brief r10788_device::device_reset device-specific reset
 */
void r10788_device::device_reset()
{
    for (int i = 0; i < 16; i++)
        m_reg[0][i] = m_reg[1][i] = 0;
    m_ktr = 0;
    m_kts = 0;
    m_kla = 0;
    m_klb = 0;
    m_mask_a = 15;
    m_mask_b = 15;
    m_ker = 0;
    m_scan_counter = 0;
}


/**
 * @brief r10788_device::device_timer timer event callback
 * @param timer emu_timer which fired
 * @param id timer identifier
 * @param param parameter
 * @param ptr pointer parameter
 */
void r10788_device::device_timer(emu_timer &timer, device_timer_id id, int param, void *ptr)
{
    UINT8 data;
    switch (id)
    {
        case TIMER_DISPLAY:
            data = (m_reg[0][m_scan_counter] & m_mask_a) +
                   16 * (m_reg[1][m_scan_counter] & m_mask_b);
            LOG(("%s: scan counter:%2d data:%02x\n", __FUNCTION__, m_scan_counter, data));
            m_display(m_scan_counter, data, 0xff);
            break;
        default:
            LOG(("%s: invalid timer id:%d\n", __FUNCTION__, id));
    }
    m_scan_counter = (m_scan_counter + 1) % 16;
}

/*************************************
 *
 *  Constants
 *
 *************************************/

/*************************************
 *
 *  Command access handlers
 *
 *************************************/

WRITE8_MEMBER( r10788_device::io_w )
{
    assert(offset < 16);
    switch (offset)
    {
        case KTR:  // Transfer Keyboard Return
            LOG(("%s: KTR data:%02x\n", __FUNCTION__, data));
            m_ktr = data;
            break;
        case KTS:  // Transfer Keyboard Strobe
            LOG(("%s: KTS data:%02x\n", __FUNCTION__, data));
            m_kts = data;
            break;
        case KLA:  // Load Display Register A
            LOG(("%s: KLA [%2d] data:%02x\n", __FUNCTION__, m_io_counter, data));
            m_kla = data;
            m_reg[0][m_io_counter] = m_kla;
            break;
        case KLB:  // Load Display Register B
            LOG(("%s: KLB [%2d] data:%02x\n", __FUNCTION__, m_io_counter, data));
            m_klb = data;
            m_reg[1][m_io_counter] = m_kla;
            break;
        case KDN:  // Turn On Display
            LOG(("%s: KDN data:%02x\n", __FUNCTION__, data));
            m_mask_a = 15;
            m_mask_b = 15;
            break;
        case KAF:  // Turn Off A
            LOG(("%s: KAF data:%02x\n", __FUNCTION__, data));
            m_mask_a = 0;
            m_mask_b &= ~3;
            break;
        case KBF:  // Turn Off B
            LOG(("%s: KBF data:%02x\n", __FUNCTION__, data));
            m_mask_b &= ~12;
            break;
        case KER:  // Reset Keyboard Error
            LOG(("%s: KER data:%02x\n", __FUNCTION__, data));
            m_ker = 10;
            break;
    }
}


READ8_MEMBER( r10788_device::io_r )
{
    assert(offset < 16);
    UINT8 data = 0xf;
    switch (offset)
    {
        case KTR:  // Transfer Keyboard Return
            data = m_ktr;
            LOG(("%s: KTR data:%02x\n", __FUNCTION__, data));
            break;
        case KTS:  // Transfer Keyboard Strobe
            data = m_kts;
            LOG(("%s: KTS data:%02x\n", __FUNCTION__, data));
            break;
        case KLA:  // Load Display Register A
            m_kla = m_reg[0][m_io_counter];
            data = m_kla;
            LOG(("%s: KLA [%2d] data:%02x\n", __FUNCTION__, m_io_counter, data));
            break;
        case KLB:  // Load Display Register B
            m_klb = m_reg[1][m_io_counter];
            data = m_klb;
            LOG(("%s: KLB [%2d] data:%02x\n", __FUNCTION__, m_io_counter, data));
            // FIXME: does it automagically increment at KLB write?
            m_io_counter = (m_io_counter + 1) % 16;
            break;
        case KDN:  // Turn On Display
            LOG(("%s: KDN data:%02x\n", __FUNCTION__, data));
            break;
        case KAF:  // Turn Off A
            LOG(("%s: KAF data:%02x\n", __FUNCTION__, data));
            break;
        case KBF:  // Turn Off B
            LOG(("%s: KBF data:%02x\n", __FUNCTION__, data));
            break;
        case KER:  // Reset Keyboard Error
            LOG(("%s: KER data:%02x\n", __FUNCTION__, data));
            break;
    }
    return data;
}
