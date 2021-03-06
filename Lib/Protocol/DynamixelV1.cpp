#include <stdexcept>
#include <cstdio>
#include <string.h>
#include "DynamixelV1.hpp"
#include <iostream>
#include <unistd.h>
#include <thread>

#define DEBUG 0
using namespace std;

namespace RhAL
{
DynamixelV1::Packet::Packet(id_t id, DynamixelV1Command instruction, size_t parameters_)
{
  position = 0;
  parameters = parameters_;

  buffer = new uint8_t[getSize()];
  buffer[2] = id;
  buffer[3] = parameters + 2;
  buffer[4] = instruction;
}

DynamixelV1::Packet::Packet(id_t id, size_t parameters_)
{
  position = 0;
  parameters = parameters_;

  buffer = new uint8_t[getSize()];
  buffer[2] = id;
  buffer[3] = parameters + 2;
}

DynamixelV1::Packet::~Packet()
{
  delete[] buffer;
}

void DynamixelV1::Packet::append(uint8_t byte)
{
  if (position < parameters)
  {
    buffer[5 + position] = byte;
    position++;
  }
}

void DynamixelV1::Packet::append(const uint8_t* data, size_t size)
{
  if (position + size <= parameters)
  {
    memcpy(buffer + 5 + position, data, size);
    position += size;
  }
}

uint8_t DynamixelV1::Packet::computeChecksum()
{
  uint8_t checksum = 0;
  for (size_t k = 2; k < 5 + parameters; k++)
  {
    checksum += buffer[k];
  }

  return ~checksum;
}

void DynamixelV1::Packet::prepare()
{
  buffer[0] = 0xff;
  buffer[1] = 0xff;
  buffer[5 + parameters] = computeChecksum();
}

void DynamixelV1::Packet::setError(uint8_t error)
{
  buffer[4] = error;
}

uint8_t DynamixelV1::Packet::getError()
{
  return buffer[4];
}

size_t DynamixelV1::Packet::getSize()
{
  // A packet is: Header (2), ID, length, instruction, parameters and checksum
  return 2 + 1 + 1 + 1 + parameters + 1;
}

uint8_t* DynamixelV1::Packet::getParameters()
{
  return buffer + 5;
}

DynamixelV1::DynamixelV1(Bus& bus) : Protocol(bus), _timeout("timeout", 0.01), _waitAfterWrite("waitAfterWrite", 0.0005)
{
  _parametersList.add(&_timeout);
  _parametersList.add(&_waitAfterWrite);
}

void DynamixelV1::writeData(id_t id, addr_t address, const uint8_t* data, size_t size)
{
  Packet packet(id, CommandWrite, size + 1);
  packet.append(address);
  packet.append(data, size);
  sendPacket(packet);
  // Can't talk to the servos too soon
  std::this_thread::sleep_for(TimeDurationFloat(_waitAfterWrite.value));
}

/**
 *Exactly the same structure than a read
 */
ResponseState DynamixelV1::writeAndCheckData(id_t id, addr_t address, const uint8_t* data, size_t size)
{
  return sendAndReceiveData(CommandWrite, id, address, const_cast<uint8_t*>(data), size);
}

ResponseState DynamixelV1::readData(id_t id, addr_t address, uint8_t* data, size_t size)
{
  return sendAndReceiveData(CommandRead, id, address, data, size);
}

ResponseState DynamixelV1::sendAndReceiveData(DynamixelV1Command instruction, id_t id, addr_t address, uint8_t* data,
                                              size_t size)
{
  Packet packet(id, instruction, 2);
  packet.append(address);
  packet.append(size);
  sendPacket(packet);

  Packet* response;
  auto code = receivePacket(response, id);
#if DEBUG
  if (response == NULL)
  {
    std::cout << "Response null" << endl;
  }
  else
  {
    std::cout << "Receiving packet : ";
    for (int i = 0; i < sizeof(response->buffer) / sizeof(*(response->buffer)); i++)
    {
      std::cout << (int)response->buffer[i] << " ";
    }
    std::cout << ", code = " << (int)code << endl;
    std::cout << std::endl;
  }
#endif
  if (code & ResponseOK)
  {
    memcpy(data, response->getParameters(), size);
    delete response;
  }
  return code;
}

bool DynamixelV1::ping(id_t id)
{
  Packet packet(id, CommandPing, 0);
  sendPacket(packet);

  Packet* response;
  auto code = receivePacket(response, id);
  if (code & ResponseOK)
  {
    delete response;
    return true;
  }
  else
  {
    return false;
  }
}

std::vector<ResponseState> DynamixelV1::syncSendAndReceiveData(DynamixelV1Command instruction,
                                                               const std::vector<id_t>& ids, addr_t address,
                                                               const std::vector<uint8_t*>& datas, size_t size)
{
  // Here instruction is either CommandSyncRead or CommandSyncWriteAndCheck
  Packet packet(0xfd, instruction, ids.size() + 2);  // Number of motor ids + starting address and size
  // packet.buffer[3]=ids.size()+4;     //length has to be 4+(number of ids) but packet already adds 2 so we replace
  // adress from where we start to read
  packet.append(address);
  // number of bytes to read
  packet.append(size);
  // ids
  for (size_t i = 0; i < ids.size(); i++)
    packet.append(ids[i]);

  sendPacket(packet);

  Packet* response;
  auto code = receivePacket(response, 0xfd);
#if DEBUG
  if (response == NULL)
  {
    std::cout << "Sync Response null" << endl;
  }
  else
  {
    std::cout << "Sync Receiving packet : ";
    for (int i = 0; i < sizeof(response->buffer) / sizeof(*(response->buffer)); i++)
    {
      std::cout << (int)response->buffer[i] << " ";
    }
    std::cout << ", code = " << (int)code << endl;
    std::cout << std::endl;
  }
#endif

  std::vector<ResponseState> ret;
  // returns: ID LENGTH ERROR ERROR_0 PARAM_0_0 PARAM_0_1 ... PARAM_0_N ERROR_1 PARAM_1_0 ...
  if (code & ResponseOK)
  {
    for (size_t i = 0; i < ids.size(); i++)
    {
      // printf("MOTOR: %d data: %x %x
      // %x\n",i,(uint8_t)*(response->getParameters()+i*(size+1)),(uint8_t)*(response->getParameters()+i*(size+1)+1),(uint8_t)*(response->getParameters()+i*(size+1)+2));
      unsigned int error = *(response->getParameters() + i * (size + 1));  // first the motor error code
      if (error == 0xFF)
      {
        ret.push_back(ResponseQuiet);  // motor timeout exceeded
      }
      else
      {
        if (error & ErrorChecksum)
        {  // we should probably ignore the data...
          ret.push_back(ResponseDeviceBadChecksum);
        }
        else if (error & ErrorInstruction)
        {
          ret.push_back(ResponseDeviceBadInstruction);
        }
        else
        {
          unsigned int ecode = ResponseOK;  // humm not a very good use of flags, we miss some errors...
          if (error & ErrorVoltage)
            ecode |= ResponseBadVoltage;
          if (error & ErrorOverheat)
            ecode |= ResponseOverheat;
          if (error & ErrorOverload)
            ecode |= ResponseOverload;
          ret.push_back(ecode);
        }

        memcpy(datas[i], response->getParameters() + i * (size + 1) + 1, size);
      }
    }
    delete response;
    return ret;
  }
  else
  {
    // std::cout<<"SYNC_READ ERROR: "<<code<<std::endl;
    for (size_t i = 0; i < ids.size(); i++)
      ret.push_back(code);
    delete response;

    return ret;
  }

  return ret;  // should never happen
}

std::vector<ResponseState> DynamixelV1::syncRead(const std::vector<id_t>& ids, addr_t address,
                                                 const std::vector<uint8_t*>& datas, size_t size)
{
  return syncSendAndReceiveData(CommandSyncRead, ids, address, datas, size);
}

void DynamixelV1::syncWrite(const std::vector<id_t>& ids, addr_t address, const std::vector<const uint8_t*>& datas,
                            size_t size)
{
  if (ids.size() != datas.size())
  {
    throw runtime_error("ids and datas should have the same size() for syncWrite");
  }

  size_t N = ids.size();
  Packet packet(Broadcast, CommandSyncWrite, 2 + N * (size + 1));
  packet.append(address);
  packet.append(size);
  for (size_t k = 0; k < N; k++)
  {
    packet.append(ids[k]);
    packet.append(datas[k], size);
  }
  sendPacket(packet);
  // Can't talk to the servos too soon
  std::this_thread::sleep_for(TimeDurationFloat(_waitAfterWrite.value));
}

std::vector<ResponseState> DynamixelV1::syncWriteAndCheck(const std::vector<id_t>& ids, addr_t address,
                                                          const std::vector<const uint8_t*>& datas, size_t size)
{
  return syncSendAndReceiveData(CommandSyncWriteAndCheck, ids, address,
                                reinterpret_cast<const std::vector<uint8_t*>&>(datas), size);
}

/**
 * Broadcasts a "disable torque" command
 */
void DynamixelV1::emergencyStop()
{
  // Disable torque
  uint8_t data1[1];
  data1[0] = 0;
  writeData(Broadcast, 0x18, data1, 1);
  std::this_thread::sleep_for(TimeDurationFloat(_waitAfterWrite.value));

  // Set torque limit to zero
  uint8_t data2[2];
  data2[0] = 0x00;
  data2[1] = 0x00;
  writeData(Broadcast, 0x22, data2, 2);
  std::this_thread::sleep_for(TimeDurationFloat(_waitAfterWrite.value));
}

/**
 * Broadcasts an "enable torque" command
 */
void DynamixelV1::exitEmergencyState()
{
  // Enable torque
  uint8_t data1[1];
  data1[0] = 1;
  writeData(Broadcast, 0x18, data1, 1);
  std::this_thread::sleep_for(TimeDurationFloat(_waitAfterWrite.value));

  // Set torque to maximum
  uint8_t data2[2];
  data2[0] = 0xFF;
  data2[1] = 0x03;
  writeData(Broadcast, 0x22, data2, 2);
  std::this_thread::sleep_for(TimeDurationFloat(_waitAfterWrite.value));
}

void DynamixelV1::sendPacket(Packet& packet)
{
  bus.clearInputBuffer();
  //    	bus.flushInput();
  packet.prepare();
#if DEBUG
  std::cout << "Sending packet : ";
  for (int i = 0; i < sizeof(packet.buffer) / sizeof(*packet.buffer); i++)
  {
    std::cout << (int)packet.buffer[i] << " ";
  }
  std::cout << std::endl;
#endif
  bus.sendData(packet.buffer, packet.getSize());
  bus.flush();
}

ResponseState DynamixelV1::receivePacket(Packet*& response, id_t id)
{
  ResponseState error = ResponseQuiet;
  response = NULL;
  TimePoint start = getTimePoint();
  size_t position = 0;
  while (duration_float(start, getTimePoint()) <= _timeout.value)
  {
    double t = _timeout.value - (duration_float(start, getTimePoint()));
    if (bus.waitForData(t))
    {
      size_t n = bus.available();
      uint8_t data[n];
      bus.readData(data, n);
      for (size_t k = 0; k < n; k++)
      {
        uint8_t byte = data[k];
        switch (position)
        {
          case 0:
          case 1:
            if (byte == 0xff)
            {
              position++;
            }
            else
            {
              position = 0;
            }
            break;
          case 2:
            if (byte == id)
            {
              position++;
            }
            else
            {
              error = ResponseBadId;
              position = 0;
            }
            break;
          case 3:
            if (byte >= 2)
            {
              response = new Packet(id, byte - 2);
              position++;
            }
            else
            {
              error = ResponseBadSize;
              position = 0;
            }
            break;
          case 4:
            response->setError(byte);
            position++;
            break;
          default:
            if (position - 5 < response->parameters)
            {
              response->append(byte);
            }
            else
            {
              if (response->computeChecksum() == byte)
              {
                uint8_t error = response->getError();

                if (error & ErrorChecksum)
                {
                  delete response;
                  return ResponseDeviceBadChecksum;
                }
                else if (error & ErrorInstruction)
                {
                  delete response;
                  return ResponseDeviceBadInstruction;
                }
                else
                {
                  unsigned int code = ResponseOK;
                  if (error & ErrorVoltage)
                    code |= ResponseBadVoltage;
                  if (error & ErrorOverheat)
                    code |= ResponseOverheat;
                  if (error & ErrorOverload)
                    code |= ResponseOverload;

                  return code;
                }
              }
              else
              {
                delete response;
                return ResponseBadChecksum;
              }
            }
            position++;
        }
      }
    }
  }

  return error;
}
}  // namespace RhAL
