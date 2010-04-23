/**
 * \file Serial.cpp
 * \author Denis Martinez
 */

#include "../Serial.h"

#include <sys/types.h>
#include <sys/ioctl.h>
#include <cstdlib>
#include <cerrno>
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>

#include <QDebug>

bool Serial::open(OpenMode mode)
{
    if (isOpen())
    {
        setErrorString(tr("Device (%0) already open").arg(mPort));
        return false;
    }

    int realBaudRate;
    switch (mBaudRate)
    {
    case 300: realBaudRate = B300; break;
    case 1200: realBaudRate = B1200; break;
    case 2400: realBaudRate = B2400; break;
    case 4800: realBaudRate = B4800; break;
    case 9600: realBaudRate = B9600; break;
    case 19200: realBaudRate = B19200; break;
    case 38400: realBaudRate = B38400; break;
    case 57600: realBaudRate = B57600; break;
    case 115200: realBaudRate = B115200; break;
    default:
        setErrorString(tr("Unknown baud rate %0").arg(mBaudRate));
        return false;
    }

    int realMode = O_NOCTTY;
    if (mode & ReadWrite)
        realMode |= O_RDWR;
    else if (mode & ReadOnly)
        realMode |= O_RDONLY;
    else if (mode & WriteOnly)
        realMode |= O_WRONLY;

    mSerial = ::open(mPort.toLocal8Bit().constData(), realMode);
    if (mSerial < 0)
        goto error;
    if (::fcntl(mSerial, F_SETFL, FNDELAY) == -1)
        goto error;

    struct termios serial_params;
    if (::tcgetattr(mSerial, &serial_params) == -1)
        goto error;
    if (::cfsetispeed(&serial_params, realBaudRate) == -1)
        goto error;
    if (::cfsetospeed(&serial_params, realBaudRate) == -1)
        goto error;
    cfmakeraw(&serial_params);
    if (::tcsetattr(mSerial, TCSANOW, &serial_params) == -1)
        goto error;

    setOpenMode(mode);
    return true;

error:
    setErrorString(strerror(errno));
    if (mSerial >= 0)
    {
        ::close(mSerial);
        mSerial = -1;
    }
    return false;
}

void Serial::close()
{
    emit aboutToClose();
    if (isOpen())
        ::close(mSerial);
    setOpenMode(NotOpen);
    setErrorString(QString());
}

qint64 Serial::readData(char *data, qint64 maxSize)
{
    ssize_t n = ::read(mSerial, data, maxSize);
    qDebug() << n;
    if (n < 0)
        goto error;
    else
        return n;

error:
   setErrorString(strerror(errno));
   return -1;
}

qint64 Serial::writeData(const char *data, qint64 maxSize)
{
    ssize_t n = ::write(mSerial, data, maxSize);
    if (n < 0)
        goto error;
    else
        return n;

error:
   setErrorString(strerror(errno));
   return -1;
}

bool Serial::setDTR(bool enable)
{
    if (! isOpen())
        return false;

    unsigned int result = 0;
    if (ioctl(mSerial, TIOCMGET, &result) == -1)
        return false;
    if (enable)
        result |= TIOCM_DTR;
    else
        result &= ~ TIOCM_DTR;
    if (ioctl(mSerial, TIOCMSET, &result) == -1)
        return false;
    return true;
}