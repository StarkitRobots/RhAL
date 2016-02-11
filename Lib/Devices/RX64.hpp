#pragma once

#include <string>
#include <mutex>
#include "Manager/BaseManager.hpp"
#include "Manager/Device.hpp"
#include "Manager/Register.hpp"
#include "Manager/Parameter.hpp"
#include "Devices/RX.hpp"

namespace RhAL {


/**
 * RX64
 *
 * Dynamixel RX-64 Device
 * implementation
 */
class RX64 : public RX
{
    public:

        /**
         * Initialization with name and id
         */
        inline RX64(const std::string& name, id_t id) :
            RX(name, id)
        {
        }
    protected :
        /**
         * Inherit.
         * Declare Registers and parameters
         */
        inline virtual void onInit() override
        {
        	RX::onInit();
        }

        /**
         * Register
         */
        //The following comments specify the register size and address in the hardware. A '*' means that the register is not present in the all of the RX children.

};

/**
 * DeviceManager specialized for RX64
 */
template <>
class ImplManager<RX64> : public BaseManager<RX64>
{
    public:

        inline static type_t typeNumber()
        {
            return 0x0040;
        }

        inline static std::string typeName()
        {
            return "RX64";
        }
};

}
