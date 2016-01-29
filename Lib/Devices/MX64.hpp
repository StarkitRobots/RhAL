#pragma once

#include <string>
#include <mutex>
#include "Manager/BaseManager.hpp"
#include "Manager/Device.hpp"
#include "Manager/Register.hpp"
#include "Manager/Parameter.hpp"
#include "Devices/DXL.hpp"

namespace RhAL {

/**
 * MX64
 *
 * Dynamixel MX-64 Device 
 * implementation
 */
class MX64 : public DXL
{
    public:

        /**
         * Initialization with name and id
         */
        inline MX64(const std::string& name, id_t id) :
            DXL(name, id),
            _goalPos("goalPos", 0X1E, 2, convIn_MXPos, convOut_MXPos, 0),
            _position("pos", 0X24, 2, convIn_MXPos, convOut_MXPos, 1),
        	_torqueEnable("torqueEnable", 0X46, 1, convIn_Default<int>, convOut_Default<int>, 0)
        {
        }

        /**
         * Inherit.
         * Set the target motor 
         * position in radians
         */
        virtual void setGoalPos(float angle) override
        {
            _goalPos.writeValue(angle);
        }
        
        /**
         * Inherit.
         * Retrieve the current motor 
         * position in radians
         */
        virtual float getPos() override
        {
            return _position.readValue().value;
        }

        virtual void enableTorque() override
       	{
        	_torqueEnable.writeValue(1);
  		}

        virtual void disableTorque() override
		{
        	_torqueEnable.writeValue(0);
		}
    
        int getTorqueEnable()
        {
        	return _torqueEnable.readValue().value;
        }

    protected:

        /**
         * Inherit.
         * Declare Registers and parameters
         */
        inline virtual void onInit() override
        {
            Device::registersList().add(&_goalPos);
            Device::registersList().add(&_position);
            Device::registersList().add(&_torqueEnable);
        }

    private:

        /**
         * Register
         */
        TypedRegisterFloat _goalPos;
        TypedRegisterFloat _position;
        TypedRegisterInt _torqueEnable;
};

/**
 * DeviceManager specialized for MX64
 */
template <>
class ImplManager<MX64> : public BaseManager<MX64>
{
    public:

        inline static type_t typeNumber() 
        {
            return 0x0136;
        }

        inline static std::string typeName()
        {
            return "MX64";
        }
};

}

