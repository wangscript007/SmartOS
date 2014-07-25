#include "Sys.h"
#include "Port.h"
#include "Spi.h"
#include "SpiFlash.h"

#define SPI_FLASH_PageSize    0x210

#define READ       0xD2  /* Read from Memory instruction */
#define WRITE      0x82  /* Write to Memory instruction */

#define RDID       0x9F  /* Read identification */
#define RDSR       0xD7  /* Read Status Register instruction  */

#define SE         0x7C  /* Sector Erase instruction */
#define PE         0x81  /* Page Erase instruction */

#define RDY_Flag   0x80  /* Ready/busy(1/0) status flag */

#define Dummy_Byte 0xA5

void SpiFlash::SetAddr(uint addr)
{
    /* Send addr high nibble address byte */
    _spi->WriteRead((addr & 0xFF0000) >> 16);
    /* Send addr medium nibble address byte */
    _spi->WriteRead((addr & 0xFF00) >> 8);
    /* Send addr low nibble address byte */
    _spi->WriteRead(addr & 0xFF);
}

void SpiFlash::WaitForEnd()
{
    uint8_t FLASH_Status = 0;

    /* Select the FLASH: Chip Select low */
    _spi->Start();

    /* Send "Read Status Register" instruction */
    _spi->WriteRead(RDSR);

    /* Loop as long as the memory is busy with a write cycle */
    do
    {
        /* Send a dummy byte to generate the clock needed by the FLASH
        and put the value of the status register in FLASH_Status variable */
        FLASH_Status = _spi->WriteRead(Dummy_Byte);
    }
    while ((FLASH_Status & RDY_Flag) == RESET); /* Busy in progress */

    /* Deselect the FLASH: Chip Select high */
    _spi->Stop();
}

SpiFlash::SpiFlash(Spi* spi)
{
    _spi = spi;
    /* Deselect the FLASH: Chip Select high */
    _spi->Stop();
    //_spi->Start();
}

SpiFlash::~SpiFlash()
{
    delete _spi;
}

void SpiFlash::Erase(uint sector)
{
    _spi->Start();
    /* Send Sector Erase instruction */
    _spi->WriteRead(SE);
    SetAddr(sector);
    _spi->Stop();

    /* Wait the end of Flash writing */
    WaitForEnd();
}

void SpiFlash::ErasePage(uint pageAddr)
{
    _spi->Start();
    /* Send Sector Erase instruction */
    _spi->WriteRead(PE);
    SetAddr(pageAddr);
    _spi->Stop();

    /* Wait the end of Flash writing */
    WaitForEnd();
}

void SpiFlash::PageWrite(byte* buf, uint addr, uint count)
{
    _spi->Start();
    /* Send "Write to Memory " instruction */
    _spi->WriteRead(WRITE);
    SetAddr(addr);

    /* while there is data to be written on the FLASH */
    while (count--)
    {
        /* Send the current byte */
        _spi->WriteRead(*buf);
        /* Point on the next byte to be written */
        buf++;
    }

    /* Deselect the FLASH: Chip Select high */
    _spi->Stop();

    /* Wait the end of Flash writing */
    WaitForEnd();
}

void SpiFlash::Write(byte* buf, uint addr, uint count)
{
    byte addr2 = addr % SPI_FLASH_PageSize;
    byte num = SPI_FLASH_PageSize - addr2;
    byte pages =  count / SPI_FLASH_PageSize;
    byte singles = count % SPI_FLASH_PageSize;

    if (addr2 == 0) /* addr is SPI_FLASH_PageSize aligned  */
    {
        if (pages == 0) /* count < SPI_FLASH_PageSize */
        {
            PageWrite(buf, addr, count);
        }
        else /* count > SPI_FLASH_PageSize */
        {
            while (pages--)
            {
                PageWrite(buf, addr, SPI_FLASH_PageSize);
                addr +=  SPI_FLASH_PageSize;
                buf += SPI_FLASH_PageSize;
            }

            PageWrite(buf, addr, singles);
        }
        }
    else /* addr is not SPI_FLASH_PageSize aligned  */
    {
        if (pages == 0) /* count < SPI_FLASH_PageSize */
        {
            if (singles > num) /* (count + addr) > SPI_FLASH_PageSize */
            {
                byte temp = singles - num;

                PageWrite(buf, addr, num);
                addr +=  num;
                buf += num;

                PageWrite(buf, addr, temp);
            }
            else
            {
                PageWrite(buf, addr, count);
            }
        }
        else /* count > SPI_FLASH_PageSize */
        {
            count -= num;
            pages =  count / SPI_FLASH_PageSize;
            singles = count % SPI_FLASH_PageSize;

            PageWrite(buf, addr, num);
            addr +=  num;
            buf += num;

            while (pages--)
            {
                PageWrite(buf, addr, SPI_FLASH_PageSize);
                addr +=  SPI_FLASH_PageSize;
                buf += SPI_FLASH_PageSize;
            }

            if (singles != 0)
            {
                PageWrite(buf, addr, singles);
            }
        }
    }
}

void SpiFlash::Read(byte* buf, uint addr, uint count)
{
    _spi->Start();

    /* Send "Read from Memory " instruction */
    _spi->WriteRead(READ);

    SetAddr(addr);

    _spi->WriteRead(Dummy_Byte);
    _spi->WriteRead(Dummy_Byte);
    _spi->WriteRead(Dummy_Byte);
    _spi->WriteRead(Dummy_Byte);

    while (count--) /* while there is data to be read */
    {
        /* Read a byte from the FLASH */
        *buf = _spi->WriteRead(Dummy_Byte);
        /* Point to the next location where the byte read will be saved */
        buf++;
    }

    _spi->Stop();
}

uint SpiFlash::ReadID()
{
    _spi->Start();

    /* Send "RDID " instruction */
    _spi->WriteRead(RDID);

    uint Temp0 = _spi->WriteRead(Dummy_Byte);
    uint Temp1 = _spi->WriteRead(Dummy_Byte);
    uint Temp2 = _spi->WriteRead(Dummy_Byte);
    uint Temp3 = _spi->WriteRead(Dummy_Byte);

    _spi->Stop();

    return (Temp0 << 24) | (Temp1 << 16) | (Temp2<<8) | Temp3;
}
